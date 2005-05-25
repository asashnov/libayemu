#ifndef _AYEMU_ay8912_h
#define _AYEMU_ay8912_h

#include <stddef.h>

BEGIN_C_DECLS

/** Types of stereo
 *
 *  The stereo types codes used for generage sound.
 */
typedef enum
{
  AYEMU_MONO = 0,
  AYEMU_ABC,
  AYEMU_ACB,
  AYEMU_BAC,
  AYEMU_BCA,
  AYEMU_CAB,
  AYEMU_CBA
}
ayemu_stereo_t;

/** AY registers data */
typedef struct
{
  int tone_a;		/** R0, R1 */
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
ayemu_ayregs_t;


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
  ayemu_ayregs_t regs;
  ayemu_sndfmt_t sndfmt;

  /* AY/YM chip params */
  int ChipFreq;		  /* chip freq */
  ayemu_chip_t ChipType;  /* chip type (AUEMU_AY | AUEMU_YM) */
  ayemu_stereo_t Stereo;  /* stereo layout (MONO, ABC, BAC, ACB...) */

  /* AY/YM volumes */
  int AYVol[32], YMVol[32];

  int Layout [6]; /** current mixer layaut A left, A right, B_l, B_r, C_l, C_r; */

  /* ----======= calculated values ======------- */
  int ChipTacts_per_outcount; /* chip's counts per one sound signal count */
  int Amp_Global;			// scale factor for amplitude

  /* Volume tables in current use It calculated by current AY/YM table and mixer values
   *  
   *  If you want change it on the fly, you must call ay8912.c::GenVol after
   *  change chip type, stereo type, AY/YM table, EQ settings
   */
  int vols [6][32];

  /* private configs - not change it from your code! */
  int bEQSet;             /* mixer initialized flag */
  int bChip;              /* AY/YM chip settings init flag */
  int bAY_table;          /* AY table init flag */
  int bYM_table;          /* YM table init flag */

  /*  -- internal variables -- */
  int EnvNum;		  /* current envilopment number (0...15) */
  int env_pos;		  /* envilop position (0...127) */
  int Cur_Seed; // = 0xffff;  /* random numbers counter */

  /* bits streams */
  int bit_a, bit_b, bit_c, bit_n;  /* current generator state */

  /* back counters */
  int cnt_a, cnt_b, cnt_c, cnt_n, cnt_e;
}
ayemu_ay_t;


/** Load user's AY volumes table
 *  (it requere 16 16-bit values
 */
extern DECLSPEC void ayemu_set_AY_table (ayemu_ay_t *ay, Uint16 *tbl);

/** Load user's YM volume table
 *  (it requered 32 16-bit values 
 */
extern DECLSPEC void ayemu_set_YM_table (ayemu_ay_t *ay, Uint16 *tbl);

/** Set chip and stereo type, chip frequence.
 *  Pass (-1) as value means set to default.
 */
extern DECLSPEC void ayemu_set_chip (ayemu_ay_t *ay, ayemu_chip_t chip, int chipfreq, ayemu_stereo_t stereo);

/** Set amplitude factor for each of channels (A,B anc C, tone and noise).
 *  Factor's value must be from (-100) to 100.
 *  If one or some values not in this interval, it accept to default.
 */
extern DECLSPEC void ayemu_set_EQ (ayemu_ay_t *ay, int A_l, int A_r, int B_l, int B_r, int C_l, int C_r);

/** Reset AY, generate internal volume tables
 *
 *  (by chip type, sound format and amplitude factors).
 *  NOTE: if you have call ayemu_set_EQ, ayemu_set_AY_table or ayemu_set_YM_table, 
 *  you need do it _before_ call this function to apply changes.
 */
extern DECLSPEC int ayemu_start (ayemu_ay_t *ay, int freq, int chans, int bits);

/** Set AY register data.
 *
 * regs is a pointer to 14-bytes frame of AY registers.
 */
extern DECLSPEC void ayemu_set_regs (ayemu_ay_t *ay, unsigned char *regs);

/** Generate sound.
 *  Fill 'numcount' of sound frames (1 sound frame is 4 bytes for 16-bit stereo)
 *  Return value: pointer to next data in output sound buffer
 */
extern DECLSPEC unsigned char * ayemu_gen_sound (ayemu_ay_t *ay, unsigned char *buf, size_t bufsize);

/** Free all data allocated by emulator
 */
extern DECLSPEC void ayemu_free (ayemu_ay_t *ay);

END_C_DECLS

#endif /* __AYEMU_ay8912_h */
