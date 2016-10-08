/*  Extract lh5 data */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define	FALSE			0
#define TRUE			1
typedef int			boolean;


#if defined(__STDC__) || defined(AIX)

#include <limits.h>

#else

#ifndef CHAR_BIT
#define CHAR_BIT  8
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX				((1<<(sizeof(unsigned char)*8))-1)
#endif

#ifndef USHRT_MAX
#define USHRT_MAX				((1<<(sizeof(unsigned short)*8))-1)
#endif

#ifndef SHRT_MAX
#define SHRT_MAX				((1<<(sizeof(short)*8-1))-1)
#endif

#ifndef SHRT_MIN
#define SHRT_MIN				(SHRT_MAX-USHRT_MAX)
#endif

#ifndef ULONG_MAX
#define ULONG_MAX	((1<<(sizeof(unsigned long)*8))-1)
#endif

#ifndef LONG_MAX
#define LONG_MAX	((1<<(sizeof(long)*8-1))-1)
#endif

#ifndef LONG_MIN
#define LONG_MIN	(LONG_MAX-ULONG_MAX)
#endif

#endif	/* not __STDC__ */


/* from crcio.c */
//#define CRCPOLY			0xA001		/* CRC-16 */
#define UPDATE_CRC(c)	crc = crctable[(crc ^ (c)) & 0xFF] ^ (crc >> CHAR_BIT)

/* huf.c */
#define NP			(MAX_DICBIT + 1)
#define NT			(USHRT_BIT + 3)


//#define PBIT		4		/* smallest integer such that (1 << PBIT) > * NP */
//#define TBIT 		5		/* smallest integer such that (1 << TBIT) > * NT */
//#define PBIT		5		/* smallest integer such that (1 << PBIT) > * NP */
#define TBIT 		5		/* smallest integer such that (1 << TBIT) > * NT */


#define NC 			(UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)

/*		#if NT > NP #define NPT NT #else #define NPT NP #endif	*/
#define NPT			0x80

#define MAX_DICBIT			15      /* lh6 use 15bits */

#define MAXMATCH			256	/* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD			3	/* choose optimal value */

/* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT				9	/* $\lfloor \log_2 NC \rfloor + 1$ */
#define USHRT_BIT			16	/* (CHAR_BIT * sizeof(ushort)) */


unsigned short  left[2 * NC - 1], right[2 * NC - 1];
unsigned char   c_len[NC], pt_len[NPT];

unsigned short  c_freq[2 * NC - 1], c_table[4096], c_code[NC], p_freq[2 * NP - 1],
                pt_table[256], pt_code[NPT], t_freq[2 * NT - 1];
static unsigned short crctable[UCHAR_MAX + 1];
static unsigned char subbitbuf, bitcount;

static unsigned short blocksize;

// ALEX можно удалить, используется только раз
static 			int	  pbit;
static 			int	  np;

unsigned short crc, bitbuf;


// ALEX можно удалить
static unsigned long dicsiz;  /* t.okamoto */



unsigned long count;
unsigned long loc;
unsigned long compsize;
unsigned long origsize;

/* оригинальные функции файлового ввода-вывода
   заменены работой с пямятью	 */
unsigned char *in_buf;
unsigned char *out_buf;

void fwrite_txt(unsigned char  *p, int n)
{
	while (--n >= 0)
		if (*p != '\015' && *p != '\032') 
			*out_buf++ = *p;
}

/* ------------------------------------------------------------------------ */
unsigned short calccrc(unsigned char  *p,unsigned int n)
{
	while (n-- > 0)
		UPDATE_CRC(*p++);
	return crc;
}


void fwrite_crc(unsigned char  *p, int n)
{
	calccrc(p, n);

	memcpy(out_buf,p,n);
	out_buf +=n;
}


/* ------------------------------------------------------------------------ */
void fillbuf(unsigned char n)			/* Shift bitbuf n bits left, read n bits */
{
	while (n > bitcount) {
		n -= bitcount;
		bitbuf = (bitbuf << bitcount) + (subbitbuf >> (CHAR_BIT - bitcount));
		if (compsize != 0) {
			compsize--;
			subbitbuf = *in_buf++;
		}
		else
			subbitbuf = 0;
		bitcount = CHAR_BIT;
	}
	bitcount -= n;
	bitbuf = (bitbuf << n) + (subbitbuf >> (CHAR_BIT - n));
	subbitbuf <<= n;
}

void init_getbits(void)
{
	bitbuf = 0;
	subbitbuf = 0;
	bitcount = 0;
	fillbuf(2 * CHAR_BIT);
}



/* ------------------------------------------------------------------------ */
unsigned short getbits(unsigned char n)
{
	unsigned short  x;

	x = bitbuf >> (2 * CHAR_BIT - n);
	fillbuf(n);
	return x;
}


void make_table(short nchar,unsigned char bitlen[],short tablebits,unsigned short table[])
{
	unsigned short  count[17];	/* count of bitlen */
	unsigned short  weight[17];	/* 0x10000ul >> bitlen */
	unsigned short  start[17];	/* first code of bitlen */
	unsigned short  total;
	unsigned int    i, l;
	int             j, k, m, n, avail;
	unsigned short *p;

	avail = nchar;

	/* initialize */
	for (i = 1; i <= 16; i++) {
		count[i] = 0;
		weight[i] = 1 << (16 - i);
	}

	/* count */
	for (i = 0; i < nchar; i++)
		count[bitlen[i]]++;

	/* calculate first code */
	total = 0;
	for (i = 1; i <= 16; i++) {
		start[i] = total;
		total += weight[i] * count[i];
	}
	if ((total & 0xffff) != 0)
		fprintf(stderr,"make_table(), Bad table (5)\n");

	/* shift data for make table. */
	m = 16 - tablebits;
	for (i = 1; i <= tablebits; i++) {
		start[i] >>= m;
		weight[i] >>= m;
	}

	/* initialize */
	j = start[tablebits + 1] >> m;
	k = 1 << tablebits;
	if (j != 0)
		for (i = j; i < k; i++)
			table[i] = 0;

	/* create table and tree */
	for (j = 0; j < nchar; j++) {
		k = bitlen[j];
		if (k == 0)
			continue;
		l = start[k] + weight[k];
		if (k <= tablebits) {
			/* code in table */
			for (i = start[k]; i < l; i++)
				table[i] = j;
		}
		else {
			/* code not in table */
			p = &table[(i = start[k]) >> m];
			i <<= tablebits;
			n = k - tablebits;
			/* make tree (n length) */
			while (--n >= 0) {
				if (*p == 0) {
					right[avail] = left[avail] = 0;
					*p = avail++;
				}
				if (i & 0x8000)
					p = &right[*p];
				else
					p = &left[*p];
				i <<= 1;
			}
			*p = j;
		}
		start[k] = l;
	}
}


/* ------------------------------------------------------------------------ */
static void read_pt_len(short nn,short nbit,short i_special)
{
	int           i, c, n;

	n = getbits(nbit);
	if (n == 0) {
		c = getbits(nbit);
		for (i = 0; i < nn; i++)
			pt_len[i] = 0;
		for (i = 0; i < 256; i++)
			pt_table[i] = c;
	}
	else {
		i = 0;
		while (i < n) {
			c = bitbuf >> (16 - 3);
			if (c == 7) {
				unsigned short  mask = 1 << (16 - 4);
				while (mask & bitbuf) {
					mask >>= 1;
					c++;
				}
			}
			fillbuf((c < 7) ? 3 : c - 3);
			pt_len[i++] = c;
			if (i == i_special) {
				c = getbits(2);
				while (--c >= 0)
					pt_len[i++] = 0;
			}
		}
		while (i < nn)
			pt_len[i++] = 0;
		make_table(nn, pt_len, 8, pt_table);
	}
}

/* ------------------------------------------------------------------------ */
static void read_c_len(void)
{
	short           i, c, n;

	n = getbits(CBIT);
	if (n == 0) {
		c = getbits(CBIT);
		for (i = 0; i < NC; i++)
			c_len[i] = 0;
		for (i = 0; i < 4096; i++)
			c_table[i] = c;
	} else {
		i = 0;
		while (i < n) {
			c = pt_table[bitbuf >> (16 - 8)];
			if (c >= NT) {
				unsigned short  mask = 1 << (16 - 9);
				do {
					if (bitbuf & mask)
						c = right[c];
					else
						c = left[c];
					mask >>= 1;
				} while (c >= NT);
			}
			fillbuf(pt_len[c]);
			if (c <= 2) {
				if (c == 0)
					c = 1;
				else if (c == 1)
					c = getbits(4) + 3;
				else
					c = getbits(CBIT) + 20;
				while (--c >= 0)
					c_len[i++] = 0;
			}
			else
				c_len[i++] = c - 2;
		}
		while (i < NC)
			c_len[i++] = 0;
		make_table(NC, c_len, 12, c_table);
	}
}


/* ------------------------------------------------------------------------ */
unsigned short decode_c_st1(void)
{
	unsigned short  j, mask;

	if (blocksize == 0) {
		blocksize = getbits(16);
		read_pt_len(NT, TBIT, 3);
		read_c_len();
		read_pt_len(np, pbit, -1);
	}
	blocksize--;
	j = c_table[bitbuf >> 4];
	if (j < NC)
		fillbuf(c_len[j]);
	else {
		fillbuf(12);
		mask = 1 << (16 - 1);
		do {
			if (bitbuf & mask)
				j = right[j];
			else
				j = left[j];
			mask >>= 1;
		} while (j >= NC);
		fillbuf(c_len[j] - 12);
	}
	return j;
}

/* ------------------------------------------------------------------------ */
unsigned short decode_p_st1(void)
{
	unsigned short  j, mask;

	j = pt_table[bitbuf >> (16 - 8)];
	if (j < np)
		fillbuf(pt_len[j]);
	else {
		fillbuf(8);
		mask = 1 << (16 - 1);
		do {
			if (bitbuf & mask)
				j = right[j];
			else
				j = left[j];
			mask >>= 1;
		} while (j >= np);
		fillbuf(pt_len[j] - 8);
	}
	if (j != 0)
		j = (1 << (j - 1)) + getbits(j - 1);
	return j;
}

/* ------------------------------------------------------------------------ */
void decode_start_st1(void)
{
	np = 14;
	pbit = 4;
	
	init_getbits();
	blocksize = 0;
}




void lh5_decode(unsigned char *inp, unsigned char *outp, long original_size, long packed_size)
{
	unsigned int i, j, k, c;
	unsigned int dicsiz1, offset;
	unsigned char *dtext;
	
	compsize = packed_size;
	origsize = original_size;
	in_buf = inp;
	out_buf = outp;

	crc = 0;
	dicsiz = 1L << 13 /* dicbit*/;
	dtext = (unsigned char *) malloc(dicsiz);
	if (dtext == NULL)
	{
		fprintf(stderr, "No mem\n");
		goto exit;
	}
	for (i=0; i<dicsiz; i++) dtext[i] = 0x20;
	decode_start_st1();
	dicsiz1 = dicsiz - 1;
	offset = 0x100 - 3;
	count = 0;
	loc = 0;
	while (count < origsize) {
		c = decode_c_st1();
		if (c <= UCHAR_MAX) {
			dtext[loc++] = c;
			if (loc == dicsiz) {
				fwrite_crc(dtext, dicsiz);
				loc = 0;
			}
			count++;
		}
		else {
			j = c - offset;
			i = (loc - decode_p_st1() - 1) & dicsiz1;
			count += j;
			for (k = 0; k < j; k++) {
				c = dtext[(i + k) & dicsiz1];

				dtext[loc++] = c;
				if (loc == dicsiz) {
					fwrite_crc(dtext, dicsiz);
					loc = 0;
				}
			}
		}
	}
	if (loc != 0) {
		fwrite_crc(dtext, loc);
	}

exit:
	free(dtext);
}
