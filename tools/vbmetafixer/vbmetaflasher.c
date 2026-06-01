#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_p(d) _mkdir(d)
#define PATH_SEP '\\'
#else
#include <unistd.h>
#include <sys/stat.h>
#define mkdir_p(d) mkdir(d, 0755)
#define PATH_SEP '/'
#endif

#define AVB_MAGIC "AVB0"
#define AVB_VBMETA_IMAGE_HEADER_SIZE 256
#define AVB_FOOTER_MAGIC "AVBf"
#define AVB_FOOTER_SIZE 64

#define MAX_PATH_LEN 512
#define MAX_CMD_LEN 1024
#define MAX_INPUT_LEN 256

static const char *fastboot_path = "fastboot";

/* ---- big-endian helpers ---- */

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

static uint64_t be64(const uint8_t *p) {
    return ((uint64_t)be32(p) << 32) | be32(p + 4);
}

static void put_be32(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8)  & 0xff;
    p[3] = v & 0xff;
}

static void put_be64(uint8_t *p, uint64_t v) {
    put_be32(p, (uint32_t)(v >> 32));
    put_be32(p + 4, (uint32_t)v);
}

/* ---- file I/O ---- */

static uint8_t *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) { fclose(f); return NULL; }

    uint8_t *buf = malloc(sz);
    if (!buf) { fclose(f); return NULL; }

    if (fread(buf, 1, sz, f) != (size_t)sz) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *out_size = sz;
    return buf;
}

static int write_file(const char *path, const uint8_t *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    if (fwrite(data, 1, size, f) != size) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* ---- exe directory resolution ---- */

static int get_exe_dir(char *buf, size_t buf_size) {
#ifdef _WIN32
    /* GetModuleFileName */
    extern unsigned long __stdcall GetModuleFileNameA(void*, char*, unsigned long);
    unsigned long len = GetModuleFileNameA(NULL, buf, (unsigned long)buf_size);
    if (len == 0 || len >= buf_size) return -1;
#else
    ssize_t len = readlink("/proc/self/exe", buf, buf_size - 1);
    if (len <= 0) return -1;
    buf[len] = '\0';
#endif
    /* strip filename, keep directory */
    char *last_sep = strrchr(buf, PATH_SEP);
#ifdef _WIN32
    if (!last_sep) last_sep = strrchr(buf, '/');
#endif
    if (last_sep)
        *last_sep = '\0';
    else
        strcpy(buf, ".");
    return 0;
}

/* ---- AVB footer / transplant ---- */

static int read_avb_footer(const uint8_t *data, size_t len,
                           uint64_t *original_size, uint64_t *vbmeta_offset,
                           uint64_t *vbmeta_size) {
    if (len < AVB_FOOTER_SIZE)
        return 0;

    const uint8_t *footer = data + len - AVB_FOOTER_SIZE;
    if (memcmp(footer, AVB_FOOTER_MAGIC, 4) != 0)
        return 0;

    *original_size = be64(footer + 12);
    *vbmeta_offset = be64(footer + 20);
    *vbmeta_size   = be64(footer + 28);
    return 1;
}

static void create_avb_footer(uint8_t *footer,
                               uint64_t original_size, uint64_t vbmeta_offset,
                               uint64_t vbmeta_size) {
    memset(footer, 0, AVB_FOOTER_SIZE);
    memcpy(footer, AVB_FOOTER_MAGIC, 4);
    put_be32(footer + 4, 1);
    put_be32(footer + 8, 0);
    put_be64(footer + 12, original_size);
    put_be64(footer + 20, vbmeta_offset);
    put_be64(footer + 28, vbmeta_size);
}

static int transplant_vbmeta(const char *vbmeta_path, const char *source_image,
                              const char *output_path) {
    size_t vbmeta_size;
    uint8_t *vbmeta_data = read_file(vbmeta_path, &vbmeta_size);
    if (!vbmeta_data) {
        fprintf(stderr, "Failed to read vbmeta: %s\n", vbmeta_path);
        return -1;
    }

    size_t target_size;
    uint8_t *target_data = read_file(source_image, &target_size);
    if (!target_data) {
        fprintf(stderr, "Failed to read source image: %s\n", source_image);
        free(vbmeta_data);
        return -1;
    }

    uint64_t original_size;
    uint64_t existing_offset, existing_size;
    if (read_avb_footer(target_data, target_size,
                        &original_size, &existing_offset, &existing_size)) {
        printf("  Target has existing VBMeta, original data size: %llu\n",
               (unsigned long long)original_size);
    } else {
        original_size = target_size - vbmeta_size - AVB_FOOTER_SIZE;
        printf("  Target has no VBMeta, calculated original size: %llu\n",
               (unsigned long long)original_size);
    }

    uint64_t vbmeta_offset = original_size;
    uint64_t footer_offset = target_size - AVB_FOOTER_SIZE;
    uint64_t required = original_size + vbmeta_size + AVB_FOOTER_SIZE;

    if (required > target_size) {
        fprintf(stderr, "Insufficient space: need %llu, have %llu\n",
                (unsigned long long)required, (unsigned long long)target_size);
        free(vbmeta_data);
        free(target_data);
        return -1;
    }

    uint8_t *output = calloc(1, target_size);
    if (!output) {
        free(vbmeta_data);
        free(target_data);
        return -1;
    }

    memcpy(output, target_data, (size_t)original_size);
    memcpy(output + vbmeta_offset, vbmeta_data, vbmeta_size);
    create_avb_footer(output + footer_offset, original_size, vbmeta_offset, vbmeta_size);

    free(target_data);
    free(vbmeta_data);

    if (write_file(output_path, output, target_size) != 0) {
        fprintf(stderr, "Failed to write transplanted image: %s\n", output_path);
        free(output);
        return -1;
    }

    /* verify */
    uint64_t v_orig, v_off, v_sz;
    if (read_avb_footer(output, target_size, &v_orig, &v_off, &v_sz)) {
        if (v_off + v_sz <= target_size &&
            memcmp(output + v_off, AVB_MAGIC, 4) == 0) {
            printf("  VBMeta transplant verified OK\n");
        } else {
            fprintf(stderr, "  VBMeta transplant verification failed\n");
            free(output);
            return -1;
        }
    }

    free(output);
    return 0;
}

/* ---- partition name helpers ---- */

static void strip_slot_suffix(const char *partition, char *base, size_t base_size) {
    size_t len = strlen(partition);

    if (len > 3 && strcmp(partition + len - 3, "_ab") == 0) {
        snprintf(base, base_size, "%.*s", (int)(len - 3), partition);
    } else if (len > 2 && (strcmp(partition + len - 2, "_a") == 0 ||
                           strcmp(partition + len - 2, "_b") == 0)) {
        snprintf(base, base_size, "%.*s", (int)(len - 2), partition);
    } else {
        snprintf(base, base_size, "%s", partition);
    }
}

/* ---- backup check ---- */

static int run_backup(const char *exe_dir) {
    char backup_bin[MAX_PATH_LEN];
    char vbmetas_dir[MAX_PATH_LEN];
    char cmd[MAX_CMD_LEN];

#ifdef _WIN32
    snprintf(backup_bin, sizeof(backup_bin), "%s\\bin\\vbmetabackup.exe", exe_dir);
#else
    snprintf(backup_bin, sizeof(backup_bin), "%s/bin/vbmetabackup", exe_dir);
#endif
    snprintf(vbmetas_dir, sizeof(vbmetas_dir), "%s%cvbmetas", exe_dir, PATH_SEP);

    if (!file_exists(backup_bin)) {
        fprintf(stderr, "Backup tool not found: %s\n", backup_bin);
        return -1;
    }

    printf("==========================================================\n");
    printf("No backup found. VBMeta backup is required before flashing.\n");
    printf("==========================================================\n\n");
    printf("Please:\n");
    printf("  1. Reboot device to Android\n");
    printf("  2. Connect via USB\n");
    printf("  3. Grant root access to ADB shell\n\n");
    printf("Press Enter to start backup...");
    fflush(stdout);
    getchar();

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "cmd /C \"\"%s\" -o \"%s\"\"", backup_bin, vbmetas_dir);
#else
    snprintf(cmd, sizeof(cmd), "\"%s\" -o \"%s\"", backup_bin, vbmetas_dir);
#endif
    printf("\n");
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Backup failed (ret=%d)\n", ret);
        return -1;
    }

    return 0;
}

static int check_and_run_backup(const char *exe_dir) {
    char marker[MAX_PATH_LEN];
    snprintf(marker, sizeof(marker), "%s%cfinish_backup", exe_dir, PATH_SEP);

    if (file_exists(marker))
        return 0;

    if (run_backup(exe_dir) != 0)
        return -1;

    /* create marker */
    FILE *f = fopen(marker, "w");
    if (f) {
        fprintf(f, "done\n");
        fclose(f);
    }
    printf("\nBackup complete. Marker created: %s\n\n", marker);
    return 0;
}

/* ---- flash ---- */

static int flash_partition(const char *partition, const char *image_path) {
    char cmd[MAX_CMD_LEN];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "cmd /C \"\"%s\" flash %s \"%s\"\"", fastboot_path, partition, image_path);
#else
    snprintf(cmd, sizeof(cmd), "\"%s\" flash %s \"%s\"", fastboot_path, partition, image_path);
#endif
    printf("$ %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0)
        fprintf(stderr, "Flash failed (ret=%d)\n", ret);
    return ret;
}

/* ---- input helpers ---- */

static void read_line(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, (int)size, stdin))
        buf[strcspn(buf, "\r\n")] = '\0';
}

/* ---- main ---- */

static void usage(const char *prog) {
    printf("Usage: %s [-f fastboot_path] [partition] [image]\n\n", prog);
    printf("  partition  fastboot partition name (e.g. boot_a, boot_ab, vbmeta_b)\n");
    printf("  image      path to the image file to flash\n");
    printf("  -f         path to fastboot executable (default: fastboot)\n\n");
    printf("If partition or image is omitted, you will be prompted interactively.\n");
    printf("Slot suffixes (_a, _b, _ab) are stripped to find matching vbmeta backup.\n");
}

int main(int argc, char **argv) {
    char exe_dir[MAX_PATH_LEN];
    if (get_exe_dir(exe_dir, sizeof(exe_dir)) != 0) {
        fprintf(stderr, "Failed to determine executable directory\n");
        return 1;
    }
    printf("Working directory: %s\n", exe_dir);

    /* parse args */
    const char *partition_arg = NULL;
    const char *image_arg = NULL;
    int positional = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            fastboot_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            if (positional == 0)
                partition_arg = argv[i];
            else if (positional == 1)
                image_arg = argv[i];
            else {
                usage(argv[0]);
                return 1;
            }
            positional++;
        }
    }

    /* backup check */
    if (check_and_run_backup(exe_dir) != 0)
        return 1;

    /* get partition */
    char partition_buf[MAX_INPUT_LEN];
    if (!partition_arg) {
        read_line("Partition name (e.g. boot_a, boot_ab): ", partition_buf, sizeof(partition_buf));
        if (partition_buf[0] == '\0') {
            fprintf(stderr, "No partition specified\n");
            return 1;
        }
        partition_arg = partition_buf;
    }

    /* get image path */
    char image_buf[MAX_PATH_LEN];
    if (!image_arg) {
        read_line("Image file path: ", image_buf, sizeof(image_buf));
        if (image_buf[0] == '\0') {
            fprintf(stderr, "No image specified\n");
            return 1;
        }
        image_arg = image_buf;
    }

    if (!file_exists(image_arg)) {
        fprintf(stderr, "Image file not found: %s\n", image_arg);
        return 1;
    }

    /* derive base partition name */
    char base_name[MAX_INPUT_LEN];
    strip_slot_suffix(partition_arg, base_name, sizeof(base_name));
    printf("Partition: %s (base: %s)\n", partition_arg, base_name);

    /* check for vbmeta backup */
    char vbmeta_path[MAX_PATH_LEN];
    snprintf(vbmeta_path, sizeof(vbmeta_path), "%s%cvbmetas%c%s.vbmeta",
             exe_dir, PATH_SEP, PATH_SEP, base_name);

    const char *flash_image = image_arg;
    char temp_image[MAX_PATH_LEN] = {0};

    if (file_exists(vbmeta_path)) {
        printf("Found vbmeta backup: %s\n", vbmeta_path);
        printf("Transplanting vbmeta...\n");

        char temp_dir[MAX_PATH_LEN];
        snprintf(temp_dir, sizeof(temp_dir), "%s%ctemp", exe_dir, PATH_SEP);
        mkdir_p(temp_dir);

        snprintf(temp_image, sizeof(temp_image), "%s%c%s.img",
                 temp_dir, PATH_SEP, partition_arg);

        if (transplant_vbmeta(vbmeta_path, image_arg, temp_image) != 0) {
            fprintf(stderr, "VBMeta transplant failed, aborting\n");
            remove(temp_image);
            return 1;
        }

        printf("VBMeta transplanted -> %s\n", temp_image);
        flash_image = temp_image;
    } else {
        printf("No vbmeta backup for '%s', flashing original image\n", base_name);
    }

    /* flash */
    printf("\nFlashing %s -> %s\n", flash_image, partition_arg);
    int ret = flash_partition(partition_arg, flash_image);

    /* cleanup temp */
    if (temp_image[0])
        remove(temp_image);

    if (ret == 0)
        printf("\nFlash complete!\n");

    return ret;
}
