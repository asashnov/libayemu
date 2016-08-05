#ifndef XMMS_VTX_H
#define XMMS_VTX_H

#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "xmms/plugin.h"


/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)


void vtx_init(void);
void vtx_about(void);
void vtx_config(void);
void vtx_file_info(char *filename);

extern int  vtx_is_our_file(char *filename);
extern void vtx_play_file(char *filename);
extern void vtx_stop(void);
extern void vtx_seek(int time);
extern void vtx_pause(short p);
extern int  vtx_get_time(void);
extern void vtx_get_song_info(char *filename, char **title, int *length);

#endif
