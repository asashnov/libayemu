#include "ayemu.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static const int DEBUG = 0;

/* defined in lh5dec.c */
void lh5_decode(unsigned char *inp,unsigned char *outp,unsigned long original_size, unsigned long packed_size);


/* read 16 bit integer from file */
static int read_word (FILE *fp)
{
  int c, ret;

  ret = fgetc(fp);
  assert (c != (EOF));

  c = fgetc(fp);
  assert (c != (EOF));
  ret += c << 8;

  return ret;
}

/* read 32 bit integer from file */
static int read_qword (FILE *fp)
{
  int c, ret;

  ret = fgetc(fp);
  assert (c != (EOF));

  c = fgetc(fp);
  assert (c != (EOF));
  ret += c << 8;

  c = fgetc(fp);
  assert (c != (EOF));
  ret += c << 16;

  c = fgetc(fp);
  assert (c != (EOF));
  ret += c << 24;

  return ret;
}


/* Read '\0' - terminated string from file.
   Max string len with terminator - 255 chars */
static int read_string (FILE * fp, char *str)
{
  int c, n, retval;

  retval = 1;

  for (n=0 ; n < (AYEMU_VTX_NTSTRING_MAX-1); n++) {
    c = fgetc (fp);
    assert (c != (EOF));
    if (c == 0)
      break;
    str[n] = (char) c;
  }
  
  str[n] = '\0';
  return retval;
}

/* Open specified .vtx file and read vtx file header in struct vf
   Return value: true if success, else false */
int ayemu_vtx_open (ayemu_vtx_t *vtx, const char *filename)
{
  char buf[2];
  vtx->regdata = NULL;
  if ((vtx->fp = fopen (filename, "rb")) == NULL)
    {
      fprintf (stderr, "Cannot open file %s\n", filename);
      return 0;
    }

    // в начале файла должно быть AY или YM (см. формат VTX)
    fread (buf, 2, 1, vtx->fp);
  if (strncmp (buf, "ay", 2) == 0 || strncmp (buf, "AY", 2))
    vtx->Chip = "ay";

  else if (strncmp (buf, "ym", 2) == 0 || strncmp (buf, "YM", 2))
    vtx->Chip = "ym";

  else
    {
      fprintf (stderr, "File %s is _not_ VORTEX format!\nIt not begins from AY or YM.\n", filename);
      fclose (vtx->fp);
      vtx->fp = NULL;
      return 0;
    }
  
    vtx->Stereo = fgetc (vtx->fp);
    vtx->Loop = read_word (vtx->fp);
    vtx->ChipFreq = read_qword (vtx->fp);
    vtx->PlayerFreq = fgetc (vtx->fp);
    vtx->Year = read_word (vtx->fp);
    vtx->regdata_size = read_qword (vtx->fp);

    read_string (vtx->fp, vtx->Title);
    read_string (vtx->fp, vtx->Author);
    read_string (vtx->fp, vtx->From);
    read_string (vtx->fp, vtx->Tracker);
    read_string (vtx->fp, vtx->Comment);
    return 1;
}

/* Read and encode lha data from .vtx file.
   Note: you must call ayemu_vtx_open first.
   Return value: pointer to AY regs frames if sucess, else NULL.
   Error description prints to stdout */
void * ayemu_vtx_load_data (ayemu_vtx_t *vtx)
{
  uint8_t * packed_data;
  size_t packed_size, buf_alloc;
  int c;

  if (vtx->fp == NULL)		/* exit if file not open */
    return 0;

  /* calculate packet data size */
  packed_size = 0; 
  buf_alloc = 1;			/* TODO: change value to 4096 */
  packed_data = (uint8_t *) malloc (buf_alloc);
  
  /* grove up buffer during reading data from file while not end of file */
  while (1)
    {
      c = fgetc (vtx->fp);	/* read next byte */
      if (c == (EOF))
	break;

      /* increase buffer on demand */
      if (buf_alloc < packed_size)
	{
	  buf_alloc *= 2;
	  packed_data = (uint8_t *) realloc (packed_data, buf_alloc);
	  if (packed_data == NULL)
	    {
	      fprintf (stderr, "VTX_ReadData: Packed data out ofmemory!\n");
	      fclose (vtx->fp);
	      return NULL;
	    }
	}

      /* write byte to buffer */
      packed_data[packed_size++] = c;
    };

  // закрытие файла
  fclose (vtx->fp);
  vtx->fp = NULL;

  // выделяем память под распакованные данные
  if ((vtx->regdata = (uint8_t *) malloc (vtx->regdata_size)) == NULL)
    {
      fprintf (stderr, "VTX_ReadData: No memory for regdata!\n");
      free (packed_data);
      return NULL;
    }
  lh5_decode (packed_data, vtx->regdata, vtx->regdata_size, packed_size);
  free (packed_data);

  // указатель на начало данных
  vtx->pos = 0;		// текущее смещение в области данных регистров
  return vtx->regdata;
}


/* Get next 14-bytes frame of AY register data.
     Return value: true if sucess, false if not enought data. */
int ayemu_vtx_get_next_frame (ayemu_vtx_t *vtx, void *regs)
{
  int n,reglen;
  uint8_t *p, *reg;
      
  reg = (uint8_t *) regs;
  reglen = vtx->regdata_size/14;

  if (vtx->pos >= reglen)
    return 0;

  p = vtx->regdata + vtx->pos;
  for(n=0; n<14; n++, p += reglen)
    reg[n] = *p;
  vtx->pos++;
  return 1;
}


/* Free all of allocaded resource for this file. */
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
