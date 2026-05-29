#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define AVB_MAGIC "AVB0"
#define AVB_VBMETA_IMAGE_HEADER_SIZE 256
#define AVB_FOOTER_MAGIC "AVBf"
#define AVB_FOOTER_SIZE 64

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

/* returns 1 if footer found, 0 otherwise */
static int read_avb_footer(const uint8_t *data, size_t len,
                           uint64_t *original_size, uint64_t *vbmeta_offset,
                           uint64_t *vbmeta_size) {
    if (len < AVB_FOOTER_SIZE)
        return 0;

    const uint8_t *footer = data + len - AVB_FOOTER_SIZE;
    if (memcmp(footer, AVB_FOOTER_MAGIC, 4) != 0)
        return 0;

    *original_size  = be64(footer + 12);
    *vbmeta_offset  = be64(footer + 20);
    *vbmeta_size    = be64(footer + 28);
    return 1;
}

static void create_avb_footer(uint8_t *footer,
                               uint64_t original_size, uint64_t vbmeta_offset,
                               uint64_t vbmeta_size) {
    memset(footer, 0, AVB_FOOTER_SIZE);
    memcpy(footer, AVB_FOOTER_MAGIC, 4);
    put_be32(footer + 4, 1);  /* version major */
    put_be32(footer + 8, 0);  /* version minor */
    put_be64(footer + 12, original_size);
    put_be64(footer + 20, vbmeta_offset);
    put_be64(footer + 28, vbmeta_size);
}

static int verify_avb_header(const uint8_t *data, size_t len) {
    if (len < AVB_VBMETA_IMAGE_HEADER_SIZE)
        return -1;
    if (memcmp(data, AVB_MAGIC, 4) != 0)
        return -1;
    return 0;
}

static int transplant_vbmeta(const char *source_vbmeta, const char *target_image,
                              const char *output_image) {
    printf("======================================================================\n");
    printf("VBMeta Transplant Tool\n");
    printf("======================================================================\n\n");

    /* 1. Read source vbmeta */
    printf("[Step 1] Read source VBMeta\n");
    printf("----------------------------------------------------------------------\n");
    size_t vbmeta_size;
    uint8_t *vbmeta_data = read_file(source_vbmeta, &vbmeta_size);
    if (!vbmeta_data) {
        fprintf(stderr, "Failed to read source vbmeta: %s\n", source_vbmeta);
        return -1;
    }
    printf("Source VBMeta: %s (%zu bytes)\n\n", source_vbmeta, vbmeta_size);

    /* 2. Read target image */
    printf("[Step 2] Read target image\n");
    printf("----------------------------------------------------------------------\n");
    size_t target_size;
    uint8_t *target_data = read_file(target_image, &target_size);
    if (!target_data) {
        fprintf(stderr, "Failed to read target image: %s\n", target_image);
        free(vbmeta_data);
        return -1;
    }
    printf("Target image: %s (%zu bytes)\n", target_image, target_size);

    uint64_t original_size;
    uint64_t existing_vbmeta_offset, existing_vbmeta_size;
    if (read_avb_footer(target_data, target_size,
                        &original_size, &existing_vbmeta_offset, &existing_vbmeta_size)) {
        printf("  Target image has existing VBMeta\n");
        printf("  Original data size: %llu\n", (unsigned long long)original_size);
    } else {
        printf("  Target image has no VBMeta, assuming entire image is original data\n");
        original_size = target_size - vbmeta_size - AVB_FOOTER_SIZE;
        printf("  Calculated original data size: %llu\n", (unsigned long long)original_size);
    }
    printf("\n");

    /* 3. Calculate layout */
    printf("[Step 3] Calculate new image layout\n");
    printf("----------------------------------------------------------------------\n");

    uint64_t vbmeta_offset = original_size;
    uint64_t footer_offset = target_size - AVB_FOOTER_SIZE;

    printf("  Partition size: %zu\n", target_size);
    printf("  Original data size: %llu (0x0 - 0x%llx)\n",
           (unsigned long long)original_size, (unsigned long long)original_size);
    printf("  VBMeta offset: %llu (0x%llx)\n",
           (unsigned long long)vbmeta_offset, (unsigned long long)vbmeta_offset);
    printf("  VBMeta size: %zu\n", vbmeta_size);
    printf("  Footer offset: %llu (0x%llx)\n",
           (unsigned long long)footer_offset, (unsigned long long)footer_offset);
    printf("  Footer size: %d\n", AVB_FOOTER_SIZE);

    uint64_t required_size = original_size + vbmeta_size + AVB_FOOTER_SIZE;
    if (required_size > target_size) {
        fprintf(stderr, "Insufficient space: need %llu bytes, only %zu available\n",
                (unsigned long long)required_size, target_size);
        free(vbmeta_data);
        free(target_data);
        return -1;
    }

    uint64_t padding_size = footer_offset - (vbmeta_offset + vbmeta_size);
    printf("  Padding size: %llu\n\n", (unsigned long long)padding_size);

    /* 4. Assemble new image */
    printf("[Step 4] Assemble new image\n");
    printf("----------------------------------------------------------------------\n");

    uint8_t *output_data = calloc(1, target_size);
    if (!output_data) {
        fprintf(stderr, "Memory allocation failed\n");
        free(vbmeta_data);
        free(target_data);
        return -1;
    }

    memcpy(output_data, target_data, (size_t)original_size);
    printf("  Written original data: %llu bytes\n", (unsigned long long)original_size);

    memcpy(output_data + vbmeta_offset, vbmeta_data, vbmeta_size);
    printf("  Written VBMeta: %zu bytes @ 0x%llx\n", vbmeta_size, (unsigned long long)vbmeta_offset);

    if (padding_size > 0)
        printf("  Padding: %llu bytes\n", (unsigned long long)padding_size);

    create_avb_footer(output_data + footer_offset, original_size, vbmeta_offset, vbmeta_size);
    printf("  Written Footer: %d bytes @ 0x%llx\n\n", AVB_FOOTER_SIZE, (unsigned long long)footer_offset);

    free(target_data);
    free(vbmeta_data);

    /* 5. Write output file */
    printf("[Step 5] Write output file\n");
    printf("----------------------------------------------------------------------\n");
    if (write_file(output_image, output_data, target_size) != 0) {
        fprintf(stderr, "Failed to write output file: %s\n", output_image);
        free(output_data);
        return -1;
    }
    printf("Written: %s (%zu bytes)\n\n", output_image, target_size);

    /* 6. Verify */
    printf("[Step 6] Verify output image\n");
    printf("----------------------------------------------------------------------\n");

    uint64_t v_orig, v_offset, v_size;
    if (read_avb_footer(output_data, target_size, &v_orig, &v_offset, &v_size)) {
        printf("  Footer verification passed\n");
        printf("    Original image size: %llu\n", (unsigned long long)v_orig);
        printf("    VBMeta offset: %llu\n", (unsigned long long)v_offset);
        printf("    VBMeta size: %llu\n", (unsigned long long)v_size);

        if (verify_avb_header(output_data + v_offset, (size_t)v_size) == 0)
            printf("  VBMeta data verification passed\n");
        else
            fprintf(stderr, "  VBMeta data verification failed\n");
    } else {
        fprintf(stderr, "  Footer verification failed\n");
    }

    free(output_data);

    printf("\n======================================================================\n");
    printf("Transplant complete!\n");
    printf("======================================================================\n");
    return 0;
}

static void usage(const char *prog) {
    printf("Usage: %s <source_vbmeta> <target_image> <output_image>\n\n", prog);
    printf("Description:\n");
    printf("  - source_vbmeta: vbmeta data to transplant (.vbmeta file)\n");
    printf("  - target_image:  image to receive vbmeta (partition size preserved)\n");
    printf("  - output_image:  output image with transplanted vbmeta\n\n");
    printf("Notes:\n");
    printf("  - Output image size equals target image size\n");
    printf("  - AVB Footer is placed at the last 64 bytes of the partition\n");
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }

    const char *source_vbmeta = argv[1];
    const char *target_image  = argv[2];
    const char *output_image  = argv[3];

    FILE *f = fopen(source_vbmeta, "rb");
    if (!f) {
        fprintf(stderr, "Source vbmeta not found: %s\n", source_vbmeta);
        return 1;
    }
    fclose(f);

    f = fopen(target_image, "rb");
    if (!f) {
        fprintf(stderr, "Target image not found: %s\n", target_image);
        return 1;
    }
    fclose(f);

    if (transplant_vbmeta(source_vbmeta, target_image, output_image) != 0)
        return 1;

    return 0;
}
