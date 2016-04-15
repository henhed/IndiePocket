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
#include "drum.h"

#define PCKT_URI "http://www.henhed.se/lv2/indiepocket"
#define MAX_NUM_SOUNDS 16

/* Named port numbers.  */
enum {
  AUDIO_OUT_KICK_1 = 0,
  AUDIO_OUT_KICK_2,
  AUDIO_OUT_SNARE_1,
  AUDIO_OUT_SNARE_2,
  MIDI_IN,
  NUM_PORTS
};

typedef struct {
  LV2_URID midi_urid;
  LV2_Log_Log *log;
  LV2_Log_Logger logger;
  unsigned int samplerate;
  void *ports[NUM_PORTS];
  PcktSound sounds[MAX_NUM_SOUNDS];
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

  plugin->midi_urid = map->map (map->handle, LV2_MIDI__MidiEvent);
  plugin->samplerate = (unsigned int) rate;

  return (LV2_Handle) plugin;
}

/* Connect host ports to the plugin instance. Port buffers may only be
   accessed by functions in the same (`audio') threading class.  */
static void
connect_port (LV2_Handle instance, uint32_t port, void *data)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  if (port < NUM_PORTS)
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

/* Generate NFRAMES frames starting at OFFSET in plugin ouput.  */
static void
write_output (IndiePocket *plugin, uint32_t nframes, uint32_t offset)
{
  if (nframes == 0)
    return;

  float *out[PCKT_NCHANNELS];
  memset (out, 0, PCKT_NCHANNELS * sizeof (float *));
  for (uint32_t port = 0; port < MIDI_IN && port < PCKT_NCHANNELS; ++port)
    out[port] = ((float *) plugin->ports[port]) + offset;

  for (uint32_t sound = 0; sound < MAX_NUM_SOUNDS; ++sound)
    pckt_process_sound (&plugin->sounds[sound], out, nframes,
                        plugin->samplerate);
}

/* Write NFRAMES frames to audio output ports. This function runs in
   in the `audio' threading class and must be real-time safe.  */
static void
run (LV2_Handle instance, uint32_t nframes)
{
  IndiePocket *plugin = (IndiePocket *) instance;
  uint32_t offset = 0;

  const LV2_Atom_Sequence *events =
    (const LV2_Atom_Sequence *) plugin->ports[MIDI_IN];
  LV2_ATOM_SEQUENCE_FOREACH (events, event)
    {
      if (event->body.type != plugin->midi_urid)
        continue;
      const uint8_t * const msg = (const uint8_t *) (event + 1);
      if (lv2_midi_message_type (msg) == LV2_MIDI_MSG_NOTE_ON)
        {
          write_output (plugin, event->time.frames - offset, offset);
          offset = (uint32_t) event->time.frames;
          lv2_log_note (&plugin->logger, "KEY: %u, VEL: %u", msg[1], msg[2]);
        }
    }

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
  free (instance);
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
  PCKT_URI,
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
