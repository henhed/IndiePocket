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

#ifndef INDIEPOCKET_IO_H
#define INDIEPOCKET_IO_H 1

#include <stdio.h>
#include <stdbool.h>

#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#define IPCKT_URI "http://www.henhed.se/lv2/indiepocket"
#define IPCKT_URI_PREFIX IPCKT_URI "#"
#define IPCKT_UI_URI IPCKT_URI_PREFIX "ui"
#define IPCKT_URI_DOAP "http://usefulinc.com/ns/doap#"

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
  IPIO_CONTROL,
  IPIO_NOTIFY,
  IPIO_NUM_PORTS
} IPIOPort;

typedef struct {
  LV2_URID atom_Blank;
  LV2_URID atom_Path;
  LV2_URID atom_URID;
  LV2_URID atom_eventTransfer;
  LV2_URID doap_name;
  LV2_URID midi_Event;
  LV2_URID patch_Set;
  LV2_URID patch_property;
  LV2_URID patch_subject;
  LV2_URID patch_value;
  LV2_URID pckt_Drum;
  LV2_URID pckt_Kit;
  LV2_URID pckt_expression;
  LV2_URID pckt_dampening;
  LV2_URID pckt_freeKit;
  LV2_URID pckt_index;
  LV2_URID pckt_tuning;
} IPIOURIs;

#define IPIO_IS_AUDIO_OUT_PORT(port) \
  ((uint32_t) (port) < IPIO_CONTROL)

#define IPIO_FORGE_BUFFER_SIZE 1024

static inline void
ipio_map_uris (IPIOURIs *uris, LV2_URID_Map *map)
{
  uris->atom_Blank = map->map (map->handle, LV2_ATOM__Blank);
  uris->atom_Path = map->map (map->handle, LV2_ATOM__Path);
  uris->atom_URID = map->map (map->handle, LV2_ATOM__URID);
  uris->atom_eventTransfer = map->map (map->handle, LV2_ATOM__eventTransfer);
  uris->doap_name = map->map (map->handle, IPCKT_URI_DOAP "name");
  uris->midi_Event = map->map (map->handle, LV2_MIDI__MidiEvent);
  uris->patch_Set = map->map (map->handle, LV2_PATCH__Set);
  uris->patch_property = map->map (map->handle, LV2_PATCH__property);
  uris->patch_subject = map->map (map->handle, LV2_PATCH__subject);
  uris->patch_value = map->map (map->handle, LV2_PATCH__value);
  uris->pckt_Drum = map->map (map->handle, IPCKT_URI_PREFIX "Drum");
  uris->pckt_Kit = map->map (map->handle, IPCKT_URI_PREFIX "Kit");
  uris->pckt_expression = map->map (map->handle, IPCKT_URI_PREFIX "expression");
  uris->pckt_dampening = map->map (map->handle, IPCKT_URI_PREFIX "dampening");
  uris->pckt_freeKit = map->map (map->handle, IPCKT_URI_PREFIX "freeKit");
  uris->pckt_index = map->map (map->handle, IPCKT_URI_PREFIX "index");
  uris->pckt_tuning = map->map (map->handle, IPCKT_URI_PREFIX "tuning");
}

static inline bool
ipio_atom_type_is_object (const LV2_Atom_Forge* forge, uint32_t type)
{
#if defined HAVE_LV2_ATOM_OBJECT && !HAVE_LV2_ATOM_OBJECT
  return type == forge->Blank || type == forge->Resource;
#else
  return lv2_atom_forge_is_object_type (forge, type);
#endif
}

static inline LV2_Atom_Forge_Ref
ipio_forge_object (LV2_Atom_Forge *forge, LV2_Atom_Forge_Frame *frame,
                   LV2_URID type)
{
#if defined HAVE_LV2_ATOM_OBJECT && !HAVE_LV2_ATOM_OBJECT
  return lv2_atom_forge_blank (forge, frame, 1, type);
#else
  return lv2_atom_forge_object (forge, frame, 0, type);
#endif
}

static inline LV2_Atom_Forge_Ref
ipio_forge_key (LV2_Atom_Forge *forge, LV2_URID key)
{
#if defined HAVE_LV2_ATOM_OBJECT && !HAVE_LV2_ATOM_OBJECT
  return lv2_atom_forge_property_head (forge, key, 0);
#else
  return lv2_atom_forge_key (forge, key);
#endif
}

static inline LV2_Atom *
ipio_forge_kit_file_atom (LV2_Atom_Forge *forge, const IPIOURIs *uris,
                          const char *path)
{
  LV2_Atom_Forge_Frame frame;
  LV2_Atom *msg = (LV2_Atom *) ipio_forge_object (forge, &frame,
                                                  uris->patch_Set);

  ipio_forge_key (forge, uris->patch_property);
  lv2_atom_forge_urid (forge, uris->pckt_Kit);
  ipio_forge_key (forge, uris->patch_value);
  lv2_atom_forge_path (forge, path, strlen (path));

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

  const LV2_Atom *file_path = NULL;
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

static inline LV2_Atom *
ipio_write_drum_property (LV2_Atom_Forge *forge, const IPIOURIs *uris,
                          int8_t drum, LV2_URID property, float value)
{
  LV2_Atom_Forge_Frame frame;
  LV2_Atom *msg = (LV2_Atom *) ipio_forge_object (forge, &frame,
                                                  uris->patch_Set);

  ipio_forge_key (forge, uris->patch_subject);
  lv2_atom_forge_int (forge, drum);
  ipio_forge_key (forge, uris->patch_property);
  lv2_atom_forge_urid (forge, property);
  ipio_forge_key (forge, uris->patch_value);
  lv2_atom_forge_float (forge, value);

  lv2_atom_forge_pop (forge, &frame);

  return msg;
}

#endif /* ! INDIEPOCKET_IO_H */
