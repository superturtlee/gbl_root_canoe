#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int inject_elf(const char *elf_path, const char *abl_path, const char *output_path)
{
    FILE *f;
    uint8_t *elf_data = NULL, *abl_data = NULL, *output = NULL;
    size_t elf_size, abl_size;
    int ret = 1;

    f = fopen(elf_path, "rb");
    if (!f) { perror("open elf"); return 1; }
    fseek(f, 0, SEEK_END);
    elf_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    elf_data = malloc(elf_size);
    if (fread(elf_data, 1, elf_size, f) != elf_size) { perror("read elf"); fclose(f); goto out; }
    fclose(f);

    f = fopen(abl_path, "rb");
    if (!f) { perror("open abl"); goto out; }
    fseek(f, 0, SEEK_END);
    abl_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    abl_data = malloc(abl_size);
    if (fread(abl_data, 1, abl_size, f) != abl_size) { perror("read abl"); fclose(f); goto out; }
    fclose(f);

    printf("ELF: %zu bytes, ABL: %zu bytes\n", elf_size, abl_size);

    uint64_t e_phoff, e_shoff;
    uint16_t e_phentsize, e_phnum, e_shentsize, e_shnum;
    memcpy(&e_phoff, elf_data + 32, 8);
    memcpy(&e_shoff, elf_data + 40, 8);
    memcpy(&e_phentsize, elf_data + 54, 2);
    memcpy(&e_phnum, elf_data + 56, 2);
    memcpy(&e_shentsize, elf_data + 58, 2);
    memcpy(&e_shnum, elf_data + 60, 2);
    const uint8_t signature[] = "ABL_PLACEHOLDER!";
    size_t placeholder_offset = 0;
    int found = 0;
    for (size_t i = 0; i <= elf_size - 16; i++) {
        if (memcmp(elf_data + i, signature, 16) == 0) {
            placeholder_offset = i;
            found = 1;
            break;
        }
    }
    if (!found) {
        fprintf(stderr, "ERROR: Placeholder not found\n");
      goto out;
    }

    size_t size_offset = placeholder_offset - 4;
    uint32_t old_size;
    memcpy(&old_size, elf_data + size_offset, 4);
    printf("Placeholder at 0x%zX, size: %u\n", placeholder_offset, old_size);

    int target_idx = -1;
    uint64_t target_off = 0, target_size = 0;
    for (int i = 0; i < e_shnum; i++) {
        size_t sh_pos = e_shoff + i * e_shentsize;
        uint64_t sh_off, sh_size;
        memcpy(&sh_off, elf_data + sh_pos + 24, 8);
        memcpy(&sh_size, elf_data + sh_pos + 32, 8);
        if (sh_off <= placeholder_offset && placeholder_offset < sh_off + sh_size) {
          target_idx = i;
            target_off = sh_off;
            target_size = sh_size;
         printf("Section %d: offset=0x%lX, size=0x%lX\n", i, (unsigned long)sh_off, (unsigned long)sh_size);
            break;
        }
    }
    if (target_idx < 0) {
        fprintf(stderr, "ERROR: Containing section not found\n");
        goto out;
    }

    size_t old_end = placeholder_offset + old_size;
    size_t remaining = (target_off + target_size) - old_end;
    size_t new_content = (size_offset - target_off + 4) + abl_size + remaining;
    uint64_t new_size = (new_content + 7) & ~(uint64_t)7;
    int64_t diff = (int64_t)new_size - (int64_t)target_size;
    printf("New section size: 0x%lX (+0x%lX)\n", (unsigned long)new_size, (unsigned long)diff);

    size_t output_size = elf_size + diff;
    output = calloc(1, output_size);

    memcpy(output, elf_data, size_offset);

    uint32_t abl_size32 = (uint32_t)abl_size;
    memcpy(output + size_offset, &abl_size32, 4);
    memcpy(output + size_offset + 4, abl_data, abl_size);

    if (remaining > 0) {
        size_t dest = size_offset + 4 + abl_size;
        memcpy(output + dest, elf_data + old_end, remaining);
    }

    size_t section_old_end = target_off + target_size;
    size_t section_new_end = target_off + new_size;
    memcpy(output + section_new_end, elf_data + section_old_end, elf_size - section_old_end);

    uint64_t new_shoff = (e_shoff < section_old_end) ? e_shoff : e_shoff + diff;
    for (int i = 0; i < e_shnum; i++) {
        size_t old_sh_pos = e_shoff + i * e_shentsize;
        size_t new_sh_pos = new_shoff + i * e_shentsize;

        memcpy(output + new_sh_pos, elf_data + old_sh_pos, e_shentsize);

        if (i == target_idx) {
            memcpy(output + new_sh_pos + 32, &new_size, 8);
        } else {
            uint64_t sh_off;
            memcpy(&sh_off, elf_data + old_sh_pos + 24, 8);
            if (sh_off > section_old_end) {
                uint64_t new_off = sh_off + diff;
                memcpy(output + new_sh_pos + 24, &new_off, 8);
            }
        }
    }

    memcpy(output + 40, &new_shoff, 8);

    for (int i = 0; i < e_phnum; i++) {
        size_t ph_pos = e_phoff + i * e_phentsize;
     uint64_t p_off, p_filesz, p_memsz;
        memcpy(&p_off, elf_data + ph_pos + 8, 8);
        memcpy(&p_filesz, elf_data + ph_pos + 32, 8);
        memcpy(&p_memsz, elf_data + ph_pos + 40, 8);

        if (p_off <= target_off && target_off < p_off + p_filesz) {
            uint64_t new_filesz = p_filesz + diff;
          uint64_t new_memsz = p_memsz + diff;
            memcpy(output + ph_pos + 32, &new_filesz, 8);
            memcpy(output + ph_pos + 40, &new_memsz, 8);
            printf("Updated PH %d: FileSiz=0x%lX\n", i, (unsigned long)new_filesz);
        } else if (p_off > section_old_end) {
            uint64_t new_off = p_off + diff;
            memcpy(output + ph_pos + 8, &new_off, 8);
        }
    }

    f = fopen(output_path, "wb");
    if (!f) { perror("open output"); goto out; }
    fwrite(output, 1, output_size, f);
    fclose(f);
    printf("Created %s\n", output_path);
    ret = 0;

out:
    free(elf_data);
    free(abl_data);
    free(output);
    return ret;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input.dll> <abl.efi> <output.dll>\n", argv[0]);
        return 1;
    }
    return inject_elf(argv[1], argv[2], argv[3]);
}
