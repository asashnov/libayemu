/* Test AY/YM emulation programm */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <memory.h>


/* Standard includes for OSS DSP using */
/*
 * Use 'aoss' wrapper from alsa-oss package:
 * $ sudo apt-get install alsa-oss
 * $ aoss ./test_libayemu
 * (tested on Ubuntu 14.04)
 */

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/soundcard.h>

#include "../include/ayemu.h"

struct option options[] = {
  { "chip",    required_argument, NULL, 'c'},
  { "test",    required_argument, NULL, 't'},
  { "seconds", required_argument, NULL, 's'},
  { "usage",   no_argument, NULL, 'u'},
  { "help",    no_argument, NULL, 'h'},
  { 0, 0, 0, 0}
};
char short_args[] = "+yt:s:uh";

#define DEVICE_NAME "/dev/dsp"

ayemu_ay_t ay;
char * audio_buf;
size_t audio_bufsize;
int audio_fd; /* OSS device file descriptor */
int freq, chans, bits;
int n;

int chip = AYEMU_AY;  // chip (0=ay, 1=ym)
int test = 0;  // test number
int seconds = 5; 		/* number of seconds for each test */

void usage ()
{
  printf(
	 "Test AY/YM library programm.\n"
	 "Visit http://libayemu.sourceforge.net for detalis\n"
	 "Usage: test [option(s)]\n"
	 "  -y --ym\tym chip test (default ay)\n"
	 "  -t --test <number>\tstart test number (default 0)\n"
	 "  -s --seconds <number>\tnumber of seconds to each test (default 1)\n"
	 "  -h --help\n"
	 "  -u --usage\tthis help\n"
	 );
}

#define MUTE       0
#define TONE_A	   1
#define TONE_B     2
#define TONE_C     4
#define NOISE_A    8
#define NOISE_B   16
#define NOISE_C   32


void Test();

void gen_sound (tonea, toneb, tonec, noise, control, vola, volb, volc, envfreq, envstyle)
{
  int n, len;
  unsigned char regs[14];

  /* setup regs */
  regs[0] = tonea & 0xff;
  regs[1] = tonea >> 8;
  regs[2] = toneb & 0xff;
  regs[3] = toneb >> 8;
  regs[4] = tonec & 0xff;
  regs[5] = tonec >> 8;
  regs[6] = noise;
  regs[7] = (~control) & 0x3f; 	/* invert bits 0-5 */
  regs[8] = vola; 		/* included bit 4 */
  regs[9] = volb;
  regs[10] = volc;
  regs[11] = envfreq & 0xff;
  regs[12] = envfreq >> 8;
  regs[13] = envstyle;

  /* test setreg function: set from array and dump internal regs data */
  ayemu_set_regs(&ay, regs);
  printf ("\tRegs: A=%d B=%d C=%d N=%02d R7=[%d%d%d%d%d%d] "
	  "vols: A=%d B=%d C=%d EnvFreq=%d style %d\n",
	  ay.regs.tone_a, ay.regs.tone_b, ay.regs.tone_c, ay.regs.noise, 
	  ay.regs.R7_tone_a, ay.regs.R7_tone_b, ay.regs.R7_tone_c, 
	  ay.regs.R7_noise_a, ay.regs.R7_noise_b, ay.regs.R7_noise_c,
	  ay.regs.vol_a, ay.regs.vol_b, ay.regs.vol_c,
	  ay.regs.env_freq, ay.regs.env_style);

  /* generate sound */
  for (n=0; n < seconds; n++) {
    ayemu_gen_sound (&ay, audio_buf, audio_bufsize);
    if (write(audio_fd, audio_buf, audio_bufsize) == -1) {
      fprintf (stderr, "Error writting to sound device, break.\n");
      break;
    }
  }
}

void init_oss()
{
  if ((audio_fd = open(DEVICE_NAME, O_WRONLY, 0)) == -1) {
    fprintf (stderr, "Can't open /dev/dsp\n");
  }
  else if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &bits) == -1) {
    fprintf (stderr, "Can't set sound format\n");
  }
  else if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &chans) == -1) {
    fprintf (stderr, "Can't set number of channels\n");
  }
  else if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &freq) == -1) {
    fprintf (stderr, "Can't set audio freq\n");
  }
  else
    return;

  fprintf(stderr, "OSS initialization failed\n");
  exit(1);
}




int main (int argc, char **argv)
{
  int c;
  /* `getopt_long' stores the option index here. */
  int option_index = 0;
  
  opterr = 0;
  
  while ((c = getopt_long (argc, argv, short_args, options, &option_index)) != -1)
    switch (c)
      {
      case 'y':
	chip = 1;
	break;
      case 't':
	test = atoi (optarg);
	break;
      case 's':
	seconds = atoi (optarg);
	break;
      case 'h':
      case 'u':
	usage ();
	exit (0);
      case '?':
	if (isprint (optopt))
	  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
	else
	  fprintf (stderr,
		   "Unknown option character `\\x%x'.\n",
		   optopt);
	usage ();
	return 1;
      case -1:
	break;
      default:
	abort ();
      }
     
  printf ("Result of option parse: chip = %d, test = %d \n", chip,test);

  memset (&ay, 0, sizeof(ay));

  freq = 44100;
  chans = 2;  /* 1=mono, 2=stereo */
  bits = 16;  /* 16 or 8 bit */

  init_oss(&freq, &chans, &bits);
  printf ("OSS sound system initialization success: bits=%d, chans=%d, freq=%d\n",
	  bits, chans, freq);

  /* Allocate audio buffer for one AY frame */
  audio_bufsize = freq * chans * (bits >> 3);
  if ((audio_buf = (char*) malloc (audio_bufsize)) == NULL) {
    fprintf (stderr, "Can't allocate sound buffer\n");
    goto free_audio;
  }

  ayemu_init(&ay);

  Test();
    
 free_audio:
  close (audio_fd);
  return 0;
}

struct test_t {
  char *name;
  int tonea, toneb, tonec, noise, control, vola, volb, volc, envfreq, envstyle;
} 
testcases [] = {

  {"Mute: tones 400, volumes 15 noise 15", 
   400, 400, 400, 15 /*Noise*/, MUTE /* Ctrl */, 15, 15, 15, /* env freq,style */4000, 4 },
  {"Mute: tones 400, noise 25, volumes 31 (use env)", 
   400, 400, 400, 25, MUTE, 31, 31, 31, /* env freq,style */4000, 4 },

  {"Channel A: tone 400, volume 0", 
   400, 0, 0, 0, TONE_A, 0, 0, 0, /* env freq,style */0, 0 },
  {"Channel A: tone 400, volume 5",  
   400, 0, 0, 0, TONE_A, 5, 0, 0, 0, 0},
  {"Channel A: tone 400, volume 10", 
   400, 0, 0, 0, TONE_A, 10, 0, 0, 0, 0},
  {"Channel A: tone 400, volume 15", 
   400, 0, 0, 0, TONE_A, 15, 0, 0, 0, 0},

  {"Channel B: tone 400, volume 0", 
   0, 400, 0, 0/*Noise*/, TONE_B /* Ctrl */, 0, 0, 0, /* env freq,style */0, 0},
  {"Channel B: tone 400, volume 5", 
   0, 400, 0, 0/*Noise*/, TONE_B /* Ctrl */, 0, 5, 0, /* env freq,style */0, 0},
  {"Channel B: tone 400, volume 10", 
   0, 400, 0, 0/*Noise*/, TONE_B /* Ctrl */, 0, 10, 0, /* env freq,style */0, 0},
  {"Channel B: tone 400, volume 15", 
   0, 400, 0, 0/*Noise*/, TONE_B /* Ctrl */, 0, 15, 0, /* env freq,style */0, 0},

  {"Channel C: tone 400, volume 0", 
   0, 0, 400, 0/*Noise*/, TONE_C /* Ctrl */, 0, 0, 0, /* env freq,style */0, 0},
  {"Channel C: tone 400, volume 5", 
   0, 0, 400, 0/*Noise*/, TONE_C /* Ctrl */, 0, 0, 5, /* env freq,style */0, 0},
  {"Channel C: tone 400, volume 10", 
   0, 0, 400, 0/*Noise*/, TONE_C /* Ctrl */, 0, 0, 10, /* env freq,style */0, 0},
  {"Channel C: tone 400, volume 15", 
   0, 0, 400, 0/*Noise*/, TONE_C /* Ctrl */, 0, 0, 15, /* env freq,style */0, 0},

  {"Channel B: noise period = 0, volume = 15", 
   0, 3000, 0, 0, NOISE_B, 0, 15, 0, 0, 0},
  {"Channel B: noise period = 5, volume = 15", 
   0, 3000, 0, 5, NOISE_B, 0, 15, 0, 0, 0},
  {"Channel B: noise period = 10, volume = 15", 
   0, 3000, 0, 10, NOISE_B, 0, 15, 0, 0, 0},
  {"Channel B: noise period = 15, volume = 15", 
   0, 3000, 0, 15, NOISE_B, 0, 15, 0, 0, 0},
  {"Channel B: noise period = 20, volume = 15", 
   0, 3000, 0, 20, NOISE_B, 0, 15, 0, 0, 0},
  {"Channel B: noise period = 25, volume = 15", 
   0, 3000, 0, 25, NOISE_B, 0, 15, 0, 0, 0},
  {"Channel B: noise period = 31, volume = 15", 
   0, 3000, 0, 31, NOISE_B, 0, 15, 0, 0, 0},

  {"Channel A: tone 400, volume = 15, envelop 0 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 0},
  {"Channel A: tone 400, volume = 15, envelop 1 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 1},
  {"Channel A: tone 400, volume = 15, envelop 2 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 2},
  {"Channel A: tone 400, volume = 15, envelop 3 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 3},
  {"Channel A: tone 400, volume = 15, envelop 4 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 4},
  {"Channel A: tone 400, volume = 15, envelop 5 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 5},
  {"Channel A: tone 400, volume = 15, envelop 6 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 6},
  {"Channel A: tone 400, volume = 15, envelop 7 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 7},
  {"Channel A: tone 400, volume = 15, envelop 8 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 8},
  {"Channel A: tone 400, volume = 15, envelop 9 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 9},
  {"Channel A: tone 400, volume = 15, envelop 10 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 10},
  {"Channel A: tone 400, volume = 15, envelop 11 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 11},
  {"Channel A: tone 400, volume = 15, envelop 12 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 12},
  {"Channel A: tone 400, volume = 15, envelop 13 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 13},
  {"Channel A: tone 400, volume = 15, envelop 14 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 400, 14},
  {"Channel A: tone 400, volume = 15, envelop 15 freq 4000", 
   400, 0, 0, 0, TONE_A, 15 | 0x10, 0, 0, 4000, 15},

  {NULL, 0,0,0,0,0,0,0,0,0,0}
};


void Test()
{ 
  if ((test * sizeof(struct test_t)) >= sizeof (testcases)) {
    printf ("Incorrect test number - %d\n", test);
    exit (1);
  }
  
  while (testcases[test].name != NULL)
    {
      printf ("Test %d: %s\n", test, testcases[test].name);
      gen_sound (testcases[test].tonea, testcases[test].toneb, 
		 testcases[test].tonec, testcases[test].noise,
		 testcases[test].control, testcases[test].vola,
		 testcases[test].volb, testcases[test].volc,
		 testcases[test].envfreq, testcases[test].envstyle);
      test++;
    }
  
  printf ("Exit from Test()\n");
}
