/**
 * @file string.c
 *
 * string.c provides basic handling functions (copy, compare, length...) at
 * the kernel level.
 *
 * string.c also provides several basic memory handling functions (memset,
 * (memcpy, memcmp) required for GCC to build a bare-metal binary.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation and styling.)
 *
 * @date 13/10/2018
 */


#include "types.h"


/**
 * Sets words in a range of memory to a specified value.
 *
 * memsetw ("Memory Set Words") assigns each of the 'n' words
 * from 'dst' to 'dst' + 'n' to the value 'c'.
 *
 * @param dst - A pointer to the block of memory to edit.
 * @param c - The value to assign to each word.
 * @param n - The number of words to edit in the block.
 * @return - A pointer to the block of words edited.
 */
void* memsetw(int *dst, int c, uint n)
{
    int *p=dst;
    uint rc=n;
    while (rc-- > 0) *p++ = c;
    return (void *)p;
}


/**
 * Sets bytes in a range of memory to a specified value.
 *
 * memsetb ("Memory Set Bytes") assigns each of the 'n' bytes
 * from 'dst' to 'dst' + 'n' to the value 'c'.
 *
 * @param dst - A pointer to the block of memory to edit.
 * @param c - The value to assign to each byte.
 * @param n - The number of bytes to edit in the block.
 * @return - A pointer to the block of bytes edited.
 */
void* memsetb(char *dst, int c, uint n)
{
    char *p=dst;
    uint rc=n;
    while (rc-- > 0) *p++ = c;
    return (void *)p;
}


/**
 * Sets bytes in a block of memory to a specified value.
 *
 * metset ("Memory Set value") assigns each byte of the 'n'
 * bytes from 'dst' to 'dst' + 'n' to the value 'c'.
 *
 * The value to assign, 'c' is passed in as an int, but is
 * assigned as if cast to a unsigned char.
 *
 * If the memory is word aligned, the value is copied by word
 * for efficiency.
 *
 * @param dst - A pointer to the block of memory to edit.
 * @param c - The value to assign to each byte.
 * @param n - The number of bytes to edit in the block.
 * @return - A pointer to the block of memory edited.
 */
void* memset(void* dst, int c, uint n)
{
    if ((int)dst % 4 == 0 && n % 4 == 0) {
        c &= 0xFF;
        return memsetw((int *)dst, (c<<24) | (c<<16) | (c<<8) | c, n / 4);
    } else {
        return memsetb((char *)dst, c, n);
    }
}


/**
 * Compares two blocks of memory.
 *
 * memcmp ("Memory Compare") compares two blocks of memory, by byte.
 * If two blocks are not identical, memcmp indicates which block has
 * a lower value in the fist diffrent byte.
 *
 * @param v1 - The first block of memory to compare.
 * @param v2 - The second block of memory to compare.
 * @param n - The length (in bytes) of 'v1' and 'v2'.
 * @return - < 0 if v1 < v2, > 0 if v2 > v1; zero otherwise.
 */
int memcmp(const void *v1, const void *v2, uint n)
{
    const u8 *s1, *s2;
    s1 = v1;
    s2 = v2;
    while (n-- > 0) {
        if (*s1 != *s2)
        return *s1 - *s2;
        s1++, s2++;
    }
    return 0;
}


/**
 * Duplicates a block of memory, byte-wise.
 *
 * memmove ("Memory Move") copies bytes of memory from one block
 * into another block, creating a duplicate of the original block.
 *
 * @param dst - The destination pointer to copy memory into.
 * @param src - The source pointer to copy memory from.
 * @param n - The number of bytes to copy.
 * @return - A pointer to the newly copied memory.
 */
void* memmove(void *dst, const void *src, uint n) {
    const char *s;
    char *d;
    s = src;
    d = dst;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0) {
            *--d = *--s;
        }
    } else {
        while (n-- > 0) {
            *d++ = *s++;
        }
    }
  return dst;
}


/**
 * Copies a block of memory between two locations.
 *
 * memcpy ("Memory Copy") is required for GCC to compile
 * free-standing/baremetal binaries. Use 'memmove()' for
 * when working with this code.
 *
 * @param dst - A pointer to the destination of the copy.
 * @param src - A pointer to the source to copy from.
 * @param n - The number of bytes to copy,
 * @return - A pointer to the newly copied memory.
 */
void* memcpy(void *dst, const void *src, uint n)
{
    return memmove(dst, src, n);
}


/**
 * Compare two strings.
 *
 * strncmp ("String N Compare") compares the first 'n' characters
 * in a string.
 *
 * strncmp checks 'n' characters, or until a null terminator is
 * found - Whichever is first.
 *
 * @param p - A pointer to a string to compare.
 * @param q - A pointer to the other string to compare.
 * @param n - The number of characters to compare in the strings.
 * @return -  < 0 if p < q, > 0 if p > q, zero otherwise.
 */
int strncmp(const char *p, const char *q, uint n)
{
    while (n > 0 && *p && *p == *q) {
        n--, p++, q++;
    }
    if (n == 0) {
        return 0;
    }
    return (u8)*p - (u8)*q;
}


/**
 * Copies two strings.
 *
 * strncpy ("String N Copy") copies the first 'n' characters of a string,
 * or until a null terminator is found - Whichever is first.
 *
 * If the 'n' character limit is reached, no null terminator will be
 * appended.
 *
 * @param s - The destination to copy into.
 * @param t - The string to copy from,
 * @param n - The number of characters to copy.
 * @return - A pointer to the newly copied string.
 */
char* strncpy(char *s, const char *t, int n)
{
    char *os;
    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0) {
        ;
    }
    while (n-- > 0) {
          *s++ = 0;
    }
    return os;
}


/**
 * Copies two strings, guaranteeing the new string is null terminated.
 *
 * safestrcopy ("Safe String Copy") copies the first 'n' characters of a
 * string, or until a null terminator is found - Whichever is first.
 *
 * If the 'n' character limit is reached, the 'n'th character in the new
 * string is replaced with a null terminator.
 *
 * @param s - A pointer to the memory to copy the string into.
 * @param t - A pointer to the string to copy.
 * @param n - The maximum number of characters to copy.
 * @return - A pointer to the newly copied string.
 */
char* safestrcpy(char *s, const char *t, int n)
{
    char *os;
    os = s;
    if (n <= 0) {
         return os;
    }
    while (--n > 0 && (*s++ = *t++) != 0) {
        ;
    }
    *s = 0;
    return os;
}

/**
 * Computes the length of a string.
 *
 * strlen ("String Length") iterates over the characters of a string to
 * compute the length of the string.
 *
 * @param s - A pointer to the string to compute the length of.
 * @return - The number of characters in the string.
 */
int strlen(const char *s)
{
  int n;
  for (n = 0; s[n]; n++) {
      ;
  }
  return n;
}


/**
 * Computes integer division.
 *
 * div ("Division") computes the integer long division of 'n' / 'd'.
 *
 * Division is a surprisingly complex algorithm, so isn't provided
 * in the ARMv7 instruction set.
 *
 * @param n - The numerator to divide.
 * @param d  - The denominator to divide by.
 * @return - The integer divisor of 'n' and 'd'.
 */
uint div(uint n, uint d)
{
    uint q=0, r=0;
    int i;
    for (i=31; i>=0; i--){
        r = r << 1;
        r = r | ((n >> i) & 1);
        if (r >= d) {
            r = r - d;
            q = q | (1 << i);
        }
    }
    return q;
}
