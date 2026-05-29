#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <lzma.h>
#include <getopt.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_p(path) _mkdir(path)
#else
#define mkdir_p(path) mkdir(path, 0755)
#endif

#define MAX_PE_FILES    4096
#define MAX_BMP_FILES   1024
#define MAX_SCAN_DEPTH  5
#define LZMA_CHUNK      0x200000
#define MAX_BMP_SIZE    (10 * 1024 * 1024)
#define HASH_TABLE_SIZE 65536

/* ── 数据结构 ── */

typedef enum {
    MODE_DEFAULT = 0,
    MODE_PE32,
    MODE_BMP,
    MODE_ALL
} extract_mode_t;

typedef struct {
    size_t   offset;
    uint8_t *data;
    size_t   data_len;
    char     info[128];
} pe_entry_t;

typedef struct {
    size_t   offset;
    uint32_t file_size;
    uint8_t *data;
} bmp_entry_t;

typedef struct {
    pe_entry_t  pe_files[MAX_PE_FILES];
    int         pe_count;
    bmp_entry_t images[MAX_BMP_FILES];
    int         img_count;
    uint64_t    seen_hashes[HASH_TABLE_SIZE];
    int         hash_count;
    bool        verbose;
    bool        info_only;
} extractor_t;


static inline uint16_t r16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t r32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline uint64_t r64(const uint8_t *p) {
    return (uint64_t)r32(p) | ((uint64_t)r32(p + 4) << 32);
}

//manaul memmem implementation to avoid dependency on GNU extensions
static void *memmem_patcher(const void *haystack, size_t haystacklen,
                           const void *needle, size_t needlelen) {
    if (needlelen == 0) return (void *)haystack;
    if (haystacklen < needlelen) return NULL;
    const uint8_t *h = (const uint8_t *)haystack;
    const uint8_t *n = (const uint8_t *)needle;
    for (size_t i = 0; i <= haystacklen - needlelen;
            i++) {
            if (h[i] == n[0] && memcmp(h + i, n, needlelen) == 0) {
                return (void *)(h + i);
            }
        }
    return NULL;
}

static const uint8_t *fast_find(const uint8_t *haystack, size_t hlen,
                                const uint8_t *needle, size_t nlen,
                                size_t start) {
    if (start >= hlen || start + nlen > hlen) return NULL;
    return (const uint8_t *)memmem_patcher(haystack + start, hlen - start, needle, nlen);
}

static void xlog(const extractor_t *ext, int depth, const char *fmt, ...) {
    if (!ext->verbose && !ext->info_only) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("[*] ");
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

static uint64_t fnv_hash(const uint8_t *data, size_t len) {
    size_t n = len < 1000 ? len : 1000;
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < n; i++) {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static bool hash_check_and_add(extractor_t *ext, const uint8_t *data, size_t len) {
    uint64_t h = fnv_hash(data, len);
    for (int i = 0; i < ext->hash_count; i++) {
        if (ext->seen_hashes[i] == h) return true; /* 已见过 */
    }
    if (ext->hash_count < HASH_TABLE_SIZE)
        ext->seen_hashes[ext->hash_count++] = h;
    return false;
}


static size_t calc_pe_real_size(const uint8_t *data, size_t len) {
    if (len < 0x40) return len;

    uint16_t pe_ptr = r16(data + 0x3C);
    if ((size_t)pe_ptr + 0x58 > len) return len;
    if (data[pe_ptr] != 'P' || data[pe_ptr + 1] != 'E') return len;

    uint16_t num_sec  = r16(data + pe_ptr + 0x06);
    uint16_t opt_size = r16(data + pe_ptr + 0x14);
    size_t sec_table  = pe_ptr + 0x18 + opt_size;

    size_t real_len = (size_t)r32(data + pe_ptr + 0x54); /* SizeOfHeaders */

    for (int i = 0; i < num_sec; i++) {
        size_t sec_off = sec_table + (size_t)i * 0x28;
        if (sec_off + 0x28 > len) break;
        uint32_t size_raw = r32(data + sec_off + 0x10);
        uint32_t ptr_raw  = r32(data + sec_off + 0x14);
        size_t end = (size_t)ptr_raw + size_raw;
        if (end > real_len) real_len = end;
    }

    if (real_len > len) real_len = len;
    return real_len;
}

static void parse_pe_info(const uint8_t *data, size_t len, char *buf, size_t bufsz) {
    if (len < 0x60) { snprintf(buf, bufsz, "Unknown PE structure"); return; }

    uint16_t pe_ptr = r16(data + 0x3C);
    if ((size_t)pe_ptr + 0x5E > len) { snprintf(buf, bufsz, "Unknown PE structure"); return; }

    uint16_t machine   = r16(data + pe_ptr + 4);
    uint16_t subsystem = r16(data + pe_ptr + 0x5C);

    const char *m_str;
    switch (machine) {
        case 0xAA64: m_str = "ARM64"; break;
        case 0x014C: m_str = "x86";   break;
        case 0x8664: m_str = "x64";   break;
        case 0x01C0: m_str = "ARM";   break;
        default: {
            static char mbuf[16];
            snprintf(mbuf, sizeof(mbuf), "0x%X", machine);
            m_str = mbuf;
            break;
        }
    }

    const char *s_str;
    switch (subsystem) {
        case 10: s_str = "EFI_APP";     break;
        case 11: s_str = "EFI_DRIVER";  break;
        case 12: s_str = "EFI_RUNTIME"; break;
        default: {
            static char sbuf[16];
            snprintf(sbuf, sizeof(sbuf), "0x%X", subsystem);
            s_str = sbuf;
            break;
        }
    }

    snprintf(buf, bufsz, "Arch: %s, Type: %s", m_str, s_str);
}

static uint8_t *try_lzma_decompress(const uint8_t *data, size_t len, size_t *out_len) {
    *out_len = 0;

    for (int skip = 0; skip < 32 && (size_t)skip < len; skip++) {
        const uint8_t *d = data + skip;
        size_t dlen = len - skip;

        if (dlen < 5 || d[0] != 0x5D) continue;

        {
            size_t src_len = 13 + (dlen - 5);
            uint8_t *src = (uint8_t *)malloc(src_len);
            if (!src) continue;

            memcpy(src, d, 5);                /* props */
            memset(src + 5, 0xFF, 8);         /* uncompressed size = UINT64_MAX */
            memcpy(src + 13, d + 5, dlen - 5);

            size_t alloc_size = dlen * 8;
            if (alloc_size < 0x100000) alloc_size = 0x100000;
            if (alloc_size > 32 * 1024 * 1024) alloc_size = 32 * 1024 * 1024;

            uint8_t *out = (uint8_t *)malloc(alloc_size);
            if (!out) { free(src); continue; }

            lzma_stream strm = LZMA_STREAM_INIT;
            lzma_ret ret = lzma_alone_decoder(&strm, 32 * 1024 * 1024);
            if (ret != LZMA_OK) { free(src); free(out); continue; }

            strm.next_in   = src;
            strm.avail_in  = src_len;
            strm.next_out  = out;
            strm.avail_out = alloc_size;

            ret = lzma_code(&strm, LZMA_FINISH);
            size_t decoded = alloc_size - strm.avail_out;
            lzma_end(&strm);
            free(src);

            if ((ret == LZMA_STREAM_END || ret == LZMA_OK) && decoded > 64) {
                /* 缩减到实际大小 */
                out = (uint8_t *)realloc(out, decoded);
                *out_len = decoded;
                return out;
            }
            free(out);
        }

        {
            size_t alloc_size = dlen * 8;
            if (alloc_size < 0x100000) alloc_size = 0x100000;
            if (alloc_size > 32 * 1024 * 1024) alloc_size = 32 * 1024 * 1024;

            uint8_t *out = (uint8_t *)malloc(alloc_size);
            if (!out) continue;

            lzma_stream strm = LZMA_STREAM_INIT;
            lzma_ret ret = lzma_auto_decoder(&strm, 32 * 1024 * 1024, 0);
            if (ret != LZMA_OK) { free(out); continue; }

            strm.next_in   = d;
            strm.avail_in  = dlen;
            strm.next_out  = out;
            strm.avail_out = alloc_size;

            ret = lzma_code(&strm, LZMA_FINISH);
            size_t decoded = alloc_size - strm.avail_out;
            lzma_end(&strm);

            if ((ret == LZMA_STREAM_END || ret == LZMA_OK) && decoded > 64) {
                out = (uint8_t *)realloc(out, decoded);
                *out_len = decoded;
                return out;
            }
            free(out);
        }
    }
    return NULL;
}

static void deep_scan(extractor_t *ext, const uint8_t *data, size_t len, int depth) {
    if (depth > MAX_SCAN_DEPTH || !data || len < 0x40) return;
    if (hash_check_and_add(ext, data, len)) return;
    {
        size_t off = 0;
        const uint8_t sig[2] = {'M', 'Z'};
        for (;;) {
            const uint8_t *p = fast_find(data, len, sig, 2, off);
            if (!p) break;
            off = (size_t)(p - data);

            if (off + 0x40 < len) {
                uint16_t pe_ptr = r16(data + off + 0x3C);
                if (off + pe_ptr + 2 <= len &&
                    data[off + pe_ptr] == 'P' && data[off + pe_ptr + 1] == 'E') {

                    if (ext->pe_count < MAX_PE_FILES) {
                        size_t remain = len - off;
                        pe_entry_t *e = &ext->pe_files[ext->pe_count];
                        e->offset   = off;
                        e->data     = (uint8_t *)malloc(remain);
                        if (e->data) {
                            memcpy(e->data, data + off, remain);
                            e->data_len = remain;
                            parse_pe_info(data + off, remain, e->info, sizeof(e->info));
                            xlog(ext, depth, "FOUND PE: Offset 0x%zX | %s", off, e->info);
                            ext->pe_count++;
                        }
                    }
                }
            }
            off += 2; /* Python: off += 2 */
        }
    }

    /* ─── 2. 扫描 BMP ─── */
    {
        size_t off = 0;
        const uint8_t sig[2] = {'B', 'M'};
        for (;;) {
            const uint8_t *p = fast_find(data, len, sig, 2, off);
            if (!p) break;
            off = (size_t)(p - data);

            if (off + 14 < len) {
                uint32_t f_size = r32(data + off + 2);
                /* Python: if 100 < f_size < 10 * 1024 * 1024 */
                if (f_size > 100 && f_size < MAX_BMP_SIZE &&
                    off + f_size <= len) {

                    if (ext->img_count < MAX_BMP_FILES) {
                        bmp_entry_t *b = &ext->images[ext->img_count];
                        b->offset    = off;
                        b->file_size = f_size;
                        b->data      = (uint8_t *)malloc(f_size);
                        if (b->data) {
                            memcpy(b->data, data + off, f_size);
                            xlog(ext, depth, "FOUND BMP: Offset 0x%zX | Size: %u bytes",
                                 off, f_size);
                            ext->img_count++;
                        }
                    }
                }
            }
            off += 2;
        }
    }

    /* ─── 3. 扫描 LZMA 压缩块 ─── */
    {
        size_t off = 0;
        const uint8_t sig[3] = {0x5D, 0x00, 0x00};
        for (;;) {
            const uint8_t *p = fast_find(data, len, sig, 3, off);
            if (!p) break;
            off = (size_t)(p - data);

            size_t chunk = len - off;
            if (chunk > LZMA_CHUNK) chunk = LZMA_CHUNK;

            size_t decomp_len = 0;
            uint8_t *decomp = try_lzma_decompress(data + off, chunk, &decomp_len);
            if (decomp) {
                for (int i = 0; i < depth; i++) printf("  ");
                printf("[Decompressed Layer %d] Size: 0x%zX\n", depth + 1, decomp_len);
                deep_scan(ext, decomp, decomp_len, depth + 1);
                free(decomp);
            }
            off += 1;
        }
    }

    /* ─── 4. 扫描 EFI Firmware Volume ─── */
    {
        size_t off = 0;
        const uint8_t sig[4] = {'_', 'F', 'V', 'H'};
        for (;;) {
            const uint8_t *p = fast_find(data, len, sig, 4, off);
            if (!p) break;
            off = (size_t)(p - data);

            size_t fv_start = (size_t)off - 0x28;
            if (fv_start >= 0 && (size_t)fv_start + 0x28 <= len) {
                uint64_t fv_len = r64(data + fv_start + 0x20);
                /* Python: if 0x100 < fv_len < len(data) - fv_start */
                if (fv_len > 0x100 && (size_t)fv_start + fv_len <= len) {
                    xlog(ext, depth, "Entering Firmware Volume (FV) at 0x%zX", (size_t)fv_start);
                    deep_scan(ext, data + fv_start, (size_t)fv_len, depth + 1);
                }
            }
            off += 4;
        }
    }
}

/* ── 文件写入 ── */

static int write_file(const char *path, const uint8_t *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }
    if (fwrite(data, 1, len, f) != len) {
        perror(path);
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

/* ── 确保目录存在 ── */

static void ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        mkdir_p(path);
        printf("[*] Created output directory: %s\n", path);
    }
}

/* ── 用法 ── */

static void usage(const char *prog) {
    printf("Advanced QCOM ABL Investigator\n\n");
    printf("Usage: %s [options] <input>\n\n", prog);
    printf("Options:\n");
    printf("  -o <dir>     Output directory (default: extracted)\n");
    printf("  -e <mode>    Extract: pe32, bmp, all (default: largest PE)\n");
    printf("  -v           Verbose output\n");
    printf("  -i           Info mode: list contents without extracting\n");
    printf("  -h           Show this help\n");
}

/* ── main ── */

int main(int argc, char **argv) {
    const char     *input_path = NULL;
    const char     *output_dir = "extracted";
    extract_mode_t  mode       = MODE_DEFAULT;
    bool            verbose    = false;
    bool            info_only  = false;

    int opt;
    while ((opt = getopt(argc, argv, "o:e:vih")) != -1) {
        switch (opt) {
            case 'o': output_dir = optarg; break;
            case 'e':
                if      (strcmp(optarg, "pe32") == 0) mode = MODE_PE32;
                else if (strcmp(optarg, "bmp")  == 0) mode = MODE_BMP;
                else if (strcmp(optarg, "all")  == 0) mode = MODE_ALL;
                else { fprintf(stderr, "Unknown mode: %s\n", optarg); return 1; }
                break;
            case 'v': verbose   = true; break;
            case 'i': info_only = true; break;
            case 'h': usage(argv[0]); return 0;
            default:  usage(argv[0]); return 1;
        }
    }

    if (optind >= argc) { usage(argv[0]); return 1; }
    input_path = argv[optind];

    /* 读取文件 */
    FILE *fp = fopen(input_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: %s not found.\n", input_path);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t *raw_data = (uint8_t *)malloc(file_size);
    if (!raw_data) {
        fprintf(stderr, "Error: Cannot allocate %ld bytes.\n", file_size);
        fclose(fp);
        return 1;
    }
    if ((long)fread(raw_data, 1, file_size, fp) != file_size) {
        fprintf(stderr, "Error: Failed to read file.\n");
        free(raw_data);
        fclose(fp);
        return 1;
    }
    fclose(fp);

    /* 文件名 */
    const char *basename = strrchr(input_path, '/');
    if (!basename) basename = strrchr(input_path, '\\');
    basename = basename ? basename + 1 : input_path;

    printf("[*] Analyzing %s (Size: %ld bytes)\n", basename, file_size);

    /* 初始化 */
    extractor_t *ext = (extractor_t *)calloc(1, sizeof(extractor_t));
    if (!ext) { free(raw_data); return 1; }
    ext->verbose   = verbose;
    ext->info_only = info_only;

    deep_scan(ext, raw_data, (size_t)file_size, 0);

    /* Info 模式 */
    if (info_only) {
        printf("\n--- Scan Summary ---\n");
        printf("Total PE Files Found: %d\n", ext->pe_count);
        printf("Total BMP Images Found: %d\n", ext->img_count);
        goto cleanup;
    }

    ensure_dir(output_dir);

    if (ext->pe_count == 0 && ext->img_count == 0) {
        printf("\n[-] No PE or BMP files found.\n");
        goto cleanup;
    }

    /* 默认模式：提取最大PE */
    if (mode == MODE_DEFAULT) {
        if (ext->pe_count > 0) {
            int best = 0;
            for (int i = 1; i < ext->pe_count; i++) {
                if (ext->pe_files[i].data_len > ext->pe_files[best].data_len)
                    best = i;
            }

            pe_entry_t *e = &ext->pe_files[best];
            size_t real_len = calc_pe_real_size(e->data, e->data_len);

            char path[512];
            snprintf(path, sizeof(path), "%s/LinuxLoader.efi", output_dir);

            if (real_len > 0 && real_len <= e->data_len) {
                if (write_file(path, e->data, real_len) == 0)
                    printf("[+] Extracted LinuxLoader.efi to %s\n", path);
            } else {
                /* 失败时保存整个块（等价于 Python except 分支） */
                if (write_file(path, e->data, e->data_len) == 0)
                    printf("[!] Saved raw PE chunk to %s (trimming failed)\n", path);
            }
        }
    }

    /* 提取所有 PE */
    if (mode == MODE_PE32 || mode == MODE_ALL) {
        for (int i = 0; i < ext->pe_count; i++) {
            pe_entry_t *e = &ext->pe_files[i];
            size_t real_len = calc_pe_real_size(e->data, e->data_len);
            if (real_len == 0 || real_len > e->data_len) real_len = e->data_len;

            char path[512];
            snprintf(path, sizeof(path), "%s/extracted_%d.efi", output_dir, i);
            if (write_file(path, e->data, real_len) == 0)
                printf("  -> Extracted PE %d: %s\n", i, path);
        }
    }

    /* 提取所有 BMP */
    if (mode == MODE_BMP || mode == MODE_ALL) {
        for (int i = 0; i < ext->img_count; i++) {
            bmp_entry_t *b = &ext->images[i];
            char path[512];
            snprintf(path, sizeof(path), "%s/image_0x%zX.bmp", output_dir, b->offset);
            if (write_file(path, b->data, b->file_size) == 0)
                printf("  -> Extracted BMP: %s\n", path);
        }
    }

cleanup:
    for (int i = 0; i < ext->pe_count; i++) free(ext->pe_files[i].data);
    for (int i = 0; i < ext->img_count; i++) free(ext->images[i].data);
    free(ext);
    free(raw_data);
    return 0;
}