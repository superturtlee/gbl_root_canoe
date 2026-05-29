#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_p(d) _mkdir(d)
#else
#include <sys/stat.h>
#define mkdir_p(d) mkdir(d, 0755)
#endif

#define AVB_MAGIC "AVB0"
#define AVB_VBMETA_IMAGE_HEADER_SIZE 256
#define AVB_FOOTER_MAGIC "AVBf"
#define AVB_FOOTER_SIZE 64
#define AVB_DESCRIPTOR_TAG_CHAIN_PARTITION 4

#define DEVICE_TEMP_DIR "/data/local/tmp"
#define MAX_CHAINS 32
#define MAX_PATH_LEN 512
#define MAX_CMD_LEN 1024
#define MAX_PARTITION_NAME 128

static const char *adb_path = "adb";

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

static uint64_t be64(const uint8_t *p) {
    return ((uint64_t)be32(p) << 32) | be32(p + 4);
}

struct avb_header {
    uint64_t auth_block_size;
    uint64_t aux_block_size;
    uint64_t descriptors_offset;
    uint64_t descriptors_size;
};

struct chain_partition {
    char name[MAX_PARTITION_NAME];
    uint32_t rollback_index_location;
};

static int parse_avb_header(const uint8_t *data, size_t len, struct avb_header *hdr) {
    if (len < AVB_VBMETA_IMAGE_HEADER_SIZE)
        return -1;
    if (memcmp(data, AVB_MAGIC, 4) != 0)
        return -1;

    hdr->auth_block_size    = be64(data + 12);
    hdr->aux_block_size     = be64(data + 20);
    hdr->descriptors_offset = be64(data + 96);
    hdr->descriptors_size   = be64(data + 104);
    return 0;
}

static size_t vbmeta_size(const struct avb_header *hdr) {
    return AVB_VBMETA_IMAGE_HEADER_SIZE + hdr->auth_block_size + hdr->aux_block_size;
}

/*
 * Search for AVB footer by scanning backwards for "AVBf" magic.
 * dd dumps the entire block device, so the footer may not be at the
 * very end — there can be trailing zeros after the real partition data.
 */
static int find_avb_footer(const uint8_t *data, size_t len,
                           uint64_t *vb_offset, uint64_t *vb_size) {
    if (len < AVB_FOOTER_SIZE)
        return 0;

    /* scan backwards in 4-byte steps looking for "AVBf" */
    size_t pos = len - AVB_FOOTER_SIZE;
    while (1) {
        if (memcmp(data + pos, AVB_FOOTER_MAGIC, 4) == 0) {
            *vb_offset = be64(data + pos + 20);
            *vb_size   = be64(data + pos + 28);
            if (*vb_offset + *vb_size <= len)
                return 1;
        }
        if (pos < 4)
            break;
        pos -= 4;
    }
    return 0;
}

/* returns vbmeta offset and size within partition data; -1 on error */
static int locate_vbmeta(const uint8_t *data, size_t len,
                         size_t *out_offset, size_t *out_size) {
    uint64_t vb_offset, vb_size;
    if (find_avb_footer(data, len, &vb_offset, &vb_size)) {
        *out_offset = (size_t)vb_offset;
        *out_size   = (size_t)vb_size;
        printf("  Found AVB Footer: vbmeta offset=%zu, size=%zu\n", *out_offset, *out_size);
        return 0;
    }

    /* no footer, read header directly */
    struct avb_header hdr;
    if (parse_avb_header(data, len, &hdr) != 0)
        return -1;

    *out_offset = 0;
    *out_size   = vbmeta_size(&hdr);
    if (*out_size > len)
        return -1;

    printf("  No footer, reading vbmeta directly: size=%zu\n", *out_size);
    return 0;
}

static int parse_chain_partitions(const uint8_t *vbmeta, size_t vbmeta_len,
                                  struct chain_partition *chains, int max_chains) {
    struct avb_header hdr;
    if (parse_avb_header(vbmeta, vbmeta_len, &hdr) != 0)
        return 0;
    if (hdr.descriptors_size == 0)
        return 0;

    size_t aux_start = AVB_VBMETA_IMAGE_HEADER_SIZE + hdr.auth_block_size;
    const uint8_t *aux = vbmeta + aux_start;
    size_t aux_len = vbmeta_len - aux_start;

    size_t offset = hdr.descriptors_offset;
    size_t end = hdr.descriptors_offset + hdr.descriptors_size;
    int count = 0;

    while (offset + 16 <= end && offset + 16 <= aux_len && count < max_chains) {
        uint64_t tag = be64(aux + offset);
        uint64_t num_bytes = be64(aux + offset + 8);
        size_t desc_end = offset + 16 + num_bytes;

        if (desc_end > end || desc_end > aux_len)
            break;

        if (tag == AVB_DESCRIPTOR_TAG_CHAIN_PARTITION) {
            const uint8_t *desc = aux + offset + 16;
            size_t desc_len = num_bytes;

            if (desc_len >= 76) {
                uint32_t rollback_loc = be32(desc);
                uint32_t pname_len    = be32(desc + 4);
                size_t pname_start = 76;

                if (pname_start + pname_len <= desc_len &&
                    pname_len < MAX_PARTITION_NAME) {
                    memcpy(chains[count].name, desc + pname_start, pname_len);
                    chains[count].name[pname_len] = '\0';
                    chains[count].rollback_index_location = rollback_loc;

                    printf("  Chain Partition: '%s' (rollback_location=%u)\n",
                           chains[count].name, rollback_loc);
                    count++;
                }
            }
        }

        size_t aligned = 16 + num_bytes;
        size_t padding = (8 - (aligned % 8)) % 8;
        offset += aligned + padding;
    }

    return count;
}

static int run_cmd(const char *cmd) {
    printf("  $ %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0)
        fprintf(stderr, "Command failed (ret=%d): %s\n", ret, cmd);
    return ret;
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

static void wait_for_device(void) {
    char cmd[MAX_CMD_LEN];
    printf("Waiting for device...\n");
    snprintf(cmd, sizeof(cmd), "%s wait-for-device", adb_path);
    run_cmd(cmd);
    printf("Device connected\n");
}

static int get_active_slot(char *slot, size_t slot_size) {
    char cmd[MAX_CMD_LEN];

    snprintf(cmd, sizeof(cmd), "%s shell getprop ro.boot.slot_suffix", adb_path);
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    char buf[64] = {0};
    if (fgets(buf, sizeof(buf), fp))
        buf[strcspn(buf, "\r\n")] = '\0';
    pclose(fp);

    if (buf[0]) {
        snprintf(slot, slot_size, "%s", buf);
        printf("Active slot: %s\n", slot);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "%s shell getprop ro.boot.slot", adb_path);
    fp = popen(cmd, "r");
    if (!fp) return -1;

    buf[0] = '\0';
    if (fgets(buf, sizeof(buf), fp))
        buf[strcspn(buf, "\r\n")] = '\0';
    pclose(fp);

    if (buf[0]) {
        snprintf(slot, slot_size, "_%s", buf);
        printf("Active slot: %s\n", slot);
        return 0;
    }

    fprintf(stderr, "Warning: cannot detect active slot, defaulting to _a\n");
    snprintf(slot, slot_size, "_a");
    return 0;
}

/* pull partition from device, read into memory, delete local img, return buffer */
static uint8_t *pull_and_read_partition(const char *partition, const char *slot,
                                        const char *output_dir, size_t *out_size) {
    char block_dev[MAX_PATH_LEN], remote_tmp[MAX_PATH_LEN];
    char local_path[MAX_PATH_LEN], cmd[MAX_CMD_LEN];

    snprintf(block_dev, sizeof(block_dev),
             "/dev/block/by-name/%s%s", partition, slot);
    snprintf(remote_tmp, sizeof(remote_tmp),
             DEVICE_TEMP_DIR "/%s%s.img", partition, slot);
    snprintf(local_path, sizeof(local_path),
             "%s/%s%s.img", output_dir, partition, slot);

    printf("\nPulling partition: %s%s\n", partition, slot);

    snprintf(cmd, sizeof(cmd),
             "%s shell su -c 'dd if=%s of=%s bs=4096'", adb_path, block_dev, remote_tmp);
    if (run_cmd(cmd) != 0) return NULL;

    snprintf(cmd, sizeof(cmd),
             "%s shell su -c 'chmod 644 %s'", adb_path, remote_tmp);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "%s pull %s %s", adb_path, remote_tmp, local_path);
    if (run_cmd(cmd) != 0) return NULL;

    snprintf(cmd, sizeof(cmd),
             "%s shell su -c 'rm -f %s'", adb_path, remote_tmp);
    run_cmd(cmd);

    uint8_t *data = read_file(local_path, out_size);
    if (!data) {
        fprintf(stderr, "Failed to read: %s\n", local_path);
        return NULL;
    }

    printf("  Pulled %s (%zu bytes)\n", local_path, *out_size);

    remove(local_path);

    return data;
}

/* extract vbmeta from partition data and save as {name}.vbmeta */
static int backup_vbmeta(const uint8_t *part_data, size_t part_size,
                         const char *name, const char *output_dir,
                         uint8_t **out_vbmeta, size_t *out_vbmeta_size) {
    size_t vb_offset, vb_size;
    if (locate_vbmeta(part_data, part_size, &vb_offset, &vb_size) != 0) {
        fprintf(stderr, "  [%s] Failed to locate vbmeta\n", name);
        return -1;
    }

    char backup_path[MAX_PATH_LEN];
    snprintf(backup_path, sizeof(backup_path), "%s/%s.vbmeta", output_dir, name);

    if (write_file(backup_path, part_data + vb_offset, vb_size) != 0) {
        fprintf(stderr, "  [%s] Failed to write: %s\n", name, backup_path);
        return -1;
    }
    printf("  [%s] Backed up vbmeta -> %s (%zu bytes)\n", name, backup_path, vb_size);

    if (out_vbmeta && out_vbmeta_size) {
        *out_vbmeta = malloc(vb_size);
        if (!*out_vbmeta) return -1;
        memcpy(*out_vbmeta, part_data + vb_offset, vb_size);
        *out_vbmeta_size = vb_size;
    }

    return 0;
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-o output_dir] [-s slot] [-a adb_path]\n", prog);
    fprintf(stderr, "  -o  vbmeta backup directory (default: ./vbmetas)\n");
    fprintf(stderr, "  -s  slot suffix, e.g. _a or _b (default: auto-detect)\n");
    fprintf(stderr, "  -a  path to adb executable (default: adb)\n");
}

int main(int argc, char **argv) {
    const char *output_dir = "./vbmetas";
    const char *slot_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-o") == 0) && i + 1 < argc) {
            output_dir = argv[++i];
        } else if ((strcmp(argv[i], "-s") == 0) && i + 1 < argc) {
            slot_arg = argv[++i];
        } else if ((strcmp(argv[i], "-a") == 0) && i + 1 < argc) {
            adb_path = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    mkdir_p(output_dir);

    wait_for_device();

    char slot[32];
    if (slot_arg) {
        if (slot_arg[0] == '_')
            snprintf(slot, sizeof(slot), "%s", slot_arg);
        else
            snprintf(slot, sizeof(slot), "_%s", slot_arg);
    } else {
        get_active_slot(slot, sizeof(slot));
    }

    static const char *roots[] = {
        "vbmeta",
/** /
        "vbmeta_system",
        "boot", "dtbo", "init_boot", "pvmfw",
        "qtvm-dtbo", "recovery", "vendor_boot",
        /**/
        NULL
    };

    for (int r = 0; roots[r]; r++) {
        /* skip this root if already backed up */
        char check_path[MAX_PATH_LEN];
        snprintf(check_path, sizeof(check_path), "%s/%s.vbmeta", output_dir, roots[r]);
        FILE *chk = fopen(check_path, "rb");
        if (chk) {
            fclose(chk);
            printf("\n[%s] Already backed up, skipping tree\n", roots[r]);
            continue;
        }

        printf("\n======== Root: %s ========\n", roots[r]);

        size_t part_size;
        uint8_t *part_data = pull_and_read_partition(roots[r], slot, output_dir, &part_size);
        if (!part_data) {
            fprintf(stderr, "  Failed to pull root: %s (may not exist, skipping)\n", roots[r]);
            continue;
        }

        uint8_t *root_vbmeta = NULL;
        size_t root_vbmeta_len = 0;
        if (backup_vbmeta(part_data, part_size, roots[r], output_dir,
                          &root_vbmeta, &root_vbmeta_len) != 0) {
            free(part_data);
            continue;
        }
        free(part_data);

        /* BFS from this root */
        struct {
            uint8_t *data;
            size_t len;
            char name[MAX_PARTITION_NAME];
        } queue[MAX_CHAINS];
        int q_head = 0, q_tail = 0;

        queue[q_tail].data = root_vbmeta;
        queue[q_tail].len = root_vbmeta_len;
        snprintf(queue[q_tail].name, MAX_PARTITION_NAME, "%s", roots[r]);
        q_tail++;

        while (q_head < q_tail) {
            uint8_t *cur_data = queue[q_head].data;
            size_t cur_len = queue[q_head].len;

            struct chain_partition chains[MAX_CHAINS];
            int chain_count = parse_chain_partitions(cur_data, cur_len,
                                                     chains, MAX_CHAINS);

            for (int i = 0; i < chain_count && q_tail < MAX_CHAINS; i++) {
                printf("\n  Found chain: %s (rollback_location=%u)\n",
                       chains[i].name, chains[i].rollback_index_location);

                size_t chain_size;
                uint8_t *chain_data = pull_and_read_partition(
                    chains[i].name, slot, output_dir, &chain_size);
                if (!chain_data) {
                    fprintf(stderr, "    Failed to pull: %s\n", chains[i].name);
                    continue;
                }

                uint8_t *chain_vbmeta = NULL;
                size_t chain_vbmeta_len = 0;
                if (backup_vbmeta(chain_data, chain_size, chains[i].name, output_dir,
                                  &chain_vbmeta, &chain_vbmeta_len) == 0 && chain_vbmeta) {
                    queue[q_tail].data = chain_vbmeta;
                    queue[q_tail].len = chain_vbmeta_len;
                    snprintf(queue[q_tail].name, MAX_PARTITION_NAME, "%s", chains[i].name);
                    q_tail++;
                }
                free(chain_data);
            }

            free(cur_data);
            q_head++;
        }
    }

    printf("\nBackup complete, files saved to: %s\n", output_dir);

    return 0;
}
