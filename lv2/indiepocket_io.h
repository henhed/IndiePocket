#ifndef INDIEPOCKET_IO_H
#define INDIEPOCKET_IO_H 1

#include <stdio.h>

#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#define INDIEPOCKET_URI "http://www.henhed.se/lv2/indiepocket"
#define INDIEPOCKET_URI_PREFIX INDIEPOCKET_URI "#"
#define INDIEPOCKET_UI_URI INDIEPOCKET_URI_PREFIX "ui"

typedef enum {
  IPIO_AUDIO_OUT_KICK_1 = 0,
  IPIO_AUDIO_OUT_KICK_2,
  IPIO_AUDIO_OUT_SNARE_1,
  IPIO_AUDIO_OUT_SNARE_2,
  IPIO_AUDIO_OUT_HIHAT_1,
  IPIO_AUDIO_OUT_HIHAT_2,
  IPIO_AUDIO_OUT_TOM_1,
  IPIO_AUDIO_OUT_TOM_2,
  IPIO_AUDIO_OUT_TOM_3,
  IPIO_AUDIO_OUT_TOM_4,
  IPIO_AUDIO_OUT_CYM_1,
  IPIO_AUDIO_OUT_CYM_2,
  IPIO_AUDIO_OUT_CYM_3,
  IPIO_AUDIO_OUT_CYM_4,
  IPIO_AUDIO_OUT_ROOM_1,
  IPIO_AUDIO_OUT_ROOM_2,
  IPIO_ATOM_IN,
  IPIO_NUM_PORTS
} IPIOPort;

typedef struct {
  LV2_URID atom_Blank;
  LV2_URID atom_Path;
  LV2_URID atom_URID;
  LV2_URID atom_eventTransfer;
  LV2_URID midi_Event;
  LV2_URID patch_Set;
  LV2_URID patch_property;
  LV2_URID patch_value;
  LV2_URID pckt_Kit;
} IPIOURIs;

#define IPIO_IS_AUDIO_OUT_PORT(port) \
  ((uint32_t) (port) < IPIO_ATOM_IN)

#define IPIO_FORGE_BUFFER_SIZE 1024

static inline void
ipio_map_uris (IPIOURIs *uris, LV2_URID_Map *map)
{
  uris->atom_Blank = map->map (map->handle, LV2_ATOM__Blank);
  uris->atom_Path = map->map (map->handle, LV2_ATOM__Path);
  uris->atom_URID = map->map (map->handle, LV2_ATOM__URID);
  uris->atom_eventTransfer = map->map (map->handle, LV2_ATOM__eventTransfer);
  uris->midi_Event = map->map (map->handle, LV2_MIDI__MidiEvent);
  uris->patch_Set = map->map (map->handle, LV2_PATCH__Set);
  uris->patch_property = map->map (map->handle, LV2_PATCH__property);
  uris->patch_value = map->map (map->handle, LV2_PATCH__value);
  uris->pckt_Kit = map->map (map->handle, INDIEPOCKET_URI_PREFIX "Kit");
}

static inline LV2_Atom *
ipio_forge_kit_file_atom (LV2_Atom_Forge *forge, const IPIOURIs *uris,
                          const char *path, const uint32_t pathlen)
{
  LV2_Atom_Forge_Frame frame;
  LV2_Atom *msg = (LV2_Atom *) lv2_atom_forge_blank (forge, &frame, 1,
                                                     uris->patch_Set);

  lv2_atom_forge_property_head (forge, uris->patch_property, 0);
  lv2_atom_forge_urid (forge, uris->pckt_Kit);
  lv2_atom_forge_property_head (forge, uris->patch_value, 0);
  lv2_atom_forge_path (forge, path, pathlen);

  lv2_atom_forge_pop (forge, &frame);

  return msg;
}

static inline const LV2_Atom *
ipio_atom_get_kit_file (const IPIOURIs *uris, const LV2_Atom_Object *obj)
{
  if (obj->body.otype != uris->patch_Set)
    {
      fprintf (stderr, "Ignoring unknown message type %d\n", obj->body.otype);
      return NULL;
    }

  const LV2_Atom *property = NULL;
  lv2_atom_object_get (obj, uris->patch_property, &property, 0);
  if (!property)
    {
      fprintf (stderr, "Malformed set message has no body.\n");
      return NULL;
    }
  else if (property->type != uris->atom_URID)
    {
      fprintf (stderr, "Malformed set message has non-URID property.\n");
      return NULL;
    }
  else if (((const LV2_Atom_URID *) property)->body != uris->pckt_Kit)
    {
      fprintf (stderr, "Set message for unknown property.\n");
      return NULL;
    }

  const LV2_Atom* file_path = NULL;
  lv2_atom_object_get (obj, uris->patch_value, &file_path, 0);
  if (!file_path)
    {
      fprintf (stderr, "Malformed set message has no value.\n");
      return NULL;
    }
  else if (file_path->type != uris->atom_Path)
    {
      fprintf (stderr, "Set message value is not a Path.\n");
      return NULL;
    }

  return file_path;
}

#endif /* ! INDIEPOCKET_IO_H */
