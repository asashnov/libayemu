#include "ayemu.h"

static const char VERSION[] = "libayemu 0.9";

enum {
  AYEMU_MAX_AMP = 24575, /** Max amplitude value for stereo signal for avoiding for possible folowwing SSRC for clipping */
  AYEMU_EQ_DEFAULT = 255,
  AYEMU_CHIP_DEFAULT = -1,
  AYEMU_DEFAULT_CHIP_FREQ = 1773400
};

/* AY/YM volume tables (16 elements for AY, 32 for YM)
 * (c) by V_Soft and Lion 17
 */
static Uint16 Lion17_AY_table [16] = {
  0, 513, 828, 1239, 1923, 3238, 4926, 9110, 10344, 17876, 24682, 30442,
  38844, 47270, 56402, 65535
};

static Uint16 Lion17_YM_table [32] = {
  0, 0, 190, 286, 375, 470, 560, 664, 866, 1130, 1515, 1803, 2253,
  2848, 3351, 3862, 4844, 6058, 7290, 8559, 10474, 12878, 15297, 17787,
  21500, 26172, 30866, 35676, 42664, 50986, 58842, 65535
};

/* (c) by Hacker KAY */
static  Uint16 KAY_AY_table [16] = {
  0, 836, 1212, 1773, 2619, 3875, 5397, 8823, 10392, 16706, 23339, 29292,
  36969, 46421, 55195, 65535
};

static  Uint16 KAY_YM_table [32] = {
  0, 0, 248, 450, 670, 826, 1010, 1239, 1552, 1919, 2314, 2626, 3131, 3778,
  4407, 5031, 5968, 7161, 8415, 9622, 11421, 13689, 15957, 18280, 21759,
  26148, 30523, 34879, 41434, 49404, 57492, 65535
};

/* default equlaizer (layout) settings for AY and YM, 7 stereo types */
static const int default_layout [2][7][6] = {
  { /* for AY */
    /*   A_l  A_r  B_l  B_r  C_l  C_r */
    {100, 100, 100, 100, 100, 100},	// _MONO
    {100, 33, 70, 70, 33, 100},	   // _ABC
    {100, 33, 33, 100, 70, 70},	   // _ACB
    {70, 70, 100, 33, 33, 100},	   // _BAC
    {33, 100, 100, 33, 70, 70},	   // _BCA
    {70, 70, 33, 100, 100, 33},	   // _CAB
    {33, 100, 70, 70, 100, 33}},   // _CBA
  { /* for YM */
    /*  A_l  A_r  B_l  B_r  C_l  C_r */
    {100, 100, 100, 100, 100, 100},	// _MONO
    {100, 5, 70, 70, 5, 100},	// _ABC
    {100, 5, 5, 100, 70, 70},	// _ACB
    {70, 70, 100, 5, 5, 100},	// _BAC
    {5, 100, 100, 5, 70, 70},	// _BCA
    {70, 70, 5, 100, 100, 5},	// _CAB
    {5, 100, 70, 70, 100, 5}}   // _CBA
};


/* Make volume tables for work by current chip type and layout */
static void GenVol (ayemu_ay_t *ay)
{
  int n, m;
  int vol;
	
  for (n = 0; n < 32; n++) {
    vol = (ay->ChipType == AYEMU_AY) ? ay->AYVol[n] : ay->YMVol[n];
    for (m=0; m < 6; m++)
      ay->vols[m][n] = (int) (((double) vol * ay->Layout[m]) / 100);
  }
}


/* AY/YM Envelops calculated by GenEnv() */
static int Envelope [16][128];
static int bEnvGenInit = 0;

/* make chip hardware envelop tables */
static void GenEnv ()
{
  int env;
  int pos;
  int hold;
  int dir;
  int vol;
	
  for (env = 0; env < 16; env++) {
    hold = 0;
    dir = (env & 4)?  1 : -1;
    vol = (env & 4)? -1 : 32;
    for (pos = 0; pos < 128; pos++) {
      if (!hold) {
	vol += dir;
	if (vol < 0 || vol >= 32) {
	  if ( env & 8 ) {
	    if ( env & 2 ) dir = -dir;
	    vol = (dir > 0 )? 0:31;
	    if ( env & 1 ) {
	      hold = 1;
	      vol = ( dir > 0 )? 31:0;
	    }
	  } else {
	    vol = 0;
	    hold = 1;
	  }
	}
      }
      Envelope[env][pos] = vol;
    }
  }
  bEnvGenInit = 1;
}

/* Set output sound format. 44100 Hz, 16-bit stereo is default */
static int ayemu_set_sound_format (ayemu_ay_t *ay, int freq, int chans, int bits)
{
  if (   (bits != 16 && bits != 8) 
	 || (chans != 2 && chans != 1)
	 || (freq < 50))
    return 0;
  ay->sndfmt.freq = freq;
  ay->sndfmt.channels = chans;
  ay->sndfmt.bpc = bits;
  return 1;
}

/** Set chip type, stereo layout and chip freq.
 *
 * You can pass AYEMU_SETCHIP_DEFAULT for default value.
 */
void ayemu_set_chip(ayemu_ay_t *ay, ayemu_chip_t chip, int chipfreq, ayemu_stereo_t stereo)
{
  ay->ChipType = (chip != AYEMU_AY || chip != AYEMU_YM) ? AYEMU_AY : chip;
  ay->Stereo   = (stereo < 0 || stereo > 6) ? AYEMU_ABC : stereo;
  ay->ChipFreq = (chipfreq < 0) ? AYEMU_DEFAULT_CHIP_FREQ : chipfreq;
  ay->bChip = 1;
  
  /* Re-gen internal volume table */
  ayemu_set_EQ (ay, 255,255,255,255,255,255);
  GenVol(ay);
}

/** Set amplitude factor for each of channels (A,B anc C, tone and noise).
 *
 * Factor's value must be from (-100) to 100.
 * If one or some values not in this interval, it accept to default.
 */
void ayemu_set_EQ(ayemu_ay_t *ay, int A_l, int A_r, int B_l, int B_r, int C_l, int C_r)
{
  int def_Amp_A_l, def_Amp_A_r;
  int def_Amp_B_l, def_Amp_B_r;
  int def_Amp_C_l, def_Amp_C_r;
  int n;

  n = (ay->ChipType == AYEMU_AY) ? 0 : 1;

  /* Get defaut mixer layout for appropriate stereo type (ABC, ACB,...) */
  def_Amp_A_l = default_layout [n] [ay->Stereo] [0];
  def_Amp_A_r = default_layout [n] [ay->Stereo] [1];
  def_Amp_B_l = default_layout [n] [ay->Stereo] [2];
  def_Amp_B_r = default_layout [n] [ay->Stereo] [3];
  def_Amp_C_l = default_layout [n] [ay->Stereo] [4];
  def_Amp_C_r = default_layout [n] [ay->Stereo] [5];

  ay->Layout[0] = ((A_l < -100) || (A_l > 100)) ? def_Amp_A_l : A_l;
  ay->Layout[1] = ((A_r < -100) || (A_r > 100)) ? def_Amp_A_r : A_r;
  ay->Layout[2] = ((B_l < -100) || (B_l > 100)) ? def_Amp_B_l : B_l;
  ay->Layout[3] = ((B_r < -100) || (B_r > 100)) ? def_Amp_B_r : B_r;
  ay->Layout[4] = ((C_l < -100) || (C_l > 100)) ? def_Amp_C_l : C_l;
  ay->Layout[5] = ((C_r < -100) || (C_r > 100)) ? def_Amp_C_r : C_r;

  ay->bEQSet = 1;
}

/** Load user's AY volumes table
 *
 *  Loading user's AY volumes table requere 16 16-bit integer values 
 */
void ayemu_set_AY_table (ayemu_ay_t *ay, Uint16 tbl[16])
{
  int n;

  for (n = 0; n < 16; n++) {
    ay->AYVol[n * 2 + 0] = tbl[n];
    ay->AYVol[n * 2 + 1] = tbl[n];
  }
  ay->bAY_table = 1;
}

/** Load user's YM volume table
 * 
 * (it requered 32 16-bit values)
 */
void ayemu_set_YM_table (ayemu_ay_t *ay, Uint16 tbl[32])
{
  int n;

  for (n = 0; n < 32; n++)
    ay->YMVol[n] = tbl[n];
  ay->bYM_table = 1;
}

/** Initialize sound chip emulator
 *
 * Reset AY, generate internal volume tables (by chip type, sound
 * format and amplitude factors).
 *
 *  NOTE: if you have call ayemu_set_EQ, ayemu_set_AY_table or
 *  ayemu_set_YM_table, you need do it _before_ call this function to
 *  apply changes. 
 */
int ayemu_start(ayemu_ay_t *ay, int freq, int chans, int bits)
{
  int vol, max_l, max_r;

  /* Global envelop table (may be hardcode it) */
  if (! bEnvGenInit)
    GenEnv ();

  /* Set defaults if not chip selected */
  if (! ay->bChip)
    ayemu_set_chip (ay, -1, -1, -1);

  ayemu_set_sound_format (ay, freq, chans, bits);

  ay->ChipTacts_per_outcount = ay->ChipFreq / ay->sndfmt.freq / 8;

  /* If equlaizer is not init yet set defaults */
  //  if (! ay->bEQSet)
  ayemu_set_EQ (ay, 255, 255, 255, 255, 255, 255);

  ay->Cur_Seed = 0xFFFF;  /* init value for noise generator */
  ay->cnt_a = ay->cnt_b = ay->cnt_c = ay->cnt_n = ay->cnt_e = 0;
  ay->bit_a = ay->bit_b = ay->bit_c = ay->bit_n = 0;
  ay->env_pos = ay->EnvNum = 0;

  //  if (! ay->bAY_table)
  ayemu_set_AY_table (ay, Lion17_AY_table);

  //  if (! ay->bYM_table)
  ayemu_set_YM_table (ay, Lion17_YM_table);

  /* You must already setup chip type (AY or YM) and equalaiser
     before calculate resulting volume tables */
  GenVol (ay);

  /* динамическая настройка глобального коэффициента усиления
     подразумевается, что в vols [x][31] лежит самая большая громкость
     TODO: Сделать проверку на это ;-)
  */
  max_l = ay->vols[0][31] + ay->vols[2][31] + ay->vols[3][31];
  max_r = ay->vols[1][31] + ay->vols[3][31] + ay->vols[5][31];
  vol = (max_l > max_r) ? max_l : max_r;  // =157283 on all defaults
  ay->Amp_Global = ay->ChipTacts_per_outcount *vol / AYEMU_MAX_AMP;

  return 1;
}

#define WARN_IF_REGISTER_GREAT_THAN(r,m) \
	if (*(regs + r) > m) \
		fprintf(stderr, "ayemu_set_regs: warning: possible bad register data- R%d > %d\n", r, m)

/** Assign values for AY registers.
 *
 * You must pass array of char [14] to this function
 */
void ayemu_set_regs(ayemu_ay_t *ay, unsigned char *regs)
{
  WARN_IF_REGISTER_GREAT_THAN(1,15);
  WARN_IF_REGISTER_GREAT_THAN(3,15);
  WARN_IF_REGISTER_GREAT_THAN(5,15);
  WARN_IF_REGISTER_GREAT_THAN(8,31);
  WARN_IF_REGISTER_GREAT_THAN(9,31);
  WARN_IF_REGISTER_GREAT_THAN(10,31);
  WARN_IF_REGISTER_GREAT_THAN(13,15);

  ay->regs.tone_a  = regs[0] + (regs[1] & 0x0f) << 8;
  ay->regs.tone_b  = regs[2] += (regs[3] & 0x0f) << 8;
  ay->regs.tone_c  = regs[4] += (regs[5] & 0x0f) << 8;

  ay->regs.noise = regs[6] & 0x1f;

  ay->regs.R7_tone_a  = ! (regs[7] & 0x01);
  ay->regs.R7_tone_b  = ! (regs[7] & 0x02);
  ay->regs.R7_tone_c  = ! (regs[7] & 0x04);

  ay->regs.R7_noise_a = ! (regs[7] & 0x08);
  ay->regs.R7_noise_b = ! (regs[7] & 0x10);
  ay->regs.R7_noise_c = ! (regs[7] & 0x20);

  ay->regs.vol_a = regs[8]  & 0x0f;
  ay->regs.vol_b = regs[9]  & 0x0f;
  ay->regs.vol_c = regs[10] & 0x0f;
  ay->regs.env_a = regs[8]  & 0x10;
  ay->regs.env_b = regs[9]  & 0x10;
  ay->regs.env_c = regs[10] & 0x10;
  ay->regs.env_freq = regs[11] += regs[12] << 8;

  if (regs[13] != 0xff) {                   /* R13 = 255 means continue curent envelop */
    ay->regs.env_style = regs[13] & 0x0f;
    ay->env_pos = ay->cnt_e = 0;
  }
}

/** Generate sound.
 *
 * Fill sound buffer with current register data
 * Return value: pointer to next data in output sound buffer
 */
unsigned char *ayemu_gen_sound(ayemu_ay_t *ay, unsigned char *sound_buf, size_t sound_bufsize)
{
  int mix_l, mix_r;
  int tmpvol;
  int m;
  int snd_numcount;
	
  snd_numcount = sound_bufsize / (ay->sndfmt.channels * (ay->sndfmt.bpc >> 3));
  while (snd_numcount-- > 0) {
    mix_l = mix_r = 0;
		
    for (m = 0 ; m < ay->ChipTacts_per_outcount ; m++) {
      if (++ay->cnt_a >= ay->regs.tone_a) {
	ay->cnt_a = 0;
	ay->bit_a = ! ay->bit_a;
      }
      if (++ay->cnt_b >= ay->regs.tone_b) {
	ay->cnt_b = 0;
	ay->bit_b = ! ay->bit_b;
      }
      if (++ay->cnt_c >= ay->regs.tone_c) {
	ay->cnt_c = 0;
	ay->bit_c = ! ay->bit_c;
      }
			
      /* GenNoise (c) Hacker KAY & Sergey Bulba */
      if (++ay->cnt_n >= (ay->regs.noise * 2)) {
	ay->cnt_n = 0;
	ay->Cur_Seed = (ay->Cur_Seed * 2 + 1) ^ \
	  (((ay->Cur_Seed >> 16) ^ (ay->Cur_Seed >> 13)) & 1); 
	ay->bit_n = ((ay->Cur_Seed >> 16) & 1);
      }
			
      if (++ay->cnt_e >= ay->regs.env_freq) {
	ay->cnt_e = 0;
	if (++ay->env_pos > 127)
	  ay->env_pos = 64;
      }

#define ENVVOL Envelope [ay->regs.env_style][ay->env_pos]

      if ((ay->bit_a | !ay->regs.R7_tone_a) & (ay->bit_n | !ay->regs.R7_noise_a)) {
	tmpvol = (ay->regs.env_a)? ENVVOL : ay->regs.vol_a * 2 + 1;
	mix_l += ay->vols[0][tmpvol];
	mix_r += ay->vols[1][tmpvol];
      }
			
      if ((ay->bit_b | !ay->regs.R7_tone_b) & (ay->bit_n | !ay->regs.R7_noise_b)) {
	tmpvol =(ay->regs.env_b)? ENVVOL :  ay->regs.vol_b * 2 + 1;
	mix_l += ay->vols[2][tmpvol];
	mix_r += ay->vols[3][tmpvol];
      }
			
      if ((ay->bit_c | !ay->regs.R7_tone_c) & (ay->bit_n | !ay->regs.R7_noise_c)) {
	tmpvol = (ay->regs.env_c)? ENVVOL : ay->regs.vol_c * 2 + 1;
	mix_l += ay->vols[4][tmpvol];
	mix_r += ay->vols[5][tmpvol];
      }			
    } /* end for (m=0; ...) */
		
    mix_l /= ay->Amp_Global;
    mix_r /= ay->Amp_Global;
		
    if (ay->sndfmt.bpc == 8) {
      mix_l = (mix_l >> 8) | 128; /* 8 bit sound */
      mix_r = (mix_r >> 8) | 128;
      *sound_buf++ = mix_l;
      if (ay->Stereo != AYEMU_MONO)
	*sound_buf++ = mix_r;
    } else {
      *sound_buf++ = mix_l & 0x00FF; /* 16 bit sound */
      *sound_buf++ = (mix_l >> 8);
      if (ay->Stereo != AYEMU_MONO) {
	*sound_buf++ = mix_r & 0x00FF;
	*sound_buf++ = (mix_r >> 8);
      }
    }
  }
  return sound_buf;
}

/** Free all data allocated by emulator
 *
 * For now it do nothing.
 */
void ayemu_free (ayemu_ay_t *ay)
{
  /* nothing to do here */
  return;
}
