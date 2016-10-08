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

/* 
   Code based on modplugxmms
   XMMS plugin:
     Kenton Varda <temporal@gauge3d.org>
   Sound Engine:
     Olivier Lapicque <olivierl@jps.net>  
*/

/*
 * SECTION:element-vtx
 * 
 * <refsect2>
 * <para>
 * Vtx uses the <ulink url="http://libayemu.sourceforge.net/">libayemu</ulink>
 * library to emulate ZX Spectrum 128 AY/YM music co-processor.
 * </para>
 * <title>Example pipeline</title>
 * <programlisting>
 * gst-launch -v filesrc location=samples/spf21_00.vtx ! vtx ! audioconvert ! alsasink
 * </programlisting>
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libayemu/ayemu.h"

#include "gstvtx.h"

#include <gst/gst.h>
#include <stdlib.h>
#include <cstring>
#include <gst/audio/audio.h>
#include <gst/gsttypefind.h>

GST_DEBUG_CATEGORY_STATIC (vtx_debug);
#define GST_CAT_DEFAULT vtx_debug

/* elementfactory information */
GstElementDetails vtx_details = {
  (char*)"Vtx",
  (char*)"Codec/Decoder/Audio",
  (char*)"AY Vtx file decoder based on libayemu",
  (char*)"Alexander Sashnov <sashnov@ngs.ru>"
};

enum
{
  ARG_0,
  ARG_SONGNAME,
  ARG_REVERB,
  ARG_REVERB_DEPTH,
  ARG_REVERB_DELAY,
  ARG_MEGABASS,
  ARG_MEGABASS_AMOUNT,
  ARG_MEGABASS_RANGE,
  ARG_NOISE_REDUCTION,
  ARG_SURROUND,
  ARG_SURROUND_DEPTH,
  ARG_SURROUND_DELAY,
  ARG_OVERSAMP
};

#define DEFAULT_REVERB           FALSE
#define DEFAULT_REVERB_DEPTH     30
#define DEFAULT_REVERB_DELAY     100
#define DEFAULT_MEGABASS         FALSE
#define DEFAULT_MEGABASS_AMOUNT  40
#define DEFAULT_MEGABASS_RANGE   30
#define DEFAULT_SURROUND         TRUE
#define DEFAULT_SURROUND_DEPTH   20
#define DEFAULT_SURROUND_DELAY   20
#define DEFAULT_OVERSAMP         TRUE
#define DEFAULT_NOISE_REDUCTION  TRUE

#define SRC_CAPS \
  "audio/x-raw-int,"                              \
  " endianness = (int) BYTE_ORDER,"               \
  " signed = (boolean) true,"                     \
  " width = (int) 16,"                            \
  " depth = (int) 16,"                            \
  " rate = (int) { 8000, 11025, 22050, 44100 },"  \
  " channels = (int) 2; "                         \
  "audio/x-raw-int,"                              \
  " endianness = (int) BYTE_ORDER,"               \
  " signed = (boolean) false,"                    \
  " width = (int) 8,"                             \
  " depth = (int) 8,"                             \
  " rate = (int) { 8000, 11025, 22050, 44100 }, " \
  " channels = (int) [ 1, 2 ]"

static GstStaticPadTemplate vtx_src_template_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS));

static GstStaticPadTemplate vtx_sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-vtx"));

static void gst_vtx_dispose (GObject * object);
static void gst_vtx_set_property (GObject * object,
    guint id, const GValue * value, GParamSpec * pspec);
static void gst_vtx_get_property (GObject * object,
    guint id, GValue * value, GParamSpec * pspec);

static void gst_vtx_fixate (GstPad * pad, GstCaps * caps);
static const GstQueryType *gst_vtx_get_query_types (GstPad * pad);
static gboolean gst_vtx_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_vtx_src_query (GstPad * pad, GstQuery * query);
static GstStateChangeReturn gst_vtx_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_vtx_sinkpad_activate (GstPad * pad);
static gboolean gst_vtx_sinkpad_activate_pull (GstPad * pad,
    gboolean active);
static void gst_vtx_loop (GstVtx * element);

GST_BOILERPLATE (GstVtx, gst_vtx, GstElement, GST_TYPE_ELEMENT);

static void
gst_vtx_base_init (gpointer g_class)
{
  GST_DEBUG ("vtx gst_vtx_base_init %p", g_class);

  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&vtx_sink_template_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&vtx_src_template_factory));
  gst_element_class_set_details (element_class, &vtx_details);

  GST_DEBUG_CATEGORY_INIT (vtx_debug, "vtx", 0, "Vtx element");
}

static void
gst_vtx_class_init (GstVtxClass * klass)
{
  GST_DEBUG ("VTX gst_vtx_class_init %p", klass);

  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_vtx_set_property;
  gobject_class->get_property = gst_vtx_get_property;
  gobject_class->dispose = gst_vtx_dispose;

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_SONGNAME,
      g_param_spec_string ("songname", "Songname", "The song name",
          NULL, G_PARAM_READABLE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_REVERB,
      g_param_spec_boolean ("reverb", "reverb", "Reverb",
          DEFAULT_REVERB, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_REVERB_DEPTH,
      g_param_spec_int ("reverb-depth", "reverb depth", "Reverb depth",
          0, 100, DEFAULT_REVERB_DEPTH, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_REVERB_DELAY,
      g_param_spec_int ("reverb-delay", "reverb delay", "Reverb delay",
          0, 200, DEFAULT_REVERB_DELAY, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MEGABASS,
      g_param_spec_boolean ("megabass", "megabass", "Megabass",
          DEFAULT_MEGABASS, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MEGABASS_AMOUNT,
      g_param_spec_int ("megabass-amount", "megabass amount", "Megabass amount",
          0, 100, DEFAULT_MEGABASS_AMOUNT, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MEGABASS_RANGE,
      g_param_spec_int ("megabass-range", "megabass range", "Megabass range",
          0, 100, DEFAULT_MEGABASS_RANGE, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_SURROUND,
      g_param_spec_boolean ("surround", "surround", "Surround",
          DEFAULT_SURROUND, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_SURROUND_DEPTH,
      g_param_spec_int ("surround-depth", "surround depth", "Surround depth",
          0, 100, DEFAULT_SURROUND_DEPTH, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_SURROUND_DELAY,
      g_param_spec_int ("surround-delay", "surround delay", "Surround delay",
          0, 40, DEFAULT_SURROUND_DELAY, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_OVERSAMP,
      g_param_spec_boolean ("oversamp", "oversamp", "oversamp",
          DEFAULT_OVERSAMP, (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_NOISE_REDUCTION,
      g_param_spec_boolean ("noise-reduction", "noise reduction",
          "noise reduction", DEFAULT_NOISE_REDUCTION,
          (GParamFlags) G_PARAM_READWRITE));

  gstelement_class->change_state = gst_vtx_change_state;
}

static void
gst_vtx_init (GstVtx * vtx, GstVtxClass * klass)
{
  GST_INFO_OBJECT (vtx, "vtx gst_vtx_init");

  /* create the sink and src pads */
  vtx->sinkpad =
      gst_pad_new_from_static_template (&vtx_sink_template_factory, "sink");
  gst_pad_set_activate_function (vtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_vtx_sinkpad_activate));
  gst_pad_set_activatepull_function (vtx->sinkpad,
      GST_DEBUG_FUNCPTR (gst_vtx_sinkpad_activate_pull));
  gst_element_add_pad (GST_ELEMENT (vtx), vtx->sinkpad);

  vtx->srcpad =
      gst_pad_new_from_static_template (&vtx_src_template_factory, "src");
  gst_pad_set_fixatecaps_function (vtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_vtx_fixate));
  gst_pad_set_event_function (vtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_vtx_src_event));
  gst_pad_set_query_function (vtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_vtx_src_query));
  gst_pad_set_query_type_function (vtx->srcpad,
      GST_DEBUG_FUNCPTR (gst_vtx_get_query_types));
  gst_element_add_pad (GST_ELEMENT (vtx), vtx->srcpad);

  vtx->reverb = DEFAULT_REVERB;
  vtx->reverb_depth = DEFAULT_REVERB_DEPTH;
  vtx->reverb_delay = DEFAULT_REVERB_DELAY;
  vtx->megabass = DEFAULT_MEGABASS;
  vtx->megabass_amount = DEFAULT_MEGABASS_AMOUNT;
  vtx->megabass_range = DEFAULT_MEGABASS_RANGE;
  vtx->surround = DEFAULT_SURROUND;
  vtx->surround_depth = DEFAULT_SURROUND_DEPTH;
  vtx->surround_delay = DEFAULT_SURROUND_DELAY;
  vtx->oversamp = DEFAULT_OVERSAMP;
  vtx->noise_reduction = DEFAULT_NOISE_REDUCTION;

  vtx->_16bit = TRUE;
  vtx->channel = 2;
  vtx->frequency = 44100;
}


static void
gst_vtx_dispose (GObject * object)
{
  GST_DEBUG ("gst_vtx_dispose %p", object);

  GstVtx *vtx = GST_VTX (object);

  G_OBJECT_CLASS (parent_class)->dispose (object);

  if (vtx->buffer) {
    gst_buffer_unref (vtx->buffer);
    vtx->buffer = NULL;
  }
}

static const GstQueryType *
gst_vtx_get_query_types (GstPad * pad)
{
  static const GstQueryType gst_vtx_src_query_types[] = {
    GST_QUERY_DURATION,
    GST_QUERY_POSITION,
    (GstQueryType) 0
  };

  GST_DEBUG ("VTX  gst_vtx_get_query_types %p", pad);

  return gst_vtx_src_query_types;
}


static gboolean
gst_vtx_src_query (GstPad * pad, GstQuery * query)
{
  GstVtx *vtx;
  gboolean res = FALSE;

  GST_DEBUG ("gst_vtx_get_query_types %p", pad);

  vtx = GST_VTX (gst_pad_get_parent (pad));

  if (!vtx->mVtxFile)
    goto done;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);
      if (format == GST_FORMAT_TIME) {
        gst_query_set_duration (query, format, vtx->song_length);
        res = TRUE;
      }
    }
      break;
    case GST_QUERY_POSITION:
    {
      GstFormat format;

      gst_query_parse_position (query, &format, NULL);
      if (format == GST_FORMAT_TIME) {
        gint64 pos;

        pos = (vtx->song_length * vtx->cur_frame_pos);
        pos /= vtx->mVtxFile->frames;
        gst_query_set_position (query, format, pos);
        res = TRUE;
      }
    }
      break;
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

done:
  gst_object_unref (vtx);

  return res;
}

static gboolean
gst_vtx_src_event (GstPad * pad, GstEvent * event)
{
  GstVtx *vtx;
  gboolean res = FALSE;


  GST_DEBUG ("VTX  gst_vtx_src_event %p", pad);

  vtx = GST_VTX (gst_pad_get_parent (pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    {
      gdouble rate;
      GstFormat format;
      GstSeekFlags flags;
      GstSeekType cur_type, stop_type;
      gboolean flush;
      gint64 cur, stop;
      guint64 timestamp;

      if (vtx->frequency == 0) {
        GST_DEBUG_OBJECT (vtx, "no song loaded yet");
        break;
      }

      timestamp = gst_util_uint64_scale_int (vtx->offset, GST_SECOND,
          vtx->frequency);

      gst_event_parse_seek (event, &rate, &format, &flags,
          &cur_type, &cur, &stop_type, &stop);

      if (format != GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (vtx, "seeking is only supported in TIME format");
        gst_event_unref (event);
        break;
      }

      /* FIXME: we should be using GstSegment for all this */
      if (cur_type != GST_SEEK_TYPE_SET || stop_type != GST_SEEK_TYPE_NONE) {
        GST_DEBUG_OBJECT (vtx, "unsupported seek type");
        gst_event_unref (event);
        break;
      }

      if (stop_type == GST_SEEK_TYPE_NONE)
        stop = GST_CLOCK_TIME_NONE;

      cur = CLAMP (cur, 0, vtx->song_length);

      GST_DEBUG_OBJECT (vtx, "seek to %" GST_TIME_FORMAT,
          GST_TIME_ARGS ((guint64) cur));

      vtx->seek_at = cur;

      flush = ((flags & GST_SEEK_FLAG_FLUSH) == GST_SEEK_FLAG_FLUSH);

      if (flush) {
        gst_pad_push_event (vtx->srcpad, gst_event_new_flush_start ());
      } else {
        gst_pad_stop_task (vtx->sinkpad);
      }

      GST_PAD_STREAM_LOCK (vtx->sinkpad);

      if (flags & GST_SEEK_FLAG_SEGMENT) {
        gst_element_post_message (GST_ELEMENT (vtx),
            gst_message_new_segment_start (GST_OBJECT (vtx), format, cur));
      }
      if (stop == -1 && vtx->song_length > 0)
        stop = vtx->song_length;

      if (flush) {
        gst_pad_push_event (vtx->srcpad, gst_event_new_flush_stop ());
      }

      GST_LOG_OBJECT (vtx, "sending newsegment from %" GST_TIME_FORMAT "-%"
          GST_TIME_FORMAT ", pos=%" GST_TIME_FORMAT,
          GST_TIME_ARGS ((guint64) cur), GST_TIME_ARGS ((guint64) stop),
          GST_TIME_ARGS ((guint64) cur));

      gst_pad_push_event (vtx->srcpad,
          gst_event_new_new_segment (FALSE, rate,
              GST_FORMAT_TIME, cur, stop, cur));

      vtx->offset =
          gst_util_uint64_scale_int (cur, vtx->frequency, GST_SECOND);

      gst_pad_start_task (vtx->sinkpad,
          (GstTaskFunction) gst_vtx_loop, vtx);

      GST_PAD_STREAM_UNLOCK (vtx->sinkpad);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (vtx);
  return res;
}

static void
gst_vtx_fixate (GstPad * pad, GstCaps * caps)
{
  GST_DEBUG ("MY  gst_vtx_fixate %p", pad);

  GstStructure *structure;

  structure = gst_caps_get_structure (caps, 0);
  if (gst_structure_fixate_field_nearest_int (structure, "rate", 44100))
    return;
  if (gst_structure_fixate_field_nearest_int (structure, "channels", 2))
    return;
}

static gboolean
gst_vtx_load_song (GstVtx * vtx)
{
  GstCaps *newcaps, *othercaps;
  GstStructure *structure;
  gint depth;

  vtx->mVtxFile = ayemu_vtx_load((const char*)GST_BUFFER_DATA (vtx->buffer),
				     (size_t)vtx->song_size);
  if (!vtx->mVtxFile) {
    GST_ELEMENT_ERROR (vtx, STREAM, DECODE, (NULL),
        ("Unable to load song"));
    return FALSE;
  }

  ayemu_init(&vtx->ay);
  ayemu_reset(&vtx->ay);
  ayemu_set_chip_type(&vtx->ay, vtx->mVtxFile->chiptype, NULL);
  ayemu_set_chip_freq(&vtx->ay, vtx->mVtxFile->chipFreq);
  //  ayemu_set_stereo(&vtx->ay, vtx->mVtxFile->stereo, NULL);

  GST_DEBUG_OBJECT (vtx, "Loading song");

  /* negotiate srcpad caps */
  othercaps = gst_pad_get_allowed_caps (vtx->srcpad);
  newcaps = gst_caps_copy_nth (othercaps, 0);
  gst_caps_unref (othercaps);
  gst_pad_fixate_caps (vtx->srcpad, newcaps);
  gst_pad_set_caps (vtx->srcpad, newcaps);

  /* set up vtx to output the negotiated format */
  structure = gst_caps_get_structure (newcaps, 0);
  gst_structure_get_int (structure, "depth", &depth);
  vtx->_16bit = (depth == 16);
  gst_structure_get_int (structure, "channels", &vtx->channel);
  gst_structure_get_int (structure, "rate", &vtx->frequency);

  vtx->read_samples = vtx->frequency / vtx->mVtxFile->playerFreq;
  vtx->read_bytes = vtx->read_samples * vtx->channel * depth / 8;

  ayemu_set_sound_format (&vtx->ay,
      vtx->frequency, vtx->channel, depth);

  vtx->song_length = (vtx->mVtxFile->frames / vtx->mVtxFile->playerFreq) * GST_SECOND;
  vtx->seek_at = -1;
  vtx->cur_frame_pos = 0;

  GST_INFO_OBJECT (vtx, "Song length: %" GST_TIME_FORMAT,
      GST_TIME_ARGS ((guint64) vtx->song_length));

  return TRUE;
}

static gboolean
gst_vtx_sinkpad_activate (GstPad * pad)
{
  GST_DEBUG ("MY  gst_vtx_sinkpad_activate %p", pad);

  if (!gst_pad_check_pull_range (pad)) {
    GST_DEBUG ("MY  no pull mode, ha-ha :-)");
    return FALSE;
  }


  GST_DEBUG ("MY  Gm, pull mode available, return result of gst_pad_activate_pull");
  return gst_pad_activate_pull (pad, TRUE);
}

static gboolean
gst_vtx_sinkpad_activate_pull (GstPad * pad, gboolean active)
{
  GstVtx *vtx = GST_VTX (GST_OBJECT_PARENT (pad));

  GST_DEBUG ("gst_vtx_sinkpad_activate_pull %p", pad);

  if (active) {
    return gst_pad_start_task (pad, (GstTaskFunction) gst_vtx_loop,
        vtx);
  } else {
    return gst_pad_stop_task (pad);
  }
}

static gboolean
gst_vtx_get_upstream_size (GstVtx * vtx, gint64 * length)
{
  GstFormat format = GST_FORMAT_BYTES;
  gboolean res = FALSE;
  GstPad *peer;

  peer = gst_pad_get_peer (vtx->sinkpad);
  if (peer == NULL)
    return FALSE;

  if (gst_pad_query_duration (peer, &format, length) && *length >= 0) {
    GST_DEBUG ("MY  gst_pad_query_duration return true");
    res = TRUE;
  }

  GST_DEBUG ("MY  gst_pad_query_duration return false %p", vtx);

  gst_object_unref (peer);
  return res;
}

static void
gst_vtx_loop (GstVtx * vtx)
{
  GstFlowReturn flow;
  GstBuffer *out = NULL;

  g_assert (GST_IS_VTX (vtx));

  GST_DEBUG ("gst_vtx_loop %p", vtx);


  /* first, get the size of the song */
  if (!vtx->song_size) {
    if (!gst_vtx_get_upstream_size (vtx, &vtx->song_size)) {
      GST_ELEMENT_ERROR (vtx, STREAM, DECODE, (NULL),
          ("Unable to load song"));
      goto pause;
    }

    if (vtx->buffer) {
      gst_buffer_unref (vtx->buffer);
    }
    vtx->buffer = gst_buffer_new_and_alloc (vtx->song_size);
    vtx->offset = 0;
  }

  /* read in the song data */
  if (!vtx->mVtxFile) {
    GstBuffer *buffer = NULL;
    guint64 read_size = vtx->song_size - vtx->offset;

    if (read_size > 4096)
      read_size = 4096;

    flow =
        gst_pad_pull_range (vtx->sinkpad, vtx->offset, read_size,
        &buffer);
    if (flow != GST_FLOW_OK) {
      GST_ELEMENT_ERROR (vtx, STREAM, DECODE, (NULL),
          ("Unable to load song"));
      goto pause;
    }

    /* GST_LOG_OBJECT (vtx, "Read %u bytes", GST_BUFFER_SIZE (buffer)); */
    g_memmove (GST_BUFFER_DATA (vtx->buffer) + vtx->offset,
        GST_BUFFER_DATA (buffer), GST_BUFFER_SIZE (buffer));
    gst_buffer_unref (buffer);

    vtx->offset += read_size;

    /* actually load it */
    if (vtx->offset == vtx->song_size) {
      GstEvent *newsegment;
      gboolean ok;

      ok = gst_vtx_load_song (vtx);
      gst_buffer_unref (vtx->buffer);
      vtx->buffer = NULL;
      vtx->offset = 0;

      if (!ok) {
        goto pause;
      }

      newsegment = gst_event_new_new_segment (FALSE, 1.0, GST_FORMAT_TIME,
          0, vtx->song_length, 0);
      gst_pad_push_event (vtx->srcpad, newsegment);
    } else {
      /* not fully loaded yet */
      return;
    }
  }

  /* comment from modplug: is it mean for vtx?
   * could move this to gst_vtx_src_event 
   * if libmodplug was definitely thread safe.. */
  if (vtx->seek_at != -1) {
    size_t seek_to_pos;
    gfloat temp;

    temp = (gfloat) vtx->song_length / vtx->seek_at;
    seek_to_pos = (int) (vtx->mVtxFile->frames / temp);

    GST_DEBUG_OBJECT (vtx, "Seeking to frame %d", seek_to_pos);

    vtx->cur_frame_pos = seek_to_pos;
    vtx->seek_at = -1;
  }

  /* read and output a buffer */
  flow = gst_pad_alloc_buffer_and_set_caps (vtx->srcpad,
      GST_BUFFER_OFFSET_NONE, vtx->read_bytes,
      GST_PAD_CAPS (vtx->srcpad), &out);

  if (flow != GST_FLOW_OK) {
    GST_LOG_OBJECT (vtx, "pad alloc flow: %s", gst_flow_get_name (flow));
    goto pause;
  }

  if (vtx->cur_frame_pos >= vtx->mVtxFile->frames)
    goto eos;

  ayemu_ay_reg_frame_t regs;
  ayemu_vtx_getframe (vtx->mVtxFile, vtx->cur_frame_pos++, regs);


  GST_DEBUG_OBJECT (vtx, "before ayemu_set_regs");
  ayemu_set_regs (&vtx->ay, regs);
  GST_DEBUG_OBJECT (vtx, "after ayemu_set_regs");

  ayemu_gen_sound (&vtx->ay, GST_BUFFER_DATA (out), vtx->read_bytes);

  GST_BUFFER_SIZE (out) = vtx->read_bytes;
  GST_BUFFER_DURATION (out) =
      gst_util_uint64_scale_int (vtx->read_samples, GST_SECOND,
      vtx->frequency);
  GST_BUFFER_OFFSET (out) = vtx->offset;
  GST_BUFFER_TIMESTAMP (out) =
      gst_util_uint64_scale_int (vtx->offset, GST_SECOND,
      vtx->frequency);

  vtx->offset += vtx->read_samples;

  flow = gst_pad_push (vtx->srcpad, out);

  if (flow != GST_FLOW_OK) {
    GST_LOG_OBJECT (vtx, "pad push flow: %s", gst_flow_get_name (flow));
    goto pause;
  }

  return;

eos:
  {
    gst_buffer_unref (out);
    GST_INFO_OBJECT (vtx, "EOS");
    gst_pad_push_event (vtx->srcpad, gst_event_new_eos ());
    goto pause;
  }

pause:
  {
    GST_INFO_OBJECT (vtx, "Pausing");
    gst_pad_pause_task (vtx->sinkpad);
  }
}


static GstStateChangeReturn
gst_vtx_change_state (GstElement * element, GstStateChange transition)
{
  GstVtx *vtx;
  GstStateChangeReturn ret;

  GST_DEBUG ("gst_vtx_change_state %p", element);


  vtx = GST_VTX (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      vtx->buffer = NULL;
      vtx->offset = 0;
      vtx->song_size = 0;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (vtx->buffer) {
        gst_buffer_unref (vtx->buffer);
        vtx->buffer = NULL;
      }
      if (vtx->mVtxFile) {
        ayemu_vtx_free(vtx->mVtxFile);
        vtx->mVtxFile = NULL;
      }
      break;
    default:
      break;
  }

  return GST_STATE_CHANGE_SUCCESS;
}


static void
gst_vtx_set_property (GObject * object, guint id, const GValue * value,
    GParamSpec * pspec)
{
  GstVtx *vtx;

  g_return_if_fail (GST_IS_VTX (object));
  vtx = GST_VTX (object);

  switch (id) {
    case ARG_REVERB:
      vtx->reverb = g_value_get_boolean (value);
      break;
    case ARG_REVERB_DEPTH:
      vtx->reverb_depth = g_value_get_int (value);
      break;
    case ARG_REVERB_DELAY:
      vtx->reverb_delay = g_value_get_int (value);
      break;
    case ARG_MEGABASS:
      vtx->megabass = g_value_get_boolean (value);
      break;
    case ARG_MEGABASS_AMOUNT:
      vtx->megabass_amount = g_value_get_int (value);
      break;
    case ARG_MEGABASS_RANGE:
      vtx->megabass_range = g_value_get_int (value);
      break;
    case ARG_NOISE_REDUCTION:
      vtx->noise_reduction = g_value_get_boolean (value);
      break;
    case ARG_SURROUND:
      vtx->surround = g_value_get_boolean (value);
      break;
    case ARG_SURROUND_DEPTH:
      vtx->surround_depth = g_value_get_int (value);
      break;
    case ARG_SURROUND_DELAY:
      vtx->surround_delay = g_value_get_int (value);
      break;
    default:
      break;
  }
}

static void
gst_vtx_get_property (GObject * object, guint id, GValue * value,
    GParamSpec * pspec)
{
  GstVtx *vtx;

  g_return_if_fail (GST_IS_VTX (object));
  vtx = GST_VTX (object);

  switch (id) {
    case ARG_REVERB:
      g_value_set_boolean (value, vtx->reverb);
      break;
    case ARG_REVERB_DEPTH:
      g_value_set_int (value, vtx->reverb_depth);
      break;
    case ARG_REVERB_DELAY:
      g_value_set_int (value, vtx->reverb_delay);
      break;
    case ARG_MEGABASS:
      g_value_set_boolean (value, vtx->megabass);
      break;
    case ARG_MEGABASS_AMOUNT:
      g_value_set_int (value, vtx->megabass_amount);
      break;
    case ARG_MEGABASS_RANGE:
      g_value_set_int (value, vtx->megabass_range);
      break;
    case ARG_SURROUND:
      g_value_set_boolean (value, vtx->surround);
      break;
    case ARG_SURROUND_DEPTH:
      g_value_set_int (value, vtx->surround_depth);
      break;
    case ARG_SURROUND_DELAY:
      g_value_set_int (value, vtx->surround_delay);
      break;
    case ARG_NOISE_REDUCTION:
      g_value_set_boolean (value, vtx->noise_reduction);
      break;
    default:
      break;
  }
}


static void
gst_vtx_typefind_function (GstTypeFind *tf, gpointer unused)
{
  const char *data = (const char *)gst_type_find_peek (tf, 0, 500);
  ayemu_vtx_t *test = 0;

  GST_DEBUG ("gst_vtx_typefind: entry");

  if (data) {
    test = ayemu_vtx_header(data, 500);
    GST_DEBUG ("gst_vtx_typefind: ayemu_vtx_header created: %p", test);

    if (test)
      gst_type_find_suggest (tf, GST_TYPE_FIND_MAXIMUM,
	   gst_caps_new_simple ("audio/x-vtx", NULL));
  }
  if (test)
    ayemu_vtx_free(test);
}



static gboolean
plugin_init (GstPlugin * plugin)
{
  static gchar *exts[] = { (char*)"vtx", NULL };

  GST_DEBUG_CATEGORY_INIT (vtx_debug, "vtx", 0, "Vtx element");

  GST_DEBUG ("vtx_plugin_init: plugin_init entry");

  if (!gst_type_find_register (plugin, "vtx_tune", GST_RANK_PRIMARY,
	gst_vtx_typefind_function, exts,
        gst_caps_new_simple ("audio/x-vtx", NULL),
	NULL, NULL))
    return FALSE;

    GST_DEBUG ("vtx_plugin_init: gst_type_find_register return success");

    gboolean ret = gst_element_register (plugin, "vtx",
					 GST_RANK_PRIMARY, GST_TYPE_VTX);
    if (ret)
      GST_DEBUG ("vtx_plugin_init: gst_element_register return success");
    else 
      GST_DEBUG ("vtx_plugin_init: gst_element_register return failure");

    return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "vtx",
    ".vtx audio decoding",
    plugin_init, VERSION, "LGPL", "vtx", "1.0")
