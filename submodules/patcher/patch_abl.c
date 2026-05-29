#include "patchlib.h"
/* ==================== main ==================== */
INT32 main(INT32 argc, CHAR8* argv[]) {
    if (argc != 3) {
        Print_patcher("Usage: %s <input_file> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    CHAR8* data = NULL;
    INT32 size = 0;
    if (read_file(argv[1], &data, &size) != 0) {
        Print_patcher("Failed to read file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    if (!PatchBuffer(data,size))
    {
        Print_patcher("Patching failed\n");
        free(data);
        return EXIT_FAILURE;
    }
    FILE* out = fopen(argv[2], "wb");
    if (!out) {
        Print_patcher("Failed to open output: %s\n", argv[2]);
        free(data);
        return EXIT_FAILURE;
    }
    if (fwrite(data, 1, size, out) != size) {
        Print_patcher("Failed to write output\n");
        fclose(out);
        free(data);
        return EXIT_FAILURE;
    }
    fclose(out);
    free(data);
    Print_patcher("Saved to %s\n", argv[2]);
    return EXIT_SUCCESS;
}