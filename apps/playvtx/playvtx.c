/*  VTXplugin - VTX player for XMMS
 *
 *  Copyright (C) 2002-2003 Sashnov Alexander
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/soundcard.h>

#include "ayemu.h"
#include "config.h"

static const int DEBUG = 0;

#define DEVICE_NAME "/dev/dsp"

struct option options[] = {
  { "quiet",   no_argument, NULL, 'q'},
  { "rand",    no_argument, NULL, 'Z'},
  { "stdout",  no_argument, NULL, 's'},
  { "verbose", no_argument, NULL, 'v'},
  { "version", no_argument, NULL, 'V'},
  { "usage",   no_argument, NULL, 'u'},
  { "help",    no_argument, NULL, 'h'},
  { 0, 0, 0, 0}
};

char short_opts[] = "+qZsvuh";

int qflag = 0;  // quite plating
int Zflag = 0;  // random list
int sflag = 0;  // to stdout
int vflag = 0;  // verbose

ayemu_ay_t ay;
ayemu_ay_reg_frame_t regs;

void *audio_buf;
int audio_bufsize;
int audio_fd;

int  freq = 44100;
int  chans = 2;
int  bits = 16;

void usage ()
{
    printf(
	   "AY/YM VTX format player.\n"
	   "It uses libayemu (http://libayemu.sourceforge.net)\n"
	   "THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! USE AT YOUR OWN RISK!\n\n"
	   "Usage: playvtx [option(s)] files...\n"
	   "  -q --quiet\tquiet (don't print title)\n"
	   "  -Z --random\tshuffle play\n"
	   "  -s --stdout\twrite to stdout\n"
	   "  -v --verbose\tincrease verbosity level\n"
	   "  --version\tshow programm version\n"
	   "  -h --help\n"
	   "  -u --usage\tthis help\n"
	   );
}

void init_oss()
{
  if ((audio_fd = open(DEVICE_NAME, O_WRONLY, 0)) == -1) {
    fprintf (stderr,
        "Unable to initialize OSS sound system: unable to open /dev/dsp\n"
        "\n"
        "Probably you are running a modern Linux with ALSA or PulseAudio.\n"
        "\n"
        "On systems with PulseAudio, such as Ubuntu, run with:\n"
        "  $ padsp playvtx music_sample/secret.vtx \n"
        "\n"
        "On systems with ALSA use alsa-oss wrapper:\n"
        "  $ aoss playvtx music_sample/secret.vtx \n"
        );
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

  exit(1);
}

void play (const char *filename)
{
  ayemu_vtx_t *vtx;
  int len;

  vtx = ayemu_vtx_load_from_file(filename);
  if (!vtx) return;

  if (!qflag)
    printf(" Title: %s\n Author: %s\n From: %s\n Comment: %s\n Year: %d\n",
	   vtx->title, vtx->author, vtx->from, vtx->comment, vtx->year);
  
  int audio_bufsize = freq * chans * (bits >> 3) / vtx->playerFreq;
  if ((audio_buf = malloc (audio_bufsize)) == NULL) {
    fprintf (stderr, "Can't allocate sound buffer\n");
    goto free_vtx;
  }

  ayemu_reset(&ay);
  ayemu_set_chip_type(&ay, vtx->chiptype, NULL);
  ayemu_set_chip_freq(&ay, vtx->chipFreq);
  ayemu_set_stereo(&ay, vtx->stereo, NULL);

  size_t pos = 0;

  while (pos++ < vtx->frames) {
    ayemu_vtx_getframe (vtx, pos, regs);
    ayemu_set_regs (&ay, regs);
    ayemu_gen_sound (&ay, audio_buf, audio_bufsize);
    if ((len = write(audio_fd, audio_buf, audio_bufsize)) == -1) {
      fprintf (stderr, "Error writting to sound device, break.\n");
      break;
    }
  }

 free_vtx:
  ayemu_vtx_free(vtx);
}


int main (int argc, char **argv)
{
  int index;
  int c;
  int option_index = 0;
	   
  opterr = 0;
     
  while ((c = getopt_long (argc, argv, short_opts, options, &option_index)) != -1)
    switch (c)
      {
      case 'q':
	qflag = 1;
	break;
      case 'Z':
	Zflag = 1;
	break;
      case 's':
	sflag = 1;
	break;
      case 'v':
	vflag = 1;
	break;
      case 'V':
	printf ("playvtx %s\n", VERSION);
	exit (0);
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
     
  if (DEBUG)
    printf ("qflag = %d, Zflag = %d, sflag = %d, vflag = %d\n", 
	    qflag, Zflag, sflag, vflag);
     
  if (Zflag)
    printf ("The -Z flag is not implemented yet, sorry\n");

  if (optind == argc) {
    fprintf (stderr, "No files to play specified, see %s --usage.\n",
	     argv[0]);
    exit (1);
  }

  init_oss();
  if (DEBUG) 
    printf ("OSS sound system initialization success: bits=%d, chans=%d, freq=%d\n",
	    bits, chans, freq);

  ayemu_init(&ay);
  ayemu_set_sound_format(&ay, freq, chans, bits);

  for (index = optind; index < argc; index++) {
    printf ("\nPlaying file %s\n", argv[index]);
    play (argv[index]);
  }

  return 0;
}
