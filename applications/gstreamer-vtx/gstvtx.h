/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __GST_VTX_H__
#define __GST_VTX_H__

#include <gst/gst.h>

G_BEGIN_DECLS
        
#define GST_TYPE_VTX \
  (gst_vtx_get_type())
  
#define GST_VTX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VTX,GstVtx))
#define GST_VTX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VTX,GstVtxClass))
#define GST_IS_VTX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VTX))
#define GST_IS_VTX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VTX))
  
struct _GstVtx {
  GstElement  element;

  GstPad     *sinkpad;
  GstPad     *srcpad;

  /* properties */
  const gchar *songname;
  gboolean     reverb;
  gint         reverb_depth;
  gint         reverb_delay;
  gboolean     megabass;
  gint         megabass_amount;
  gint         megabass_range;
  gboolean     surround;
  gint         surround_depth;
  gint         surround_delay;
  gboolean     noise_reduction;
  gboolean     _16bit;
  gboolean     oversamp;
  gint         channel;
  gint         frequency;

  /* state */
  GstBuffer   *buffer;

  gint32       read_bytes;
  gint32       read_samples;

  gint64       seek_at;      /* pending seek, or -1                 */
  gint64       song_size;    /* size of the raw song data in bytes  */
  gint64       song_length;  /* duration of the song in nanoseconds */
  gint64       offset;       /* current position in samples         */
  gint64       timestamp;

  ayemu_ay_t   ay;
  ayemu_vtx_t  *mVtxFile;
  size_t        cur_frame_pos;
};

struct _GstVtxClass {
  GstElementClass parent_class;
};

typedef struct _GstVtx GstVtx;
typedef struct _GstVtxClass GstVtxClass;

GType gst_vtx_get_type (void);

G_END_DECLS

#endif /* __GST_VTX_H__ */
