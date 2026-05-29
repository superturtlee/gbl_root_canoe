#ifndef LIBDTOVERLAY_SYSDEPS_H
#define LIBDTOVERLAY_SYSDEPS_H

#ifdef INCLUDE_PLATFORM_HDRS
/* Change these includes to match your platform to bring in the
 * equivalent types available in a normal C runtime. At least things
 * like uint8_t, uint64_t, and bool (with |false|, |true| keywords)
 * must be present.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#else
#include <libfdt_env.h>
#endif

#ifdef DTO_ENABLE_DEBUG
/* Print functions, used for diagnostics.
 *
 * These have no effect unless FDT_ENABLE_DEBUG is defined.
 */
#define dto_debug(...)                   \
  do {                                   \
    dto_print("DEBUG: %s():", __func__); \
    dto_print(__VA_ARGS__);              \
  } while (0)
#else
#define dto_debug(...)
#endif

#define dto_error(...)                   \
  do {                                   \
    dto_print("ERROR: %s():", __func__); \
    dto_print(__VA_ARGS__);              \
  } while (0)

int dto_print(const char *fmt, ...);

void dto_qsort(void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *));

void *dto_malloc(size_t size);

void dto_free(void *ptr);

char *dto_strdup(const char *s);

char *dto_strchr(const char *s, int c);

unsigned long int dto_strtoul(const char *nptr, char **endptr, int base);

size_t dto_strlen(const char *s);

void *dto_memcpy(void *dest, const void *src, size_t n);

int dto_strcmp(const char *s1, const char *s2);

int dto_strncmp(const char *s1, const char *s2, size_t n);

void *dto_memchr(const void *s, int c, size_t n);

void *dto_memset(void *s, int c, size_t n);

void* pre_overlay_malloc();

void post_overlay_free();

#endif /* LIBDTOVERLAY_SYSDEPS_H */
