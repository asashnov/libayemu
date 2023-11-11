#include <xmms/plugin.h>
#include <xmms/configfile.h>
#include <xmms/util.h>

#include "xmms-vtx.h"
#include "ayemu.h"

void
vtx_about (void)
{
  static GtkWidget *box;
  box = xmms_show_message (_("About Vortex Player"),
			   _
			   ("Vortex file format player by \n"
			    "Sashnov Alexander <asashnov@rambler.ru>\n"
			    "Founded on original source in_vtx.dll by \n"
			    "Roman Sherbakov <v_soft@microfor.ru>\n"
			    "\n"
			    "Music in vtx format can be found at \n"
			    "http://vtx.microfor.ru/music.htm\n"
			    "and other AY/YM music sites."), 
			   _("Ok"), FALSE, NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (box), "destroy", gtk_widget_destroyed,
		      &box);
}
