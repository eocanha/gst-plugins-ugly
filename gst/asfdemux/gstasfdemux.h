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


#ifndef __ASF_DEMUX_H__
#define __ASF_DEMUX_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

#include "asfheaders.h"

G_BEGIN_DECLS
  
#define GST_TYPE_ASF_DEMUX \
  (gst_asf_demux_get_type())
#define GST_ASF_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ASF_DEMUX,GstASFDemux))
#define GST_ASF_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ASF_DEMUX,GstASFDemuxClass))
#define GST_IS_ASF_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ASF_DEMUX))
#define GST_IS_ASF_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ASF_DEMUX))

GST_DEBUG_CATEGORY_EXTERN (asfdemux_dbg);
#define GST_CAT_DEFAULT asfdemux_dbg

typedef struct _GstASFDemux GstASFDemux;
typedef struct _GstASFDemuxClass GstASFDemuxClass;

typedef struct {
  AsfPayloadExtensionID   id : 16;  /* extension ID; the :16 makes sure the
                                     * struct gets packed into 4 bytes       */
  guint16                 len;      /* save this so we can skip unknown IDs  */
} AsfPayloadExtension;

typedef struct
{
  gboolean        valid;               /* TRUE if structure is valid/filled */

  GstClockTime    start_time;
  GstClockTime    end_time;
  GstClockTime    avg_time_per_frame;
  guint32         data_bitrate;
  guint32         buffer_size;
  guint32         intial_buf_fullness;
  guint32         data_bitrate2;
  guint32         buffer_size2;
  guint32         intial_buf_fullness2;
  guint32         max_obj_size;
  guint32         flags;
  guint16         lang_idx;

  /* may be NULL if there are no extensions; otherwise, terminated by
   * an AsfPayloadExtension record with len 0 */
  AsfPayloadExtension  *payload_extensions;

  /* missing: stream names */
} AsfStreamExtProps;

typedef struct
{
  AsfStreamType      type;

  gboolean           active;  /* if the stream has been activated (pad added) */

  GstPad     *pad;
  guint16     id;
  guint32     frag_offset;
  guint32     sequence;
  guint64     delay;
  guint64     first_pts;
  guint64     last_pts; /* FIXME: remove, not actually evaluated */
  GstBuffer  *payload;

  /* video-only */
  guint64     last_buffer_timestamp;  /* timestamp of last buffer sent out */
  gboolean    is_video;
  gboolean    fps_known;
  GstBuffer  *cache;

  GstCaps    *caps;

  GstTagList *pending_tags;

  gboolean    discont;

  /* for new parsing code */
  GstFlowReturn   last_flow; /* last flow return */
  GArray         *payloads;  /* pending payloads */

  /* extended stream properties (optional) */
  AsfStreamExtProps  ext_props;

} AsfStream;

typedef enum {
  GST_ASF_DEMUX_STATE_HEADER,
  GST_ASF_DEMUX_STATE_DATA
} GstAsfDemuxState;

#define GST_ASF_DEMUX_NUM_VIDEO_PADS   16
#define GST_ASF_DEMUX_NUM_AUDIO_PADS   32
#define GST_ASF_DEMUX_NUM_STREAMS      32
#define GST_ASF_DEMUX_NUM_STREAM_IDS  127

struct _GstASFDemux {
  GstElement 	     element;

  GstPad            *sinkpad;

  GstAdapter        *adapter;
  GstTagList        *taglist;
  GstAsfDemuxState   state;
  
  guint64            index_offset; /* byte offset where index might be, or 0   */
  guint64            data_offset;  /* byte offset where packets start          */
  guint64            data_size;    /* total size of packet data in bytes, or 0 */
  guint64            num_packets;  /* total number of data packets, or 0       */
  guint64            packet;       /* current packet                           */

  /* bitrates are unused at the moment */
  guint32              bitrate[GST_ASF_DEMUX_NUM_STREAM_IDS];

  gchar              **languages;
  guint                num_languages;

  GstCaps             *metadata;         /* metadata, for delayed parsing; one
                                          * structure ('stream-N') per stream */

  GSList              *ext_stream_props; /* for delayed processing (buffers) */
  GSList              *mut_ex_streams;   /* mutually exclusive streams */

  guint32              num_audio_streams;
  guint32              num_video_streams;
  guint32              num_streams;
  AsfStream            stream[GST_ASF_DEMUX_NUM_STREAMS];
  gboolean             activated_streams;

  GstClockTime         first_ts;        /* first timestamp found        */

  guint32              packet_size;
  guint32              timestamp;       /* in milliseconds              */
  guint64              play_time;

  guint64              preroll;         /* FIXME: make into GstClockTime */
  guint64              pts;

  gboolean             seekable;
  gboolean             broadcast;

  GstSegment           segment;          /* configured play segment                 */

  gboolean             need_newsegment;  /* do we need to send a new-segment event? */
  gboolean             segment_running;  /* if we've started the current segment    */
  gboolean             streaming;        /* TRUE if we are operating chain-based    */
  gboolean             push_mode;        /* are we operating in push_mode? */

  /* Descrambler settings */
  guint8               span;
  guint16              ds_packet_size;
  guint16              ds_chunk_size;
  guint16              ds_data_size;

  /* for debugging only */
  gchar               *objpath;

  /* simple index, if available */
  GstClockTime         sidx_interval;    /* interval between entries in ns */
  guint                sidx_num_entries; /* number of index entries        */
  guint32             *sidx_entries;     /* packet number for each entry   */
};

struct _GstASFDemuxClass {
  GstElementClass parent_class;
};

GType           gst_asf_demux_get_type (void);

AsfStream     * gst_asf_demux_get_stream (GstASFDemux * demux, guint16 id);

G_END_DECLS

#endif /* __ASF_DEMUX_H__ */
