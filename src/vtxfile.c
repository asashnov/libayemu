#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "ayemu.h"

/* defined in lh5dec.c */
extern void lh5_decode(unsigned char *inp,unsigned char *outp,unsigned long original_size, unsigned long packed_size);

/* read 16-bit integer from file. Return 1 if success */
static int read_word16(FILE *fp, int *p)
{
  int c;
  *p = getc(fp);
  *p += ((c = getc(fp)) << 8);
  return (c == EOF)? 0 : 1;
}

/* read 32-bit integer from file. Returns 1 if success */
static int read_word32(FILE *fp, int *p)
{
  int c;
  *p = fgetc(fp);
  *p += (getc(fp) << 8);
  *p += (getc(fp) << 16);
  *p += ((c = getc(fp)) << 24);
  return (c == EOF)? 0 : 1;
}

/* read_NTstring: reads null-terminated string from file. Return strlen. */
static int read_NTstring(FILE *fp, char s[])
{
  int i, c;
  for (i = 0 ; i < AYEMU_VTX_NTSTRING_MAX && (c = getc(fp)) != EOF && c ; i++)
    s[i] = c;
  s[i] = '\0';
  return i;
}

/** Open specified .vtx file and read vtx file header
 *
 *  Open specified .vtx file and read vtx file header in struct vtx
 *  Return value: true if success, else false
 */
int ayemu_vtx_open (ayemu_vtx_t *vtx, const char *filename)
{
  char buf[2];
  vtx->regdata = NULL;
  if ((vtx->fp = fopen (filename, "rb")) == NULL) {
    fprintf(stderr, "ayemu_vtx_open: Cannot open file %s\n", filename);
    return 0;
  }
  if (fread(buf, 2, 1, vtx->fp) != 2) {
    fprintf(stderr,"ayemu_vtx_open: Can't read from %s\n", filename);
    return 0;
  }
  buf[0] = tolower(buf[0]);
  buf[1] = tolower(buf[1]);
  if (strncmp(buf, "ay", 2) == 0)
    vtx->hdr.chiptype = AYEMU_AY;
  else if (strncmp (buf, "ym", 2) == 0)
    vtx->hdr.chiptype = AYEMU_YM;
  else {
    fprintf (stderr, "File %s is _not_ VORTEX format!\nIt not begins from AY or YM.\n", filename);
    fclose (vtx->fp);
    vtx->fp = NULL;
    return 0;
  }
  /* read VTX header info in order format specified, see http:// ..... */
  vtx->hdr.stereo = fgetc (vtx->fp);
  read_word16(vtx->fp, &vtx->hdr.loop);
  read_word32(vtx->fp, &vtx->hdr.chipFreq);
  vtx->hdr.playerFreq = fgetc (vtx->fp);
  read_word16(vtx->fp, &vtx->hdr.year);
  read_word32(vtx->fp, &vtx->hdr.regdata_size);
  read_NTstring(vtx->fp, vtx->hdr.title);
  read_NTstring(vtx->fp, vtx->hdr.author);
  read_NTstring(vtx->fp, vtx->hdr.from);
  read_NTstring(vtx->fp, vtx->hdr.tracker);
  /* if error occurs on read last line return false */
  return read_NTstring (vtx->fp, vtx->hdr.comment);
}

/** Read and encode lha data from .vtx file
 *
 * Return value: pointer to unpacked data or NULL
 * Note: you must call ayemu_vtx_open() first.
 */
char *ayemu_vtx_load_data (ayemu_vtx_t *vtx)
{
  char *packed_data;
  size_t packed_size;
  size_t buf_alloc;
  int c;

  if (vtx->fp == NULL) {
    fprintf(stderr, "ayemu_vtx_load_data: tune file not open yet (do you call ayemu_vtx_open first?)\n");
    return NULL;
  }
  packed_size = 0; 
  buf_alloc = 4096;
  packed_data = (char *) malloc (buf_alloc);
  /* read packed AY register data to end of file. */
  while ((c = fgetc (vtx->fp)) != EOF) {
    if (buf_alloc < packed_size) {              
      buf_alloc *= 2;
      packed_data = (char *) realloc (packed_data, buf_alloc);
      if (packed_data == NULL) {
	fprintf (stderr, "ayemu_vtx_load_data: Packed data out of memory!\n");
	fclose (vtx->fp);
	return NULL;
      }
    }
    packed_data[packed_size++] = c;
  }  
  fclose (vtx->fp);
  vtx->fp = NULL;
  if ((vtx->regdata = (char *) malloc (vtx->hdr.regdata_size)) == NULL) {
    fprintf (stderr, "ayemu_vtx_load_data: Can allocate %d bytes for unpack register data\n", vtx->hdr.regdata_size);
    free (packed_data);
    return NULL;
  }
  lh5_decode (packed_data, vtx->regdata, vtx->hdr.regdata_size, packed_size);
  free (packed_data);
  vtx->pos = 0;
  return vtx->regdata;
}

/** Get next 14-bytes frame of AY register data.
 *
 * Return value: 1 if sucess, 0 if no enought data.
 */
int ayemu_vtx_get_next_frame (ayemu_vtx_t *vtx, char *regs)
{
  int numframes = vtx->hdr.regdata_size / 14;
  if (vtx->pos++ >= numframes)
    return 0;
  else {
    int n;
    char *p = vtx->regdata + vtx->pos;
    for(n = 0 ; n < 14 ; n++, p+=numframes)
      regs[n] = *p;
    return 1;
  }
}

/** Print formated file name. If fmt is NULL the default format %a - %t will used
 *
 * %% the % sign
 * %a author of song
 * %t song title
 * %y year
 * %f song from
 * %T Tracker
 * %s stereo type (ABC, BCA, ...)
 * %l 'looped' or 'non-looped'
 * %c chip type: 'AY' or 'YM'
 * %F chip Freq
 * %P player freq
 */
void ayemu_vtx_sprintname (ayemu_vtx_t *vtx, char *buf, int bufsize, char *fmt)
{
#define FMT_STRING s
#define FMT_NUM d
#define APPEND(type, what) \
  do {\
  int n;\
  n = sprintf(buf, "%*" #type, bufsize, what);\
  bufsize -= n;\
  buf += n;\
  } while (0)

  char *stereo_types[] = { "MONO", "ABC", "ACB", "BAC", "BCA", "CAB", "CBA" };

  if (fmt == NULL)
    fmt = "%a - %t";
  
  while (--bufsize > 0 && *fmt != '\0')
    if (*fmt != '%')
      *buf++ = *fmt++;
    else {
      char c = *++fmt;
      switch(c) {
      case 'a':
        APPEND(FMT_STRING, vtx->hdr.author);
        break;
      case 't':
        APPEND(FMT_STRING, vtx->hdr.title);
	break;
      case 'y':
	APPEND(FMT_NUM, vtx->hdr.year);
	break;
      case 'f':
	APPEND(FMT_STRING, vtx->hdr.from);
	break;
      case 'T':
	APPEND(FMT_STRING, vtx->hdr.tracker);
	break;
      case 's':
	APPEND(FMT_STRING, stereo_types[vtx->hdr.stereo]);
	break;
      case 'l':
	APPEND(FMT_STRING, (vtx->hdr.loop)? "looped" : "non-looped" );
	break;
      case 'c':
	APPEND(FMT_STRING, (vtx->hdr.chiptype == AYEMU_AY)? "AY" : "YM" );
	break;
      case 'F':
	APPEND(FMT_NUM, vtx->hdr.chipFreq);
	break;
      case 'P':
	APPEND(FMT_NUM, vtx->hdr.playerFreq);
	break;
      default:
	*buf++ = c;
      }
    }
}

/** Free all of allocaded resource for this file.
 *
 * Free the unpacket register data if is and close file.
 */
void ayemu_vtx_free (ayemu_vtx_t *vtx)
{
  if (vtx->fp) {
    fclose (vtx->fp);
    vtx->fp = NULL;
  }
  if (vtx->regdata) {
    free (vtx->regdata);
    vtx->regdata = NULL;
  }
}
