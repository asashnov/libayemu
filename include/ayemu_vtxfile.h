#ifndef _AYEMU_vtxfile_h
#define _AYEMU_vtxfile_h

#include <stdio.h>
#include "ayemu_8912.h"

/* The following constants and data structure comes from
   VTX file format description. You can see them on
   http://
*/

#define AYEMU_VTX_NTSTRING_MAX 255

BEGIN_C_DECLS

typedef struct
{
  /* File header */
  char *Chip;			/* Type of sound chip (pointer to string "ay" or "ym") */
  uint8_t Stereo;			/* Type of stereo: 0-ABC, 1-BCA... (see VTX format description) */
  uint16_t Loop;			/* song loop */
  int ChipFreq;			/* AY chip freq (1773400 for ZX) */
  uint8_t PlayerFreq;		/* 50 Hz for ZX, 60 Hz for yamaha */
  uint16_t Year;			/* year song composed */

  /* following data from var-lenght strings */
  char Title   [AYEMU_VTX_NTSTRING_MAX+1];
  char Author  [AYEMU_VTX_NTSTRING_MAX+1];
  char From    [AYEMU_VTX_NTSTRING_MAX+1];
  char Tracker [AYEMU_VTX_NTSTRING_MAX+1];
  char Comment [AYEMU_VTX_NTSTRING_MAX+1];
 
  /* end file format data */

  /* decoded from lha data part */
  uint8_t *regdata;		/* NULL if not loaded */
  size_t regdata_size;

  FILE *fp;			/* opening .vtx file pointer */
  int pos;			/* current play register data offset (in 14-bytes frames) */
} ayemu_vtx_t;

  /* Open .vtx file and read vtx file header Return true if success,
     else false */
  extern int ayemu_vtx_open (ayemu_vtx_t *vtx, const char *filename);
  
  /* Read and encode lha data from .vtx file.
       Return pointer to unpacked data or NULL */
  extern void * ayemu_vtx_load_data (ayemu_vtx_t *vtx);
  
  /* Get next 14-bytes frame of AY register data.
     Return value: true if sucess, false if not enought data. */
  extern int ayemu_vtx_get_next_frame (ayemu_vtx_t *vtx, void *regs);
  
   /* Free all of allocaded resource for this file.
    You must call this function on end work with vtx file */
  extern void ayemu_vtx_free (ayemu_vtx_t *vtx);

END_C_DECLS

#endif
