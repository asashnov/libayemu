#include "ayemu.h"

static const int DEBUG = 0;

/* Max amplitude value for stereo signal.
   Selected is 24575 for possible folowwing SSRC for clipping escape

original russian comment:
   Максимальная амплитуда возможная при проигрывании стерео сигнала
   24575 выбрано для последующего введения SSRC для избежания клиппинга
 */
#define MAX_AMP                     24575

const int AYEMU_DEFAULT_CHIP_FREQ = 1773400;

/* Prepare work volumes array (included AY table and mixer settings) */
static void GenVol (ayemu_ay_t *ay);	


/* AY/YM volume tables (16 elements for AY, 32 for YM) */
/* (c) by V_Soft and Lion 17 */
static uint16_t 
Lion17_AY_table [16] = {
  0, 513, 828, 1239, 1923, 3238, 4926, 9110, 10344, 17876, 24682, 30442,
  38844, 47270, 56402, 65535
};
static uint16_t 
Lion17_YM_table [32] = {
  0, 0, 190, 286, 375, 470, 560, 664, 866, 1130, 1515, 1803, 2253,
  2848, 3351, 3862, 4844, 6058, 7290, 8559, 10474, 12878, 15297, 17787,
  21500, 26172, 30866, 35676, 42664, 50986, 58842, 65535
};


/* (c) by Hacker KAY */
static  uint16_t 
KAY_AY_table [16] = {
  0, 836, 1212, 1773, 2619, 3875, 5397, 8823, 10392, 16706, 23339, 29292,
  36969, 46421, 55195, 65535
};
static  uint16_t 
KAY_YM_table [32] = {
  0, 0, 248, 450, 670, 826, 1010, 1239, 1552, 1919, 2314, 2626, 3131, 3778,
  4407, 5031, 5968, 7161, 8415, 9622, 11421, 13689, 15957, 18280, 21759,
  26148, 30523, 34879, 41434, 49404, 57492, 65535
};


/* default equlaizer (layout) settings for AY and YM, 7 stereo types */
static const int
default_layout [2][7][6] = {
{ /* for AY */
   /*   A_l  A_r  B_l  B_r  C_l  C_r */
  {100, 100, 100, 100, 100, 100},	// _MONO
  {100, 33, 70, 70, 33, 100},	// _ABC
  {100, 33, 33, 100, 70, 70},	// _ACB
  {70, 70, 100, 33, 33, 100},	// _BAC
  {33, 100, 100, 33, 70, 70},	// _BCA
  {70, 70, 33, 100, 100, 33},	// _CAB
  {33, 100, 70, 70, 100, 33} }  // _CBA
, { /* for YM */
    /*  A_l  A_r  B_l  B_r  C_l  C_r */
  {100, 100, 100, 100, 100, 100},	// _MONO
  {100, 5, 70, 70, 5, 100},	// _ABC
  {100, 5, 5, 100, 70, 70},	// _ACB
  {70, 70, 100, 5, 5, 100},	// _BAC
  {5, 100, 100, 5, 70, 70},	// _BCA
  {70, 70, 5, 100, 100, 5},	// _CAB
  {5, 100, 70, 70, 100, 5}}      // _CBA
};



/* AY/YM Envelops calculated by GenEnv() */
static int Envelope [16][128];

/* Init envelops global table (may be the better do it statical data) */
static void GenEnv ();
static int bEnvGenInit = 0;


/* TODO: deprecated, insert body to ayemu_start () */
/* Set output sound format. 44100 Hz, 16-bit stereo is default */
static int ayemu_set_sound_format (ayemu_ay_t *ay, int freq, int chans, int bits)
{
  if ((bits != 16 && bits != 8) || (chans != 2 && chans != 1))
    return 0;

  ay->sndfmt.freq = freq;
  ay->sndfmt.channels = chans;
  ay->sndfmt.bpc = bits;

  return 1;
}

/* Set chip and stereo type, chip frequence.
  Pass (-1) as value means set to default. */
void
ayemu_set_chip (ayemu_ay_t * ay, ayemu_chip_t chip, int chipfreq, ayemu_stereo_t stereo)
{
  ay->ChipType = (chip == -1) ? AYEMU_AY : chip;
  ay->Stereo   = (stereo == -1) ? AYEMU_ABC : stereo;
  ay->ChipFreq = (chipfreq == -1) ? AYEMU_DEFAULT_CHIP_FREQ : chipfreq;
  ay->bChip = 1;
  
  /* Re-gen internal volume table */
  ayemu_set_EQ (ay, 255,255,255,255,255,255);
  GenVol(ay);
}

  /* Set amplitude factor for each of channels (A,B anc C, tone and noise).
Factor's value must be from (-100) to 100.
If one or some values not in this interval, it accept to default. */
void
ayemu_set_EQ (ayemu_ay_t * ay, int A_l, int A_r, int B_l, int B_r, int C_l, int C_r)
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

  /* Load user's AY volumes table
(it requere 16 16-bit values */
void ayemu_set_AY_table (ayemu_ay_t * ay, uint16_t tbl[])
{
  int n;

  /* т.к. внутри программы для простоты обе таблицы по 32 элемента
     для АУ приходится их дублировать */
  for (n = 0; n < 16; n++)
    {
      ay->AYVol[n * 2 + 0] = tbl[n];
      ay->AYVol[n * 2 + 1] = tbl[n];
    }

  ay->bAY_table = 1;
}

  /* Load user's YM volume table
(it requered 32 16-bit values */
void ayemu_set_YM_table (ayemu_ay_t * ay, uint16_t * tbl)
{
  int n;

  for (n = 0; n < 32; n++)
    ay->YMVol[n] = tbl[n];

  ay->bYM_table = 1;
}


/* Reset AY, generate internal volume tables (by chip type, sound
   format and amplitude factors).

   NOTE: if you have call ayemu_set_EQ, ayemu_set_AY_table or
   ayemu_set_YM_table, you need do it _before_ call this function to
   apply changes. 
  */
int ayemu_start (ayemu_ay_t * ay, int freq, int chans, int bits)
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

  /* engine initialization */
  ay->CntA = 0;
  ay->CntB = 0;
  ay->CntC = 0;
  ay->CntN = 0;
  ay->Cur_Seed = 0xFFFF;  /* noise generator */
  ay->BitA = 0;  /* generator state bits */
  ay->BitB = 0;
  ay->BitC = 0;
  ay->BitN = 0;
  ay->EnvNum = 0; /* envelop init */
  ay->CntE = 0;
  ay->EnvPos = 0;

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
  ay->Amp_Global = ay->ChipTacts_per_outcount * vol / MAX_AMP;

  return 1;
}


void ayemu_set_regs (ayemu_ay_t * ay, uint8_t *regs)
{
  ay->regs.tone_a  = regs[0];
  ay->regs.tone_a += (regs[1] & 0x0f) << 8;
  if (DEBUG && (regs[1] & 0xf0))
    printf ("Warning: Possible invalid register data: R1 has some of bit 7-4.\n");

  ay->regs.tone_b  = regs[2];
  ay->regs.tone_b += (regs[3] & 0x0f) << 8;
  if (DEBUG && (regs[3] & 0xf0))
    printf ("Warning: Possible invalid register data: R3 has some of bit 7-4.\n");

  ay->regs.tone_c  = regs[4];
  ay->regs.tone_c += (regs[5] & 0x0f) << 8;
  if (DEBUG && (regs[5] & 0xf0))
    printf ("Warning: Possible invalid register data: R5 has some of bit 7-4.\n");

  ay->regs.noise = regs[6] & 0x1f;
  if (DEBUG && (regs[6] & 0xe0))
    printf ("Warning: Possible invalid register data: R6 has some of bit 7-5.\n");

  ay->regs.R7_tone_a  = ! (regs[7] & 0x01);
  ay->regs.R7_tone_b  = ! (regs[7] & 0x02);
  ay->regs.R7_tone_c  = ! (regs[7] & 0x04);
  ay->regs.R7_noise_a = ! (regs[7] & 0x08);
  ay->regs.R7_noise_b = ! (regs[7] & 0x10);
  ay->regs.R7_noise_c = ! (regs[7] & 0x20);

  /* set value to (-1) if use envelop */
  ay->regs.vol_a = regs[8]  & 0x0f;
  ay->regs.vol_b = regs[9]  & 0x0f;
  ay->regs.vol_c = regs[10] & 0x0f;

  ay->regs.env_a = regs[8]  & 0x10;
  ay->regs.env_b = regs[9]  & 0x10;
  ay->regs.env_c = regs[10] & 0x10;

  if (DEBUG && (regs[8] & 0xe0))
    printf ("Warning: Possible invalid register data: R8 has some of bit 7-5.\n");
  if (DEBUG && (regs[9] & 0xe0))
    printf ("Warning: Possible invalid register data: R9 has some of bit 7-5.\n");
  if (DEBUG && (regs[10] & 0xe0))
    printf ("Warning: Possible invalid register data: R10 has some of bit 7-5.\n");

  ay->regs.env_freq  = regs[11];
  ay->regs.env_freq += regs[12] << 8;

  if (regs[13] != 255)
    {
      ay->regs.env_style = regs[13] & 0x0f;
      ay->EnvPos = 0;
      ay->CntE = 0;
    }
  if (DEBUG && (regs[13] & 0xf0))
    printf ("Warning: Possible invalid register data: R13 has some of bit 7-4.\n");

}


/* Generate sound.
   Fill sound buffer with current register data
   Return value: pointer to next data in output sound buffer
*/
uint8_t *
ayemu_gen_sound (ayemu_ay_t * ay, uint8_t * sound_buf, size_t sound_bufsize)
{
  int mix_l, mix_r;
  int vol_e; /* value from current envelop */
  int vol;  			/* tmp result volume */
  int n, m;
  int snd_numcount;

  /* sound buffer size must be divide on 4 */
  snd_numcount = sound_bufsize / (ay->sndfmt.channels * (ay->sndfmt.bpc >> 3));

  for (n=0; n < snd_numcount; n++)
    {
      /* accumulate chip volumes for one sound tick */
      mix_l = mix_r = 0;
      
      /* doing all chip tackts for one sound tick */
      for (m = 0 ; m < ay->ChipTacts_per_outcount ; m++)
	{
	  /* increate channel counters */
	  if (++ay->CntA >= ay->regs.tone_a) 
	    {
	      ay->CntA = 0;
	      ay->BitA = ! ay->BitA;
	    }
	  if (++ay->CntB >= ay->regs.tone_b)
	    {
	      ay->CntB = 0;
	      ay->BitB = ! ay->BitB;
	    }
	  if (++ay->CntC >= ay->regs.tone_c)
	    {
	      ay->CntC = 0;
	      ay->BitC = ! ay->BitC;
	    }

	  /* GenNoise (c) Hacker KAY & Sergey Bulba */
	  if (++ay->CntN >= (ay->regs.noise * 2))
	    {
	      ay->CntN = 0;
	      ay->Cur_Seed = (ay->Cur_Seed * 2 + 1) ^ \
		(((ay->Cur_Seed >> 16) ^ (ay->Cur_Seed >> 13)) & 1); 
	      ay->BitN = ((ay->Cur_Seed >> 16) & 1);
	    }

	  /* gen envelope */
	  if (++ay->CntE >= ay->regs.env_freq)
	    {
	      ay->CntE = 0;
	      if (++ay->EnvPos > 127)
		ay->EnvPos = 64;
	    }

	  vol_e = Envelope [ay->regs.env_style] [ay->EnvPos];

	  /* channel A */
	  if ((ay->BitA | !ay->regs.R7_tone_a) & (ay->BitN | !ay->regs.R7_noise_a))
	    {
	      vol = (ay->regs.env_a)? vol_e : 
		ay->regs.vol_a * 2 + 1;   /* 15 * 2 + 1 = 31 max */
	      mix_l += ay->vols[0][vol];
	      mix_r += ay->vols[1][vol];
	    }

	  /* channel B */
	  if ((ay->BitB | !ay->regs.R7_tone_b) & (ay->BitN | !ay->regs.R7_noise_b))
	    {
	      vol =(ay->regs.env_b)? vol_e : 
		ay->regs.vol_b * 2 + 1;
	      mix_l += ay->vols[2][vol];
	      mix_r += ay->vols[3][vol];
	    }

	  /* channel C */
	  if ((ay->BitC | !ay->regs.R7_tone_c) & (ay->BitN | !ay->regs.R7_noise_c))
	    {
	      vol = (ay->regs.env_c)? vol_e : 
		ay->regs.vol_c * 2 + 1;
	      mix_l += ay->vols[4][vol];
	      mix_r += ay->vols[5][vol];
	    }

	} /* end for (m=0; ...) */

      mix_l /= ay->Amp_Global;
      mix_r /= ay->Amp_Global;
     
      if (ay->sndfmt.bpc == 8)
	{
	  mix_l = (mix_l >> 8) | 128; /* 8 bit sound */
	  mix_r = (mix_r >> 8) | 128;
	  *sound_buf++ = mix_l;
	  if (ay->Stereo != AYEMU_MONO)
	    *sound_buf++ = mix_r;
	}
      else
	{
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


/* Make volume tables for work by current chip type and layout */
static void
GenVol (ayemu_ay_t *ay)
{
  int n, m;
  int vol;

  for (n = 0; n < 32; n++)
    {
      vol = (ay->ChipType == AYEMU_AY) ? ay->AYVol[n] : ay->YMVol[n];

      for (m=0; m < 6; m++)
	ay->vols[m][n] = (int) (((double) vol * ay->Layout[m]) / 100);
    }
}


/* make chip hardware envelop tables */
static void
GenEnv ()
{
        int env, pos;
        int Hold;
        int Dir;
        int Vol;


	int n, m;

        bEnvGenInit = 1;

        // цикл по каждой огибающей
        for( env = 0; env<16; env++)
        {
                Hold = 0; // выключаем заморозку
                // Начинаем сверху или снизу? (бит 2)
                Dir = (env & 4)?  1 : -1;
                Vol = (env & 4)? -1 : 32;
                // строим огибающую
                for( pos=0; pos<128; pos++)
                {
                        // если не заморожено
                        if( !Hold )
                        {
                                // то изменяем громкость в соответствие с направлением
                                Vol += Dir;
                                // если достигли верхнего или нижнего предела
                                if ( Vol < 0 || Vol >= 32 )
                                {
                                        // если установлен бит 3 - продолжение
                                        if ( env & 8 )
                                        {
                                                // то если установлен бит 2- чередование
                                                // меняем направление
                                                if ( env & 2 ) Dir = -Dir;
                                                Vol = ( Dir > 0 ) ? 0:31;
                                                // если установлен бит 0-задержка
                                                // то замораживаем
                                                if ( env & 1 )
                                                {
                                                        Hold = 1;
                                                        Vol = ( Dir > 0 ) ? 31:0;
                                                }
                                        }
                                        // иначе, если нет бита продолжения, то
                                        // замораживаем и выключаем
                                        else
                                        {
                                                Vol = 0;
                                                Hold = 1;
                                        }
                                }
                        }
                        Envelope[env][pos] = Vol;
                }
        }

	/*
	for (n=0; n < 16 ; n++) {
	  for (m=0; m < 128; m++)
	    fprintf (stderr, "%3d ", Envelope[n][m]);
	  fprintf (stderr, "\n");
	}
	*/
}


/* Free all data allocated by emulator */
void ayemu_free (ayemu_ay_t *ay)
{
  /* nothing to free */
  return;
}
