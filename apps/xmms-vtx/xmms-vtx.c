/*  VTXplugin - VTX player for XMMS
 *
 *  Copyright (C) 2002-2004 Sashnov Alexander
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

#include <xmms/plugin.h>
#include <xmms/configfile.h>
#include <xmms/util.h>

#include "xmms-vtx.h"
#include "ayemu.h"


#define SNDBUFSIZE 1024
char sndbuf[SNDBUFSIZE];

gboolean going = FALSE;
gboolean audio_error = FALSE;
pthread_t play_thread;

int seek_to;

int freq = 44100;
int chans = 2;

// current frame playing
size_t pos = 0;

/* NOTE: if you change 'bits' you also need change constant FMT_S16_NE
 * to more appropriate in line
 *
 * (vtx_ip.output->open_audio (FMT_S16_NE, * freq, chans) == 0)
 * in function vtx_play_file()
 *
 * and in vtx_ip.add_vis_pcm () function call.
 */
const int bits = 16;		

ayemu_ay_t ay;
ayemu_vtx_t *vtx;
int end;

static InputPlugin vtx_ip = {
  NULL,				/* Filed in by xmms */
  NULL,				/* Filed in by xmms */
  NULL,				/* Description */
  NULL,				/* vtx_init */
  vtx_about,			/* show the about dialog */
  vtx_config,			/* show the configure dialog */
  vtx_is_our_file,		/* return 1 if the plugin can handle this file */
  NULL,				/* scandir, example is cdaudio plugin */
  vtx_play_file,
  vtx_stop,
  vtx_pause,
  vtx_seek,
  NULL,				/* set the equalizer  */
  vtx_get_time,
  NULL,				/* get_volume */
  NULL,				/* set volume */
  NULL,				/* clean up, called when xmms exit */
  NULL,				/* get_vis_type (DO NOT USE!) */
  NULL,				/* add_vis_pcm */
  NULL,				/* set info */
  NULL,				/* set info text */
  vtx_get_song_info,		/* grab the title string */
  vtx_file_info,		/* show the file info dialog */
  NULL				/* OutputPlugin output. Filled by xmms. */
};

/* called from xmms for plug */
InputPlugin *
get_iplugin_info (void)
{
  vtx_ip.description = g_strdup_printf (_("Vortex file Player %s"), VERSION);
  return &vtx_ip;
}


/* Return 1 if the plugin can handle the file */
int
vtx_is_our_file (char *filename)
{
  char *p;

  /* if file extension not ".vtx" return FALSE */
  for (p = filename; *p != 0; p++);
  p -= 4;
  if (strcmp (p, ".vtx"))
    return 0;
  
  /* not check either file begins from ay or ym string for work faster */
  if (0) {
    FILE *fp;
    char buf[2];
    
    fp = fopen (filename, "rb");
    fread (buf, 2, 1, fp);
    fclose (fp);
    return (!strncasecmp (buf, "ay", 2) || !strncasecmp (buf, "ym", 2));
  }
  return 1;
}


/* sound playing thread, runing by vtx_play_file() */
void *
play_loop (void *args)
{
  void *stream;	     /* pointer to current position in sound buffer */
  ayemu_ay_reg_frame_t regs;
  int need;
  int left; /* how many sound frames can play with current AY register frame */
  int donow;
  int rate;

  pos = 0;
  left = 0;
  rate = chans * (bits / 8);

  while (going && ! end)
    {
      /* fill sound buffer */
      stream = sndbuf;
      for (need = SNDBUFSIZE / rate ; need > 0 ; need -= donow) {
	if (left > 0) {
	  /* use current AY register frame */
	  donow = (need > left) ? left : need;
	  left -= donow;
	  stream = ayemu_gen_sound (&ay, (char *)stream, donow * rate);
	}
	else {			
	  /* get next AY register frame */
	  if (pos++ > vtx->frames) {
	    end = 1;
	    donow = need;
	    memset (stream, 0, donow * rate);
	  }
	  else {
	    ayemu_vtx_getframe(vtx, pos, regs);
	    left = freq / vtx->playerFreq;
	    ayemu_set_regs (&ay, regs);
	    donow = 0;
	  }
	}
      }

      vtx_ip.add_vis_pcm (vtx_ip.output->written_time (), FMT_S16_NE,
			  chans , SNDBUFSIZE, sndbuf);
      while (vtx_ip.output->buffer_free () < SNDBUFSIZE && going
	     && seek_to == -1)
	xmms_usleep (10000);
    
      if (going && seek_to == -1)
	vtx_ip.output->write_audio (sndbuf, SNDBUFSIZE);
    
      if (end) {
	vtx_ip.output->buffer_free ();
	vtx_ip.output->buffer_free ();
      }
    
      /* jump to time in seek_to (in seconds) */
      if (seek_to != -1) {
	/* (time in sec) * 50 = offset in AY register data frames */
	pos = seek_to * 50;
	vtx_ip.output->flush (seek_to * 1000);
	seek_to = -1;
      }
    }

  xmms_usleep (10000);

  /* close sound and release vtx file must be done in vtx_stop() */
  pthread_exit (NULL);
}


void vtx_play_file (char *filename)
{
  gchar *buf;

  memset (&ay, 0, sizeof(ay));

  vtx = ayemu_vtx_load_from_file(filename);

  if (!vtx) {
    printf ("error open file: %s\n", filename);
  }
  else
    {
      ayemu_init(&ay);
      ayemu_set_chip_type(&ay, vtx->chiptype, NULL);
      ayemu_set_chip_freq(&ay, vtx->chipFreq);
      ayemu_set_stereo(&ay, vtx->stereo, NULL);

      audio_error = FALSE;
      if (vtx_ip.output->open_audio (FMT_S16_NE, freq, chans) == 0)
	{
	  fprintf (stderr, "libvtx: output audio error!\n");
	  audio_error = TRUE;
	  going = FALSE;
	  return;
	}

      end = 0;
      seek_to = -1;

      if ((buf = g_malloc (2048)) != NULL)
	{
	  snprintf (buf, 2048, "%s - %s", vtx->author, vtx->title);

	  vtx_ip.set_info (buf, vtx->regdata_size / 14 * 1000 / 50,
			   14 * 50 * 8, freq, bits / 8);
	  g_free (buf);
	}
      going = TRUE;
      pthread_create (&play_thread, NULL, play_loop, NULL);
    }
}


void
vtx_stop (void)
{
  if (going)
    {
      going = FALSE;
      pthread_join (play_thread, NULL);
      vtx_ip.output->close_audio ();
      ayemu_vtx_free (vtx);
    }
}


/* seek to specified number of seconds */
void
vtx_seek (int time)
{
  if (time * 50 < vtx->regdata_size / 14) {
    end = 0;
    seek_to = time; 
    /* wait for affect changes in parallel thread */
    while (seek_to != -1)
      xmms_usleep (10000);
  }
}


/* Pause or unpause */
void
vtx_pause (short p)
{
  vtx_ip.output->pause (p);
}


/* Get the time, usually returns the output plugins output time */
int
vtx_get_time (void)
{
  if (audio_error)
    return -2;
  if (!going || (end && !vtx_ip.output->buffer_playing ()))
    return -1;
  return vtx_ip.output->output_time ();
}


/* Function to grab the title string */
void
vtx_get_song_info (char *filename, char **title, int *length)
{
  ayemu_vtx_t *tmp;

  (*length) = -1;
  (*title) = NULL;

  tmp = ayemu_vtx_header_from_file(filename);

  if (tmp) {
    *length = tmp->regdata_size / 14 * 1000 / 50; /* lenght in miliseconds */

    if ((*title = g_malloc (2048)) != NULL)
      snprintf(*title, 2048, "%s - %s", tmp->author, tmp->title);

    ayemu_vtx_free (tmp);
  }
}
