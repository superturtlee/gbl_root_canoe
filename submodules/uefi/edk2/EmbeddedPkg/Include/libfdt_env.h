/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi.h>

typedef UINT16 fdt16_t;
typedef UINT32 fdt32_t;
typedef UINT64 fdt64_t;

typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;
typedef UINTN uintptr_t;
typedef UINTN size_t;
typedef BOOLEAN bool;

static inline uint16_t fdt16_to_cpu(fdt16_t x)
{
  return SwapBytes16 (x);
}
#define cpu_to_fdt16(x) fdt16_to_cpu(x)

static inline uint32_t fdt32_to_cpu(fdt32_t x)
{
  return SwapBytes32 (x);
}
#define cpu_to_fdt32(x) fdt32_to_cpu(x)

static inline uint64_t fdt64_to_cpu(fdt64_t x)
{
  return SwapBytes64 (x);
}
#define cpu_to_fdt64(x) fdt64_to_cpu(x)

static inline void* memcpy(void* dest, const void* src, size_t len) {
  return CopyMem (dest, src, len);
}

static inline void *memmove(void *dest, const void *src, size_t n) {
  return CopyMem (dest, src, n);
}

static inline __attribute__ ((always_inline))
void *memset (void *s, int c, size_t n)
{
  return SetMem (s, n, c);
}

static inline int memcmp(const void* dest, const void* src, int len) {
  return CompareMem (dest, src, len);
}

static inline void *memchr(const void *s, int c, size_t n) {
  return ScanMem8 (s, n, c);
}

static inline size_t strlen (const char* str) {
  return AsciiStrLen (str);
}

static inline char *strchr(const char *s, int c) {
  char pattern[2];
  pattern[0] = c;
  pattern[1] = 0;
  return AsciiStrStr (s, pattern);
}

static inline int strcmp(const char *s1, const char *s2)
{
  return (int)AsciiStrCmp( s1, s2);
}

static inline int strncmp(const char *s1, const char *s2, size_t n)
{
  return (int)AsciiStrnCmp( s1, s2, n);
}

/**
  Simple character classification routines, corresponding to POSIX class names
  and ASCII encoding.
**/
#define toupper(a)  ((((a) >= 'a') && ((a) <= 'z')) ? ((a) - 'a' + 'A') : (a))

#define isalpha(chr) (('a' <= chr && chr <= 'z') || ('A' <= chr && chr <= 'Z'))
STATIC
BOOLEAN
isalnum (
  IN  CHAR8 Chr
  )
{
  return (('0' <= Chr && Chr <= '9') ||
          ('A' <= Chr && Chr <= 'Z') ||
          ('a' <= Chr && Chr <= 'z')
          );
}

static int
Digit2Val( int c)
{
  if(isalpha(c)) {  /* If c is one of [A-Za-z]... */
    c = toupper(c) - 7;   // Adjust so 'A' is ('9' + 1)
  }
  return c - '0';   // Value returned is between 0 and 35, inclusive.
}

static inline size_t strnlen (const char* str, size_t strsz ) {
  return AsciiStrnLenS (str, strsz);
}

/* Determines if a particular character represents a space character */
static inline int isspace (int c)
{
  //
  // <space> ::= [ ]
  //
  return ((c) == ' ');
}

#define UINT32_MAX      0xffffffffU                     /* uint32_t       */
/** The strtoul function converts the initial portion of the string pointed to
    by nptr to unsigned long int representation.

    See the description for strtol for more information.

  @return   The strtoul function returns the converted value, if any. If no
            conversion could be performed, zero is returned. If the correct
            value is outside the range of representable values, ULONG_MAX is
            returned and the value of the macro ERANGE is stored in errno.
**/

static inline unsigned long
strtoul(const char * __restrict nptr, char ** __restrict endptr, int base)
{
  const char     *pEnd;
  unsigned long   Result = 0;
  unsigned long   Previous;
  int             temp;

  pEnd = nptr;

  if((base < 0) || (base == 1) || (base > 36)) {
    if(endptr != NULL) {
    *endptr = NULL;
    }
    return 0;
  }
  // Skip leading spaces.
  while(isspace(*nptr))   ++nptr;

  // Process Subject sequence: optional + sign followed by digits.
  if(*nptr == '+') {
    ++nptr;
  }

  if(*nptr == '0') {  /* Might be Octal or Hex */
    if(toupper(nptr[1]) == 'X') {   /* Looks like Hex */
      if((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base = 16;  /* In case base was 0 */
      }
    }
    else {    /* Looks like Octal */
      if((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }
  if(base == 0) {   /* If still zero then must be decimal */
    base = 10;
  }
  if(*nptr  == '0') {
    for( ; *nptr == '0'; ++nptr);  /* Skip any remaining leading zeros */
    pEnd = nptr;
  }

  while( isalnum(*nptr) && ((temp = Digit2Val(*nptr)) < base)) {
    Previous = Result;
    Result = (Result * base) + (unsigned long)temp;
    if( Result < Previous)  {   // If we overflowed
      Result = UINT32_MAX;
      //errno = -1;
      break;
    }
    pEnd = ++nptr;
  }

  // Save pointer to final sequence
  if(endptr != NULL) {
    *endptr = (char *)pEnd;
  }
  return Result;
}
#endif /* _LIBFDT_ENV_H */
