/* extractiong lh5 module
   (c) Haruhiko Okumura
   (m) Roman Scherbakov
 */

// #include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>

extern uint16_t bitbuf;

#define BITBUFSIZ (CHAR_BIT * sizeof bitbuf)

#define DICBIT    13    /* 12(-lh4-) or 13(-lh5-) */
#define DICSIZ   (1L << DICBIT)
#define MATCHBIT   8    /* bits for MAXMATCH - THRESHOLD */
#define MAXMATCH 256    /* formerly F (not more than UINT8_T_MAX + 1) */
#define THRESHOLD  3    /* choose optimal value */
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)
#define CBIT 9          /* $\lfloor \log_2 NC \rfloor + 1$ */
#define CODE_BIT  16    /* codeword length */
#define NP (DICBIT + 1)
#define NT (CODE_BIT + 3)
#define PBIT 4          /* smallest integer such that (1U << PBIT) > NP */
#define TBIT 5          /* smallest integer such that (1U << TBIT) > NT */
#if NT > NP
        #define NPT NT
#else
        #define NPT NP
#endif

static unsigned long origsize, compsize;
static unsigned char *in_buf;
static unsigned char *out_buf;

static uint16_t bitbuf;
static uint16_t  subbitbuf;
static int   bitcount;

static uint16_t left[2 * NC - 1], right[2 * NC - 1];
static uint8_t  c_len[NC], pt_len[NPT];
static uint16_t   blocksize;

static uint16_t c_table[4096], pt_table[256];

static int j;  /* remaining bytes to copy */


static void error(char *msg)
{
        fprintf(stderr, "libayemu: lh5dec.c: %s\n", msg);
        exit(EXIT_FAILURE);
}

static void fillbuf(int n)  /* Shift bitbuf n bits left, read n bits */
{
        bitbuf <<= n;
        while (n > bitcount) {
                bitbuf |= subbitbuf << (n -= bitcount);
                if (compsize != 0) {
                        compsize--;  subbitbuf = *in_buf++;
                } else subbitbuf = 0;
                bitcount = CHAR_BIT;
        }
        bitbuf |= subbitbuf >> (bitcount -= n);
}

static uint16_t getbits(int n)
{
        uint16_t x;

        x = bitbuf >> (BITBUFSIZ - n);  fillbuf(n);
        return x;
}

// make table for decoding

static void make_table(int nchar, uint8_t bitlen[], int tablebits, uint16_t table[])
{
        uint16_t count[17], weight[17], start[18], *p;
        uint16_t i, k, len, ch, jutbits, avail, nextcode, mask;

        for (i = 1; i <= 16; i++) count[i] = 0;
        for (i = 0; i < nchar; i++) count[bitlen[i]]++;

        start[1] = 0;
        for (i = 1; i <= 16; i++)
                start[i + 1] = start[i] + (count[i] << (16 - i));
        if (start[17] != (uint16_t)(1U << 16)) error("Bad table");

        jutbits = 16 - tablebits;
        for (i = 1; i <= tablebits; i++) {
                start[i] >>= jutbits;
                weight[i] = 1U << (tablebits - i);
        }
        while (i <= 16) weight[i++] = 1U << (16 - i);

        i = start[tablebits + 1] >> jutbits;
        if (i != (uint16_t)(1U << 16)) {
                k = 1U << tablebits;
                while (i != k) table[i++] = 0;
        }

        avail = nchar;
        mask = 1U << (15 - tablebits);
        for (ch = 0; ch < nchar; ch++) {
                if ((len = bitlen[ch]) == 0) continue;
                nextcode = start[len] + weight[len];
                if (len <= tablebits) {
                        for (i = start[len]; i < nextcode; i++) table[i] = ch;
                } else {
                        k = start[len];
                        p = &table[k >> jutbits];
                        i = len - tablebits;
                        while (i != 0) {
                                if (*p == 0) {
                                        right[avail] = left[avail] = 0;
                                        *p = avail++;
                                }
                                if (k & mask) p = &right[*p];
                                else          p = &left[*p];
                                k <<= 1;  i--;
                        }
                        *p = ch;
                }
                start[len] = nextcode;
        }
}

// static Huffman

static void read_pt_len(int nn, int nbit, int i_special)
{
        int i, c, n;
        uint16_t mask;

        n = getbits(nbit);
        if (n == 0) {
                c = getbits(nbit);
                for (i = 0; i < nn; i++) pt_len[i] = 0;
                for (i = 0; i < 256; i++) pt_table[i] = c;
        } else {
                i = 0;
                while (i < n) {
                        c = bitbuf >> (BITBUFSIZ - 3);
                        if (c == 7) {
                                mask = 1U << (BITBUFSIZ - 1 - 3);
                                while (mask & bitbuf) {  mask >>= 1;  c++;  }
                        }
                        fillbuf((c < 7) ? 3 : c - 3);
                        pt_len[i++] = c;
                        if (i == i_special) {
                                c = getbits(2);
                                while (--c >= 0) pt_len[i++] = 0;
                        }
                }
                while (i < nn) pt_len[i++] = 0;
                make_table(nn, pt_len, 8, pt_table);
        }
}

static void read_c_len(void)
{
        int i, c, n;
        uint16_t mask;

        n = getbits(CBIT);
        if (n == 0) {
                c = getbits(CBIT);
                for (i = 0; i < NC; i++) c_len[i] = 0;
                for (i = 0; i < 4096; i++) c_table[i] = c;
        } else {
                i = 0;
                while (i < n) {
                        c = pt_table[bitbuf >> (BITBUFSIZ - 8)];
                        if (c >= NT) {
                                mask = 1U << (BITBUFSIZ - 1 - 8);
                                do {
                                        if (bitbuf & mask) c = right[c];
                                        else               c = left [c];
                                        mask >>= 1;
                                } while (c >= NT);
                        }
                        fillbuf(pt_len[c]);
                        if (c <= 2) {
                                if      (c == 0) c = 1;
                                else if (c == 1) c = getbits(4) + 3;
                                else             c = getbits(CBIT) + 20;
                                while (--c >= 0) c_len[i++] = 0;
                        } else c_len[i++] = c - 2;
                }
                while (i < NC) c_len[i++] = 0;
                make_table(NC, c_len, 12, c_table);
        }
}


static uint16_t decode_c(void)
{
        uint16_t j, mask;

        if (blocksize == 0) {
                blocksize = getbits(16);
                read_pt_len(NT, TBIT, 3);
                read_c_len();
                read_pt_len(NP, PBIT, -1);
        }
        blocksize--;
        j = c_table[bitbuf >> (BITBUFSIZ - 12)];
        if (j >= NC) {
                mask = 1U << (BITBUFSIZ - 1 - 12);
                do {
                        if (bitbuf & mask) j = right[j];
                        else               j = left [j];
                        mask >>= 1;
                } while (j >= NC);
        }
        fillbuf(c_len[j]);
        return j;
}


static uint16_t decode_p(void)
{
        uint16_t j, mask;

        j = pt_table[bitbuf >> (BITBUFSIZ - 8)];
        if (j >= NP) {
                mask = 1U << (BITBUFSIZ - 1 - 8);
                do {
                        if (bitbuf & mask) j = right[j];
                        else               j = left [j];
                        mask >>= 1;
                } while (j >= NP);
        }
        fillbuf(pt_len[j]);
        if (j != 0) j = (1U << (j - 1)) + getbits(j - 1);
        return j;
}


static void decode(uint16_t count, uint8_t buffer[])
{
        static uint16_t i;
        uint16_t r, c;

        r = 0;
        while (--j >= 0) {
                buffer[r] = buffer[i];
                i = (i + 1) & (DICSIZ - 1);
                if (++r == count) return;
        }
        for ( ; ; ) {
                c = decode_c();
                if (c <= UCHAR_MAX) {
                        buffer[r] = c & UCHAR_MAX;
                        if (++r == count) return;
                } else {
                        j = c - (UCHAR_MAX + 1 - THRESHOLD);
                        i = (r - decode_p() - 1) & (DICSIZ - 1);
                        while (--j >= 0) {
                                buffer[r] = buffer[i];
                                i = (i + 1) & (DICSIZ - 1);
                                if (++r == count) return;
                        }
                }
        }
}

void lh5_decode(unsigned char *inp, unsigned char *outp, unsigned long original_size, unsigned long packed_size)
{
        unsigned short n;
        unsigned char *buffer;

        compsize = packed_size;
        origsize = original_size;
        in_buf = inp;
        out_buf = outp;

        buffer = (unsigned char *) malloc(DICSIZ);
        if (!buffer) error ("Out of memory");

        bitbuf = 0;  subbitbuf = 0;  bitcount = 0;
        fillbuf(BITBUFSIZ);
        blocksize = 0;
        j = 0;

        while (origsize != 0) {
                n = (origsize > DICSIZ) ? DICSIZ : (unsigned short)origsize;
                decode(n, buffer);
                memcpy(out_buf, buffer, n);
                out_buf += n;
                origsize -= n;
        }

        if (buffer) free (buffer);
        buffer = NULL;
}
