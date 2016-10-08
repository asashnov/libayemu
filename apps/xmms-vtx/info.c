#include <xmms/plugin.h>
#include <xmms/configfile.h>
#include <xmms/util.h>

#include "ayemu.h"
#include "xmms-vtx.h"

void vtx_file_info(char *filename)
{
  static GtkWidget *box;
  ayemu_vtx_t *vtx;

  vtx = ayemu_vtx_header_from_file(filename);

  if (!vtx)
    {
      fprintf(stderr, "Can't open file %s\n", filename);
      return;
    }
  else
    {
      char head[1024];
      char body[8192];

      sprintf(head, "Detalis about %s", filename);

      snprintf(body, sizeof(body),
	       " Title: %s\n Author: %s\n From: %s\n Comment: %s\n Year: %d\n",
	       vtx->title, vtx->author, vtx->from, vtx->comment, vtx->year);

      box = xmms_show_message (head,
			       body,
			       _("Ok"), FALSE, NULL, NULL);
      
      ayemu_vtx_free(vtx);
    }
  
  gtk_signal_connect (GTK_OBJECT (box), "destroy", gtk_widget_destroyed,
		      &box);
}
