#ifndef _AYEMU_ay8912_h
#define _AYEMU_ay8912_h

#include <stddef.h>
#include <stdint.h>

BEGIN_C_DECLS

/* Types of stereo (see VTX format description) */
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

/* Sound chip type */
typedef enum
  {
    AYEMU_AY = 0,
    AYEMU_YM
  }
ayemu_chip_t;

  /* Data structure for sound chip emulation */
typedef struct
{
  /* AY registers */
  int reg_tone_a;		/* R0, R1 */
  int reg_tone_b;		/* R2, R3 */
  int reg_tone_c;		/* R4, R5 */
  int reg_noise;		/* R6 */

  int reg_R7_tone_a;		/* R7 bit 0 */
  int reg_R7_tone_b;		/* R7 bit 1 */
  int reg_R7_tone_c;		/* R7 bit 2 */
  int reg_R7_noise_a;		/* R7 bit 3 */
  int reg_R7_noise_b;		/* R7 bit 4 */
  int reg_R7_noise_c;		/* R7 bit 5 */

  int reg_vol_a;		/* R8 bits 3-0 */
  int reg_vol_b;		/* R9 bits 3-0 */
  int reg_vol_c;		/* R10 bits 3-0 */

  int reg_env_a;		/* R8 bit 4 */
  int reg_env_b;		/* R9 bit 4 */
  int reg_env_c;		/* R10 bit 4 */

  int reg_env_freq;		/* R11, R12 */
  int reg_env_style;		/* R13 */

  /* Output sound format */
  int output_freq;    
  int output_channels; 
  int output_bpc;

  /* AY/YM chip params */
  int ChipFreq;		  /* chip freq */
  ayemu_chip_t ChipType;  /* chip type (AUEMU_AY | AUEMU_YM) */
  ayemu_stereo_t Stereo;  /* stereo layout (MONO, ABC, BAC, ACB...) */

  /* AY/YM volumes */
  uint16_t AYVol[32], YMVol[32];

  /* current mixer layaut A left, A right, B_l, B_r, C_l, C_r; */
  int Layout [6];

  /* ----======= calculated values ======------- */
  int ChipTacts_per_outcount; /* chip's counts per one sound signal count */
  int Amp_Global;			// scale factor for amplitude

  /* Volume tables in current use It calculated by current AY/YM table and mixer values
     
     If you want change it on the fly, you must call ay8912.c::GenVol after
     change chip type, stereo type, AY/YM table, EQ settings
   */
  int vols [6][32];

  /* private configs - not change it from your code! */
  int bEQSet;             /* mixer initialized flag */
  int bChip;              /* AY/YM chip settings init flag */
  int bAY_table;          /* AY table init flag */
  int bYM_table;          /* YM table init flag */

  /*  -- internal variables -- */
  int EnvNum;		  /* current envilopment number (от 0 до 15) */
  int EnvPos;		  /* envilop position (от 0 до 127) */
  int Cur_Seed; // = 0xffff;  /* random numbers counter */

  /* bits streams */
  int BitA, BitB, BitC, BitN;  /* current generator state */

  /* back counters */
  int CntA,  CntB, CntC, CntE, CntN;
}
ayemu_ay_t;


  /* Load user's AY volumes table
     (it requere 16 16-bit values */
extern void ayemu_set_AY_table (ayemu_ay_t * ay, uint16_t tbl[]);

  /* Load user's YM volume table
     (it requered 32 16-bit values */
extern void ayemu_set_YM_table (ayemu_ay_t * ay, uint16_t * tbl);

/* Set chip and stereo type, chip frequence.
   Pass (-1) as value means set to default. */
extern void ayemu_set_chip (ayemu_ay_t *ay, ayemu_chip_t chip, int chipfreq, ayemu_stereo_t stereo);

  /* Set amplitude factor for each of channels (A,B anc C, tone and noise).
     Factor's value must be from (-100) to 100.
     If one or some values not in this interval, it accept to default. */
extern void ayemu_set_EQ (ayemu_ay_t *ay, int A_l, int A_r, int B_l, int B_r, int C_l, int C_r);

  /* Reset AY, generate internal volume tables
     (by chip type, sound format and amplitude factors).
     NOTE: if you have call ayemu_set_EQ, ayemu_set_AY_table or ayemu_set_YM_table, you need do it _before_ call this function to apply changes. */
extern int ayemu_start (ayemu_ay_t * ay, int freq, int chans, int bits);

  /* Set AY register data.
     regs is a pointer to 14-bytes frame of AY registers. */
extern void ayemu_set_regs (ayemu_ay_t *ay, uint8_t regs[]);

  /* Generate sound.
     Fill 'numcount' of sound frames (1 sound frame is 4 bytes for 16-bit stereo)
     Return value: pointer to next data in output sound buffer  */
extern uint8_t * ayemu_gen_sound (ayemu_ay_t *ay, uint8_t * buf, size_t bufsize);

/* Free all data allocated by emulator */
extern void ayemu_free (ayemu_ay_t *ay);

END_C_DECLS

#endif /* __AYEMU_ay8912_h */
