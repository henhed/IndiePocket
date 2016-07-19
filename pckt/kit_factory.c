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

#include <serd/serd.h>
#include <sord/sord.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include "kit.h"
#include "drum.h"
#include "sample.h"

#define URI_PCKT "http://www.freeztile.org/rdf-schema/indiepocket#"
#define URI_SNTX "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define URI_XSD "http://www.w3.org/2001/XMLSchema#"
#define URI_DOAP "http://usefulinc.com/ns/doap#"

#define URI_PCKT__Drum (const uint8_t *) URI_PCKT "Drum"
#define URI_PCKT__DrumHit (const uint8_t *) URI_PCKT "DrumHit"
#define URI_PCKT__Kit (const uint8_t *) URI_PCKT "Kit"
#define URI_PCKT__Mic (const uint8_t *) URI_PCKT "Mic"
#define URI_PCKT__Sound (const uint8_t *) URI_PCKT "Sound"
#define URI_PCKT__bleed (const uint8_t *) URI_PCKT "bleed"
#define URI_PCKT__channel (const uint8_t *) URI_PCKT "channel"
#define URI_PCKT__choke (const uint8_t *) URI_PCKT "choke"
#define URI_PCKT__drum (const uint8_t *) URI_PCKT "drum"
#define URI_PCKT__kit (const uint8_t *) URI_PCKT "kit"
#define URI_PCKT__key (const uint8_t *) URI_PCKT "key"
#define URI_PCKT__mic (const uint8_t *) URI_PCKT "mic"
#define URI_DOAP__name (const uint8_t *) URI_DOAP "name"
#define URI_PCKT__sample (const uint8_t *) URI_PCKT "sample"
#define URI_PCKT__sound (const uint8_t *) URI_PCKT "sound"
#define URI_SNTX__type (const uint8_t *) URI_SNTX "type"

#ifdef _WIN32
# define DIR_SEP '\\'
# define BAD_DIR_SEP '/'
#else
# define DIR_SEP '/'
# define BAD_DIR_SEP '\\'
#endif

struct PcktKitFactoryImpl {
  char *filename;
  char *basedir;
  SordWorld *world;
  SordModel *model;
  struct {
    SordNode *class_drum;
    SordNode *class_drum_hit;
    SordNode *class_kit;
    SordNode *class_mic;
    SordNode *class_sound;
    SordNode *prop_bleed;
    SordNode *prop_channel;
    SordNode *prop_choke;
    SordNode *prop_drum;
    SordNode *prop_kit;
    SordNode *prop_key;
    SordNode *prop_mic;
    SordNode *prop_name;
    SordNode *prop_sample;
    SordNode *prop_sound;
    SordNode *prop_type;
  } uris;
  const SordNode *mics[PCKT_NCHANNELS];
};

static inline bool
node_is_type (const SordNode *node, const char *type_uri)
{
  if (node && sord_node_get_type (node) == SORD_LITERAL)
    {
      const SordNode *datatype = sord_node_get_datatype (node);
      if (datatype && !strcmp ((const char *) sord_node_get_string (datatype),
                               type_uri))
        return true;
    }
  return false;
}

static inline bool
node_is_int (const SordNode *node)
{
  return node_is_type (node, URI_XSD "integer");
}

static inline bool
node_is_float (const SordNode *node)
{
  return node_is_type (node, URI_XSD "decimal");
}

static inline int32_t
node_to_int (const SordNode *node)
{
  return atoi ((const char*) sord_node_get_string (node));
}

static inline float
node_to_float (const SordNode *node)
{
  return serd_strtod ((const char*) sord_node_get_string (node), NULL);
}

static PcktStatus
init_sord (PcktKitFactory *factory, const char *filename)
{
  FILE *fd = fopen (filename, "rb");
  if (!fd)
    return PCKTE_INVAL;

  factory->filename = realpath (filename, NULL);
  if (!factory->filename)
    {
      fclose (fd);
      return PCKTE_NOMEM;
    }

  uint8_t *file_uri = (uint8_t *) factory->filename;
  SordWorld *world = sord_world_new ();
  SordModel *model = sord_new (world, SORD_OPS | SORD_SPO, 0);
  SerdURI base_uri = SERD_URI_NULL;
  SerdNode root = serd_node_new_file_uri (file_uri, NULL, &base_uri, 0);
  SerdEnv *env = serd_env_new (&root);
  SerdReader *reader = sord_new_reader (model, env, SERD_TURTLE, NULL);
  SerdStatus status = serd_reader_read_file_handle (reader, fd, file_uri);
  serd_reader_free (reader);
  serd_env_free (env);
  serd_node_free (&root);

  fclose (fd);

  if (status > SERD_FAILURE)
    {
      sord_free (model);
      sord_world_free (world);
      free (factory->filename);
      factory->filename = NULL;
      return PCKTE_INVAL;
    }

  factory->model = model;
  factory->world = world;

  return PCKTE_SUCCESS;
}

static void
init_uris (PcktKitFactory *factory)
{
  SordWorld *world = factory->world;
  factory->uris.class_drum = sord_new_uri (world, URI_PCKT__Drum);
  factory->uris.class_drum_hit = sord_new_uri (world, URI_PCKT__DrumHit);
  factory->uris.class_kit = sord_new_uri (world, URI_PCKT__Kit);
  factory->uris.class_mic = sord_new_uri (world, URI_PCKT__Mic);
  factory->uris.class_sound = sord_new_uri (world, URI_PCKT__Sound);
  factory->uris.prop_bleed = sord_new_uri (world, URI_PCKT__bleed);
  factory->uris.prop_channel = sord_new_uri (world, URI_PCKT__channel);
  factory->uris.prop_choke = sord_new_uri (world, URI_PCKT__choke);
  factory->uris.prop_drum = sord_new_uri (world, URI_PCKT__drum);
  factory->uris.prop_kit = sord_new_uri (world, URI_PCKT__kit);
  factory->uris.prop_key = sord_new_uri (world, URI_PCKT__key);
  factory->uris.prop_mic = sord_new_uri (world, URI_PCKT__mic);
  factory->uris.prop_name = sord_new_uri (world, URI_DOAP__name);
  factory->uris.prop_sample = sord_new_uri (world, URI_PCKT__sample);
  factory->uris.prop_sound = sord_new_uri (world, URI_PCKT__sound);
  factory->uris.prop_type = sord_new_uri (world, URI_SNTX__type);
}

static void
init_mics (PcktKitFactory *factory)
{
  PcktChannel ch;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    factory->mics[ch] = NULL;

  SordIter *mic_it = sord_search (factory->model, NULL,
                                  factory->uris.prop_type,
                                  factory->uris.class_mic, NULL);
  if (!mic_it)
    return; // no mics defined

  for (; !sord_iter_end (mic_it); sord_iter_next (mic_it))
    {
      const SordNode *mic_node = sord_iter_get_node (mic_it, SORD_SUBJECT);
      SordNode *ch_node = sord_get (factory->model, mic_node,
                                    factory->uris.prop_channel, NULL, NULL);
      if (ch_node)
        {
          if (node_is_int (ch_node))
            {
              int32_t i = node_to_int (ch_node);
              if (i >= PCKT_CH0 && i < PCKT_NCHANNELS)
                factory->mics[i] = mic_node;
            }
          sord_node_free (factory->world, ch_node);
        }
    }
  sord_iter_free (mic_it);
}

PcktKitFactory *
pckt_kit_factory_new (const char *filename, PcktStatus *status)
{
  PcktKitFactory *factory = malloc (sizeof (PcktKitFactory));
  if (!factory)
    {
      if (status)
        *status = PCKTE_NOMEM;
      return NULL;
    }

  memset (factory, 0, sizeof (PcktKitFactory));

  PcktStatus err = init_sord (factory, filename);
  if (err != PCKTE_SUCCESS)
    {
      if (status)
        *status = err;
      free (factory);
      return NULL;
    }

  /* `dirname' will modify given file name in most cases.  `filename' is not
     currently used and is only kept because the pointer returned by `dirname'
     can not be safely passed to `free'.  */
  factory->basedir = dirname (factory->filename);

  init_uris (factory);
  init_mics (factory);

  return factory;
}

void
pckt_kit_factory_free (PcktKitFactory *factory)
{
  if (!factory)
    return;

  sord_free (factory->model);
  sord_world_free (factory->world);
  free (factory->filename);
  free (factory);
}

static inline PcktChannel
get_sound_channel (const PcktKitFactory *factory, const SordNode *sound_node)
{
  PcktChannel ch = PCKT_NCHANNELS;
  SordNode *mic = sord_get (factory->model, sound_node, factory->uris.prop_mic,
                            NULL, NULL);
  if (!mic)
    return ch;

  for (PcktChannel i = PCKT_CH0; i < PCKT_NCHANNELS; ++i)
    {
      if (factory->mics[i] && sord_node_equals (mic, factory->mics[i]))
        {
          ch = i;
          break;
        }
    }

  sord_node_free (factory->world, mic);
  return ch;
}

static inline float
get_sound_bleed (const PcktKitFactory *factory, const SordNode *sound_node)
{
  float bleed = 1;
  SordNode *bleed_node = sord_get (factory->model, sound_node,
                                   factory->uris.prop_bleed, NULL, NULL);
  if (bleed_node)
    {
      if (node_is_float (bleed_node))
        bleed = node_to_float (bleed_node);
      sord_node_free (factory->world, bleed_node);
    }

  if (bleed > 1)
    bleed = 1;
  else if (bleed < 0)
    bleed = 0;
  return bleed;
}

static inline PcktSample *
load_sample (const PcktKitFactory *factory, const SordNode *file_node)
{
  if (sord_node_get_type (file_node) != SORD_LITERAL)
    return NULL;

  const char *filename = (const char *) sord_node_get_string (file_node);
  size_t flen = strlen (filename), dlen = strlen (factory->basedir);
  char localized[flen + 1]; // + \0
  char fullpath[dlen + 1 + flen + 1]; // + DIR_SEP + \0
  char *dirsep;

  if (flen == 0)
    return NULL;

  strcpy (localized, filename);
  while ((dirsep = strchr (localized, BAD_DIR_SEP)))
    *dirsep = DIR_SEP;

  sprintf (fullpath, "%s%c%s", factory->basedir, DIR_SEP, localized);
  return pckt_sample_factory (fullpath);
}

static inline void
load_drum_samples (const PcktKitFactory *factory, PcktDrum *drum,
                   PcktChannel channel, const SordNode *sound_node)
{
  SordIter *sample_it = sord_search (factory->model, sound_node,
                                     factory->uris.prop_sample, NULL, NULL);
  if (!sample_it)
    return;

  for (; !sord_iter_end (sample_it); sord_iter_next (sample_it))
    {
      const SordNode *sample_node = sord_iter_get_node (sample_it, SORD_OBJECT);
      PcktSample *sample = load_sample (factory, sample_node);
      if (sample)
        {
          const char *name = (const char *) sord_node_get_string (sample_node);
          pckt_drum_add_sample (drum, sample, channel, name);
        }
    }
  sord_iter_free (sample_it);
}

static inline PcktDrum *
load_drum_hit (const PcktKitFactory *factory, const SordNode *hit_node)
{
  PcktDrum *drum = NULL;
  SordIter *sound_it = sord_search (factory->model, hit_node,
                                    factory->uris.prop_sound, NULL, NULL);
  if (!sound_it)
    return drum;

  drum = pckt_drum_new ();
  for (; !sord_iter_end (sound_it); sord_iter_next (sound_it))
    {
      const SordNode *sound_node = sord_iter_get_node (sound_it, SORD_OBJECT);
      SordQuad pat = {
        sound_node,
        factory->uris.prop_type,
        factory->uris.class_sound,
        NULL
      };
      if (!sord_contains (factory->model, pat))
        continue;

      PcktChannel ch = get_sound_channel (factory, sound_node);
      if (ch == PCKT_NCHANNELS)
        continue; // invalid channel

      float bleed = get_sound_bleed (factory, sound_node);
      pckt_drum_set_bleed (drum, ch, bleed);

      load_drum_samples (factory, drum, ch, sound_node);
    }
  sord_iter_free (sound_it);

  return drum;
}

static inline int8_t
get_drum_id (const PcktKitFactory *factory, const SordNode *drum_node)
{
  int8_t id = -1;
  SordNode *key = sord_get (factory->model, drum_node, factory->uris.prop_key,
                            NULL, NULL);
  if (key)
    {
      if (node_is_int (key))
        {
          int32_t i = node_to_int (key);
          if (i > 0 && i <= INT8_MAX)
            id = i;
        }
      sord_node_free (factory->world, key);
    }
  return id;
}

static inline void
load_kit_chokes (const PcktKitFactory *factory, PcktKit *kit, int8_t id,
                 const SordNode *drum_node)
{
  SordIter *it = sord_search (factory->model, drum_node,
                              factory->uris.prop_choke, NULL, NULL);
  if (it)
    {
      for (; !sord_iter_end (it); sord_iter_next (it))
        {
          const SordNode *choke_node = sord_iter_get_node (it, SORD_OBJECT);
          if (node_is_int (choke_node))
            pckt_kit_set_choke (kit, node_to_int (choke_node), id, true);
        }
      sord_iter_free (it);
    }
}

static inline void
load_drum (const PcktKitFactory *factory, PcktKit *kit,
           const SordNode *drum_node)
{
  SordIter *it = sord_search (factory->model, NULL, factory->uris.prop_drum,
                              drum_node, NULL);
  if (!it)
    return;

  SordNode *name_node = sord_get (factory->model, drum_node,
                                  factory->uris.prop_name, NULL, NULL);
  const char *name = (name_node
                      ? (const char *) sord_node_get_string (name_node)
                      : NULL);
  PcktDrumMeta *meta = pckt_drum_meta_new (name);
  if (name_node)
    sord_node_free (factory->world, name_node);

  if (!meta || pckt_kit_add_drum_meta (kit, meta) < 0)
    {
      if (meta)
        pckt_drum_meta_free (meta);
      return;
    }

  for (; !sord_iter_end (it); sord_iter_next (it))
    {
      const SordNode *hit_node = sord_iter_get_node (it, SORD_SUBJECT);
      SordQuad pat = {
        hit_node,
        factory->uris.prop_type,
        factory->uris.class_drum_hit,
        NULL
      };
      if (!sord_contains (factory->model, pat))
        continue;

      int8_t id = get_drum_id (factory, hit_node);
      if (id < 0 || pckt_kit_get_drum (kit, id))
        continue; // invalid or occupied ID

      PcktDrum *drum = load_drum_hit (factory, hit_node);
      if (drum)
        {
          pckt_drum_set_meta (drum, meta);
          pckt_kit_add_drum (kit, drum, id);
          load_kit_chokes (factory, kit, id, hit_node);
        }
    }

  sord_iter_free (it);
}

static PcktKit *
load_kit (const PcktKitFactory *factory, const SordNode *kit_node)
{
  PcktKit *kit = pckt_kit_new ();
  SordIter *it = sord_search (factory->model, NULL, factory->uris.prop_kit,
                              kit_node, NULL);
  if (!it)
    return kit; // no drums found

  for (; !sord_iter_end (it); sord_iter_next (it))
    {
      const SordNode *drum_node = sord_iter_get_node (it, SORD_SUBJECT);
      SordQuad pat = {
        drum_node,
        factory->uris.prop_type,
        factory->uris.class_drum,
        NULL
      };
      if (sord_contains (factory->model, pat))
        load_drum (factory, kit, drum_node);
    }
  sord_iter_free (it);

  return kit;
}

PcktKit *
pckt_kit_factory_load (const PcktKitFactory *factory)
{
  PcktKit *kit = NULL;
  if (!factory)
    return kit;

  SordNode *kit_node = sord_get (factory->model, NULL, factory->uris.prop_type,
                                 factory->uris.class_kit, NULL);
  if (kit_node)
    {
      kit = load_kit (factory, kit_node);
      sord_node_free (factory->world, kit_node);
    }

  return kit;
}
