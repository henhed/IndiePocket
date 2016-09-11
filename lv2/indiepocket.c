/* Copyright (C) 2016 Henrik Hedelund.

   This file is part of IndiePocket.

   IndiePocket is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   IndiePocket is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with IndiePocket.  If not, see <http://www.gnu.org/licenses/>. */

/* Standard headers.  */
#include <stdlib.h>
#include <string.h>

/* LV2 headers.  */
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>

/* IndiePocket headers.  */
#include "../pckt/kit_factory.h"
#include "../pckt/kit.h"
#include "../pckt/sound.h"
#include "../pckt/drum.h"
#include "indiepocket_io.h"

#define MAX_NUM_SOUNDS 32

/* Plugin struct.  */
typedef struct {
  IPIOURIs uris;
  LV2_Log_Log *log;
  LV2_Log_Logger logger;
  LV2_Atom_Forge forge;
  LV2_Atom_Forge_Frame notify_frame;
  LV2_Worker_Schedule *schedule;
  uint32_t samplerate;
  uint32_t frame_offset;
  void *ports[IPIO_NUM_PORTS];
  PcktKit *kit;
  char *kit_filename;
  PcktSoundPool *pool;
} IndiePocket;

/* Internal message structs.  */
typedef struct {
  LV2_Atom atom;
  PcktKit *kit;
  char *kit_filename;
} IPcktKitMsg;

typedef struct {
  LV2_Atom atom;
  PcktDrum *drum;
  int8_t id;
} IPcktDrumMsg;

typedef struct {
  LV2_Atom atom;
  PcktDrumMeta *meta;
  int8_t id;
} IPcktDrumMetaMsg;

typedef struct {
  IndiePocket *plugin;
  LV2_Worker_Respond_Function respond;
  LV2_Worker_Respond_Handle handle;
  PcktKit *kit;
} IPcktDrumLoadedHandle;

/* Set up plugin.  */
static LV2_Handle
instantiate (const LV2_Descriptor *descriptor, double rate,
             const char *bundle_path,
             const LV2_Feature * const *features)
{
  (void) descriptor;
  (void) bundle_path;

  IndiePocket *plugin = (IndiePocket *) malloc (sizeof (IndiePocket));
  if (!plugin)
    return NULL;
  memset (plugin, 0, sizeof (IndiePocket));

  LV2_URID_Map *map = NULL;
  uint32_t i;
  for (i = 0; features[i]; ++i)
    if (!strcmp (features[i]->URI, LV2_URID__map))
      map = (LV2_URID_Map *) features[i]->data;
    else if (!strcmp (features[i]->URI, LV2_LOG__log))
      plugin->log = (LV2_Log_Log *) features[i]->data;
    else if (!strcmp (features[i]->URI, LV2_WORKER__schedule))
      plugin->schedule = (LV2_Worker_Schedule *) features[i]->data;

  lv2_log_logger_init (&plugin->logger, map, plugin->log);

  if (!map)
    {
      lv2_log_error (&plugin->logger, "Missing feature uri:map");
      free (plugin);
      return NULL;
    }
  else if (!plugin->schedule)
    {
      lv2_log_error (&plugin->logger, "Missing feature work:schedule");
      free (plugin);
      return NULL;
    }

  ipio_map_uris (&plugin->uris, map);
  lv2_atom_forge_init (&plugin->forge, map);
  plugin->samplerate = (uint32_t) rate;
  plugin->kit = NULL;
  plugin->pool = pckt_soundpool_new (MAX_NUM_SOUNDS);

  return (LV2_Handle) plugin;
}

/* Connect host ports to the plugin instance. Port buffers may only be
   accessed by functions in the same (`audio') threading class.  */
static void
connect_port (LV2_Handle instance, uint32_t port, void *data)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  if (port < IPIO_NUM_PORTS)
    plugin->ports[port] = data;
}

/* Initialize plugin in preparation for `run' by allocating required
   resources and resetting state data.  */
static void
activate (LV2_Handle instance)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  (void) plugin;
}

/* Handle incoming non-midi events in the audio thread.  */
static inline void
handle_event (IndiePocket *plugin, LV2_Atom_Event *event)
{
  if (!ipio_atom_type_is_object (&plugin->forge, event->body.type))
    {
      lv2_log_trace (&plugin->logger,
                     "Unknown event type %d\n", event->body.type);
      return;
    }

  const LV2_Atom_Object *obj = (const LV2_Atom_Object *) &event->body;
  if (obj->body.otype == plugin->uris.patch_Set)
    {
      const LV2_Atom *sub = NULL, *prop = NULL, *val = NULL;
      lv2_atom_object_get (obj,
                           plugin->uris.patch_subject, &sub,
                           plugin->uris.patch_property, &prop,
                           plugin->uris.patch_value, &val,
                           0);
      if (!prop)
        {
          lv2_log_error (&plugin->logger, "patch:Set missing property\n");
          return;
        }
      else if (prop->type != plugin->uris.atom_URID)
        {
          lv2_log_error (&plugin->logger,
                         "patch:Set property is not a URID\n");
          return;
        }

      const uint32_t key = ((const LV2_Atom_URID *) prop)->body;
      if (key == plugin->uris.pckt_Kit)
        plugin->schedule->schedule_work (plugin->schedule->handle,
                                         lv2_atom_total_size (&event->body),
                                         &event->body);
      else if (sub && (sub->type == plugin->forge.Int)
               && val && (val->type == plugin->forge.Float))
        {
          int8_t index = ((const LV2_Atom_Int *) sub)->body;
          float value = ((const LV2_Atom_Float *) val)->body;
          PcktDrumMeta *meta = pckt_kit_get_drum_meta (plugin->kit, index);
          if (meta)
            {
              if (key == plugin->uris.pckt_tuning)
                pckt_drum_meta_set_tuning (meta, value);
              else if (key == plugin->uris.pckt_dampening)
                pckt_drum_meta_set_dampening (meta, value);
              else if (key == plugin->uris.pckt_expression)
                pckt_drum_meta_set_expression (meta, value);
              else if (key == plugin->uris.pckt_overlap)
                pckt_drum_meta_set_sample_overlap (meta, value);
            }
        }
      else
        lv2_log_error (&plugin->logger,
                       "Unknown patch:Set property %u\n", key);
    }
  else
    lv2_log_trace (&plugin->logger,
                   "Unknown object type %d\n", obj->body.otype);
}

/* Generate NFRAMES frames starting at OFFSET in plugin ouput.  */
static void
write_output (IndiePocket *plugin, uint32_t nframes, uint32_t offset)
{
  if (!plugin->kit || nframes == 0)
    return;

  float *out[PCKT_NCHANNELS];
  for (uint32_t port = 0; port < PCKT_NCHANNELS; ++port)
    out[port] = (IPIO_IS_AUDIO_OUT_PORT (port)
                 ? ((float *) plugin->ports[port]) + offset
                 :  NULL);

  PcktSound *sound;
  uint32_t i = 0;
  while ((sound = pckt_soundpool_at (plugin->pool, i++)))
    pckt_sound_process (sound, out, nframes, plugin->samplerate);
}

/* Write NFRAMES frames to audio output ports. This function runs in
   in the `audio' threading class and must be real-time safe.  */
static void
run (LV2_Handle instance, uint32_t nframes)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  uint32_t offset = 0;
  plugin->frame_offset = 0;

  /* Connect forge to notify output port.  */
  LV2_Atom_Sequence *notify = (LV2_Atom_Sequence *) plugin->ports[IPIO_NOTIFY];
  lv2_atom_forge_set_buffer (&plugin->forge, (uint8_t *) notify,
                             notify->atom.size);
  lv2_atom_forge_sequence_head (&plugin->forge, &plugin->notify_frame, 0);

  /* Read incoming events.  */
  const LV2_Atom_Sequence *events =
    (const LV2_Atom_Sequence *) plugin->ports[IPIO_CONTROL];
  LV2_ATOM_SEQUENCE_FOREACH (events, event)
    {
      plugin->frame_offset = event->time.frames;
      if (event->body.type != plugin->uris.midi_Event)
        {
          handle_event (plugin, event);
          continue;
        }
      const uint8_t * const msg = (const uint8_t *) (event + 1);
      if (lv2_midi_message_type (msg) == LV2_MIDI_MSG_NOTE_ON)
        {
          if (event->time.frames > offset)
            {
              write_output (plugin, event->time.frames - offset, offset);
              offset = (uint32_t) event->time.frames;
            }

          /* Make a sound if there is a drum for the pressed MIDI note.  */
          PcktDrum *drum = pckt_kit_get_drum (plugin->kit, (int8_t) msg[1]);
          if (drum)
            {
              PcktSound *sound = pckt_soundpool_get (plugin->pool, drum);
              if (sound)
                pckt_drum_hit (drum, sound, ((float) msg[2]) / 127);
            }

          /* Choke any sounds affected by the note.  */
          pckt_kit_choke_by_id (plugin->kit, plugin->pool, (int8_t) msg[1]);
        }
    }

  if (nframes > offset)
    write_output (plugin, nframes - offset, offset);

  plugin->frame_offset = nframes;
}

/* Free any resources allocated in `activate'.  */
static void
deactivate (LV2_Handle instance)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  (void) plugin;
}

/* Free any resources allocated in `instantiate'.  */
static void
cleanup (LV2_Handle instance)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  if (plugin->kit)
    pckt_kit_free (plugin->kit);
  if (plugin->kit_filename)
    free (plugin->kit_filename);
  pckt_soundpool_free (plugin->pool);
  free (plugin);
}

/* Callback for `pckt_kit_factory_load_drums'.  */
static void
on_drum_loaded (void *data, PcktDrum *drum, int8_t id, const int8_t *chokers,
                size_t nchokers)
{
  IPcktDrumLoadedHandle *handle = (IPcktDrumLoadedHandle *) data;
  IPcktDrumMsg message = {
    {sizeof (PcktDrum *) + sizeof (int8_t), handle->plugin->uris.pckt_Drum},
    drum,
    id
  };

  if (pckt_kit_get_drum (handle->kit, id))
    {
      lv2_log_note (&handle->plugin->logger, "ID %d is occupied\n", id);
      pckt_drum_free (drum);
      return;
    }

  for (uint8_t i = 0; i < nchokers; ++i)
    pckt_kit_set_choke (handle->kit, chokers[i], id, true);

  /* Tell audio thread to add this drum to current kit.  */
  handle->respond (handle->handle, sizeof (IPcktDrumMsg), &message);
}

/* Handle scheduled non-realtime work.  */
static LV2_Worker_Status
work (LV2_Handle instance, LV2_Worker_Respond_Function respond,
      LV2_Worker_Respond_Handle handle, uint32_t size, const void *data)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  (void) size;

  const LV2_Atom *atom = (const LV2_Atom *) data;
  if (atom->type == plugin->uris.pckt_freeKit)
    {
      const IPcktKitMsg *msg = (const IPcktKitMsg *) data;
      lv2_log_note (&plugin->logger, "Freeing kit %s\n", msg->kit_filename);
      pckt_kit_free (msg->kit);
      free (msg->kit_filename);
      return LV2_WORKER_SUCCESS;
    }

  const LV2_Atom_Object *obj = (const LV2_Atom_Object *) data;
  const LV2_Atom *kit_path = ipio_atom_get_kit_file (&plugin->uris, obj);
  if (!kit_path)
    return LV2_WORKER_ERR_UNKNOWN;

  const char *filename = (const char *) LV2_ATOM_BODY_CONST (kit_path);
  PcktKit *kit = NULL;
  PcktStatus err = PCKTE_SUCCESS;
  PcktKitFactory *factory = pckt_kit_factory_new (filename, &err);
  if (factory)
    {
      lv2_log_note (&plugin->logger, "Loading %s\n", filename);
      kit = pckt_kit_new ();
      err = pckt_kit_factory_load_metas (factory, kit);
      if (err != PCKTE_SUCCESS)
        {
          pckt_kit_factory_free (factory);
          pckt_kit_free (kit);
          factory = NULL;
          kit = NULL;
        }
    }
  else
    lv2_log_error (&plugin->logger, "pckt_kit_factory_new: %s\n",
                   pckt_strerror (err));

  IPcktKitMsg kit_msg = {
    {sizeof (PcktKit *) + sizeof (char *), plugin->uris.pckt_Kit},
    kit,
    NULL
  };

  if (kit)
    {
      kit_msg.kit_filename = (char *) malloc (strlen (filename) + 1);
      strcpy (kit_msg.kit_filename, filename);
    }
  else
    lv2_log_error (&plugin->logger, "Failed to load %s\n", filename);

  /* Tell audio thread to use new kit.  */
  respond (handle, sizeof (IPcktKitMsg), &kit_msg);

  if (factory)
    {
      IPcktDrumLoadedHandle on_load_handle = {
        plugin, respond, handle, kit
      };

      PCKT_KIT_EACH_DRUM_META (kit, meta)
        {
          IPcktDrumMetaMsg meta_msg = {
            {
              sizeof (PcktDrumMeta *) + sizeof (int8_t),
              plugin->uris.pckt_DrumMeta
            },
            meta, pckt_kit_get_drum_meta_id (kit, meta)
          };

          pckt_kit_factory_load_drums (factory, meta,
                                       on_drum_loaded,
                                       &on_load_handle);

          /* Tell audio thread that a meta drum has finsished loading.  */
          respond (handle, sizeof (IPcktDrumMetaMsg), &meta_msg);
        }

      /* Send kit message again when all drums are loaded.  */
      respond (handle, sizeof (IPcktKitMsg), &kit_msg);

      pckt_kit_factory_free (factory);
    }

  return LV2_WORKER_SUCCESS;
}

/* Send drum meta to notification port.  */
static inline void
write_drum_message (IndiePocket *plugin, int8_t index, PcktDrumMeta *drum)
{
  LV2_Atom_Forge_Frame frame;
  const char *name = pckt_drum_meta_get_name (drum);

  ipio_forge_object (&plugin->forge, &frame, plugin->uris.pckt_DrumMeta);
  ipio_forge_key (&plugin->forge, plugin->uris.pckt_index);
  lv2_atom_forge_int (&plugin->forge, index);
  ipio_forge_key (&plugin->forge, plugin->uris.doap_name);

  if (name)
    lv2_atom_forge_string (&plugin->forge, name, strlen (name));
  else
    lv2_atom_forge_string (&plugin->forge, "", 0);

  lv2_atom_forge_pop (&plugin->forge, &frame);
}

/* Apply `work' result in audio thread.  */
static LV2_Worker_Status
work_response (LV2_Handle instance, uint32_t size, const void *data)
{
  (void) size;

  IndiePocket *plugin = (IndiePocket *) instance;
  const LV2_Atom *atom = (const LV2_Atom *) data;

  if (atom->type == plugin->uris.pckt_Drum)
    {
      const IPcktDrumMsg *msg = (const IPcktDrumMsg *) atom;
      pckt_kit_add_drum (plugin->kit, msg->drum, msg->id);
      return LV2_WORKER_SUCCESS;
    }
  else if (atom->type == plugin->uris.pckt_DrumMeta)
    {
      const IPcktDrumMetaMsg *msg = (const IPcktDrumMetaMsg *) atom;
      lv2_atom_forge_frame_time (&plugin->forge, plugin->frame_offset);
      write_drum_message (plugin, msg->id, msg->meta);
      return LV2_WORKER_SUCCESS;
    }
  else if (atom->type != plugin->uris.pckt_Kit)
    return LV2_WORKER_ERR_UNKNOWN;

  const IPcktKitMsg *kitmsg_new = (const IPcktKitMsg *) atom;
  if (!kitmsg_new->kit)
    {
      if (plugin->kit)
        {
          /* Remind UI about old kit.  */
          PCKT_KIT_EACH_DRUM_META (plugin->kit, meta)
            {
              lv2_atom_forge_frame_time (&plugin->forge, plugin->frame_offset);
              write_drum_message (plugin,
                                  pckt_kit_get_drum_meta_id (plugin->kit, meta),
                                  meta);
            }

          lv2_atom_forge_frame_time (&plugin->forge, plugin->frame_offset);
          ipio_forge_kit_file_atom (&plugin->forge, &plugin->uris,
                                    plugin->kit_filename);
        }
      else
        {
          /* Send empty message if no kit is loaded.  */
          lv2_atom_forge_frame_time (&plugin->forge, plugin->frame_offset);
          ipio_forge_kit_file_atom (&plugin->forge, &plugin->uris, "");
        }

      return LV2_WORKER_SUCCESS;
    }

  if (kitmsg_new->kit == plugin->kit)
    {
      /* Notify UI that the new kit has finished loading.  */
      lv2_atom_forge_frame_time (&plugin->forge, plugin->frame_offset);
      ipio_forge_kit_file_atom (&plugin->forge, &plugin->uris,
                                plugin->kit_filename);
    }
  else
    {
      if (plugin->kit)
        {
          /* Kill all current sounds since they hold references to samples that
             will be freed by the kit.  */
          pckt_soundpool_clear (plugin->pool);
          /* Tell worker to free the old kit.  */
          IPcktKitMsg kitmsg_free = {
            {sizeof (PcktKit *) + sizeof (char *), plugin->uris.pckt_freeKit},
            plugin->kit,
            plugin->kit_filename
          };
          plugin->schedule->schedule_work (plugin->schedule->handle,
                                           sizeof (IPcktKitMsg), &kitmsg_free);
        }

      /* Start using new kit.  */
      plugin->kit = kitmsg_new->kit;
      plugin->kit_filename = kitmsg_new->kit_filename;
    }

  return LV2_WORKER_SUCCESS;
}

/* IndiePocket worker descriptor.  */
static const LV2_Worker_Interface worker = {
  work,
  work_response,
  NULL /* `end_run' N/A.  */
};

/* Return any extension data supported by this plugin.  */
static const void *
extension_data (const char *uri)
{
  if (!strcmp (uri, LV2_WORKER__interface))
    return &worker;

  return NULL;
}

/* IndiePocket plugin descriptor.  */
static const LV2_Descriptor descriptor = {
  IPCKT_URI,
  instantiate,
  connect_port,
  activate,
  run,
  deactivate,
  cleanup,
  extension_data
};

/* Export plugin to lv2 host.  */
LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor (uint32_t index)
{
  if (index == 0)
    return &descriptor;
  return NULL;
}
