#ifndef _AYEMU_vtxfile_h
#define _AYEMU_vtxfile_h

#include <stdio.h>
#include "ayemu_8912.h"

/* The following constants and data structure comes from
   VTX file format description. You can see them on
   http://
*/

#define AYEMU_VTX_NTSTRING_MAX 255

typedef char NTstring[AYEMU_VTX_NTSTRING_MAX+1];

BEGIN_C_DECLS

/** VTX file format header and status of open file
 * 
 * This structure used for save VTX file format header's data
 * (song author, title and so on).
 */
struct VTXFileHeader
{
  ayemu_chip_t chiptype;    /* Type of sound chip */
  int      stereo;	    /* Type of stereo: 0-ABC, 1-BCA... (see VTX format description) */
  int      loop;	    /* song loop */
  int      chipFreq;	    /* AY chip freq (1773400 for ZX) */
  int      playerFreq;	    /* 50 Hz for ZX, 60 Hz for yamaha */
  int      year;            /* year song composed */
  NTstring title;
  NTstring author;
  NTstring from;
  NTstring tracker;
  NTstring comment;
  size_t   regdata_size;
};

/** structure for VTX file format handler
 *
 * It stores VTX file header and current state
 * (open file pointer, extracted register data, etc).
 */
typedef struct
{
  FILE *fp;			/* opening .vtx file pointer */
  struct VTXFileHeader hdr;  
  char *regdata;		/* NULL if not loaded */
  int pos;			/* current data frame offset */
} ayemu_vtx_t;

/* Open .vtx file and read vtx file header Return true if success,
   else false */
extern DECLSPEC int ayemu_vtx_open (ayemu_vtx_t *vtx, const char *filename);
  
/* Read and encode lha data from .vtx file.
   Return pointer to unpacked data or NULL */
extern DECLSPEC char *ayemu_vtx_load_data (ayemu_vtx_t *vtx);
  
/* Get next 14-bytes frame of AY register data.
   Return value: true if sucess, false if not enought data. */
extern DECLSPEC int ayemu_vtx_get_next_frame (ayemu_vtx_t *vtx, char *regs);
  
/* Print formated file name. If fmt is NULL the default format %a - %t will used */
extern DECLSPEC void ayemu_vtx_sprintname (const ayemu_vtx_t *vtx, char *buf, const int sz, const char *fmt);

/* Free all of allocaded resource for this file.
   You must call this function on end work with vtx file */
extern DECLSPEC void ayemu_vtx_free (ayemu_vtx_t *vtx);

END_C_DECLS

#endif
