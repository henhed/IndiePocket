/* Standard headers.  */
#include <stdlib.h>

/* LV2 headers.  */
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>

/* IndiePocket headers.  */
#include "../pckt/kit.h"
#include "../pckt/sound.h"
#include "../pckt/drum.h"
#include "indiepocket_io.h"

#define MAX_NUM_SOUNDS 32

typedef struct {
  IPIOURIs uris;
  LV2_Log_Log *log;
  LV2_Log_Logger logger;
  uint32_t samplerate;
  void *ports[IPIO_NUM_PORTS];
  PcktKit *kit;
  PcktSoundPool *pool;
} IndiePocket;

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

  lv2_log_logger_init (&plugin->logger, map, plugin->log);

  if (!map)
    {
      lv2_log_error (&plugin->logger, "Missing feature uri:map");
      free (plugin);
      return NULL;
    }

  ipio_map_uris (&plugin->uris, map);
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
static void
handle_event (IndiePocket *plugin, LV2_Atom_Event *event)
{
  if (event->body.type != plugin->uris.atom_Blank)
    {
      lv2_log_trace (&plugin->logger,
                     "Unknown event type %d", event->body.type);
      return;
    }

  const LV2_Atom_Object *obj = (const LV2_Atom_Object *) &event->body;
  if (obj->body.otype == plugin->uris.patch_Set)
    {
      const LV2_Atom *kit_path = ipio_atom_get_kit_file (&plugin->uris, obj);
      if (kit_path)
        {
          lv2_log_note (&plugin->logger,
                        "Recieved request to load: \"%s\"",
                        (const char *) LV2_ATOM_BODY_CONST (kit_path));
        }
    }
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
  const LV2_Atom_Sequence *events =
    (const LV2_Atom_Sequence *) plugin->ports[IPIO_ATOM_IN];
  LV2_ATOM_SEQUENCE_FOREACH (events, event)
    {
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
  pckt_kit_free (plugin->kit);
  pckt_soundpool_free (plugin->pool);
  free (plugin);
}

/* Return any extension data supported by this plugin.  */
static const void *
extension_data (const char *uri)
{
  (void) uri;
  return NULL;
}

/* IndiePocket plugin descriptor.  */
static const LV2_Descriptor descriptor = {
  INDIEPOCKET_URI,
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
