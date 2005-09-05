#ifndef _AYEMU_ay8912_h
#define _AYEMU_ay8912_h

#include <stddef.h>

BEGIN_C_DECLS

/** Types of stereo.
    The stereo types codes used for generage sound. */
typedef enum
{
  AYEMU_MONO = 0,
  AYEMU_ABC,
  AYEMU_ACB,
  AYEMU_BAC,
  AYEMU_BCA,
  AYEMU_CAB,
  AYEMU_CBA,
  AYEMU_STEREO_CUSTOM = 255
} ayemu_stereo_t;

/** Sound chip type */
typedef enum {
  AYEMU_AY,
  AYEMU_YM,
  AYEMU_AY_LION17,
  AYEMU_YM_LION17,
  AYEMU_AY_KAY,
  AYEMU_YM_KAY,
  AYEMU_AY_CUSTOM,
  AYEMU_YM_CUSTOM
} ayemu_chip_t;

/** AY registers data */
typedef struct
{
  int tone_a;           /** R0, R1 */
  int tone_b;		/** R2, R3 */	
  int tone_c;		/** R4, R5 */
  int noise;		/** R6 */
  int R7_tone_a;	/** R7 bit 0 */
  int R7_tone_b;	/** R7 bit 1 */
  int R7_tone_c;	/** R7 bit 2 */
  int R7_noise_a;	/** R7 bit 3 */
  int R7_noise_b;	/** R7 bit 4 */
  int R7_noise_c;	/** R7 bit 5 */
  int vol_a;		/** R8 bits 3-0 */
  int vol_b;		/** R9 bits 3-0 */
  int vol_c;		/** R10 bits 3-0 */
  int env_a;		/** R8 bit 4 */
  int env_b;		/** R9 bit 4 */
  int env_c;		/** R10 bit 4 */
  int env_freq;		/** R11, R12 */
  int env_style;	/** R13 */
}
ayemu_regdata_t;


/** Output sound format */
typedef struct
{
  int freq;
  int channels;
  int bpc;
}
ayemu_sndfmt_t;


/** Data structure for sound chip emulation */
typedef struct
{
  /* emulator settings */
  int table[32];		/* table of volumes for chip */
  ayemu_chip_t type;		/* general chip type (AY or YM) */
  int ChipFreq;			/* chip frequency */
  int eq[6];			/* volumes for channels (A left, A right,...) range: -100...100 */
  ayemu_regdata_t regs;		/* registers data */
  ayemu_sndfmt_t sndfmt;	/* output sound format */

  /* flags */
  int default_chip_flag;
  int default_stereo_flag;
  int default_sound_format_flag;
  int dirty;

  /* emulator valiables */
  int bit_a, bit_b, bit_c, bit_n; /* current generator state */
  int cnt_a, cnt_b, cnt_c, cnt_n, cnt_e; /* back counters */
  int ChipTacts_per_outcount; /* chip's counts per one sound signal count */
  int Amp_Global;		/* scale factor for amplitude */
  int vols[6][32];  /* stereo type (channel volumes) and chip table */
  int EnvNum;		     /* current envilopment number (0...15) */
  int env_pos;			/* envilop position (0...127) */
  int Cur_Seed;		       /* random numbers counter */
}
ayemu_ay_t;


/* Interface of library */

EXTERN void
ayemu_init(ayemu_ay_t *ay);

EXTERN void
ayemu_reset(ayemu_ay_t *ay);

EXTERN int 
ayemu_set_chip_type(ayemu_ay_t *ay, ayemu_chip_t chip, int *custom_table);

EXTERN void
ayemu_set_chip_freq(ayemu_ay_t *ay, int chipfreq);

EXTERN int
ayemu_set_stereo(ayemu_ay_t *ay, ayemu_stereo_t stereo, int *custom_eq);

EXTERN int
ayemu_set_sound_format (ayemu_ay_t *ay, int freq, int chans, int bits);

EXTERN void 
ayemu_set_regs (ayemu_ay_t *ay, unsigned char *regs);

EXTERN void*
ayemu_gen_sound (ayemu_ay_t *ay, void *buf, size_t bufsize);


END_C_DECLS
#endif
