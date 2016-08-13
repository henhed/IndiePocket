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
#include <string.h>
#include <glob.h>
#include "kit_factory.h"
#include "util.h"
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

typedef struct {
  PcktKitParserIface iface;
  const PcktKitFactory *factory;
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
} TurtleParser;

static inline bool
node_is_type (const SordNode *node, const char *type_uri)
{
  if (node && (sord_node_get_type (node) == SORD_LITERAL))
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
  return pckt_strtof ((const char*) sord_node_get_string (node), NULL);
}

static PcktStatus
init_sord (TurtleParser *parser)
{
  const char *filename = pckt_kit_factory_get_filename (parser->factory);
  FILE *fd = fopen (filename, "rb");

  if (!fd)
    return PCKTE_INVAL;

  const uint8_t *file_uri = (const uint8_t *) filename;
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
      return PCKTE_INVAL;
    }

  parser->model = model;
  parser->world = world;

  return PCKTE_SUCCESS;
}

static void
init_uris (TurtleParser *parser)
{
  SordWorld *world = parser->world;
  parser->uris.class_drum = sord_new_uri (world, URI_PCKT__Drum);
  parser->uris.class_drum_hit = sord_new_uri (world, URI_PCKT__DrumHit);
  parser->uris.class_kit = sord_new_uri (world, URI_PCKT__Kit);
  parser->uris.class_mic = sord_new_uri (world, URI_PCKT__Mic);
  parser->uris.class_sound = sord_new_uri (world, URI_PCKT__Sound);
  parser->uris.prop_bleed = sord_new_uri (world, URI_PCKT__bleed);
  parser->uris.prop_channel = sord_new_uri (world, URI_PCKT__channel);
  parser->uris.prop_choke = sord_new_uri (world, URI_PCKT__choke);
  parser->uris.prop_drum = sord_new_uri (world, URI_PCKT__drum);
  parser->uris.prop_kit = sord_new_uri (world, URI_PCKT__kit);
  parser->uris.prop_key = sord_new_uri (world, URI_PCKT__key);
  parser->uris.prop_mic = sord_new_uri (world, URI_PCKT__mic);
  parser->uris.prop_name = sord_new_uri (world, URI_DOAP__name);
  parser->uris.prop_sample = sord_new_uri (world, URI_PCKT__sample);
  parser->uris.prop_sound = sord_new_uri (world, URI_PCKT__sound);
  parser->uris.prop_type = sord_new_uri (world, URI_SNTX__type);
}

static void
init_mics (TurtleParser *parser)
{
  PcktChannel ch;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    parser->mics[ch] = NULL;

  SordIter *mic_it = sord_search (parser->model, NULL,
                                  parser->uris.prop_type,
                                  parser->uris.class_mic, NULL);
  if (!mic_it)
    return; /* No mics defined.  */

  for (; !sord_iter_end (mic_it); sord_iter_next (mic_it))
    {
      const SordNode *mic_node = sord_iter_get_node (mic_it, SORD_SUBJECT);
      SordNode *ch_node = sord_get (parser->model, mic_node,
                                    parser->uris.prop_channel, NULL, NULL);
      if (ch_node)
        {
          if (node_is_int (ch_node))
            {
              int32_t i = node_to_int (ch_node);
              if (i >= PCKT_CH0 && i < PCKT_NCHANNELS)
                parser->mics[i] = mic_node;
            }
          sord_node_free (parser->world, ch_node);
        }
    }
  sord_iter_free (mic_it);
}

static inline PcktChannel
get_sound_channel (const TurtleParser *parser, const SordNode *sound_node)
{
  PcktChannel ch = PCKT_NCHANNELS;
  SordNode *mic = sord_get (parser->model, sound_node, parser->uris.prop_mic,
                            NULL, NULL);
  if (!mic)
    return ch;

  for (PcktChannel i = PCKT_CH0; i < PCKT_NCHANNELS; ++i)
    {
      if (parser->mics[i] && sord_node_equals (mic, parser->mics[i]))
        {
          ch = i;
          break;
        }
    }

  sord_node_free (parser->world, mic);
  return ch;
}

static inline float
get_sound_bleed (const TurtleParser *parser, const SordNode *sound_node)
{
  float bleed = 1;
  SordNode *bleed_node = sord_get (parser->model, sound_node,
                                   parser->uris.prop_bleed, NULL, NULL);
  if (bleed_node)
    {
      if (node_is_float (bleed_node))
        bleed = node_to_float (bleed_node);
      sord_node_free (parser->world, bleed_node);
    }

  if (bleed > 1)
    bleed = 1;
  else if (bleed < 0)
    bleed = 0;
  return bleed;
}

static inline bool
get_sample_pattern (const TurtleParser *parser, const SordNode *sample_node,
                    glob_t *globbuf)
{
  bool result = false;
  char *abspath;
  const char *globpat;

  if (sord_node_get_type (sample_node) != SORD_LITERAL)
    return result;

  globpat = (const char *) sord_node_get_string (sample_node);
  abspath = pckt_kit_factory_get_abspath (parser->factory, globpat);

  if (abspath)
    {
      result = (glob (abspath, 0, NULL, globbuf) == 0) ? true : false;
      free (abspath);
    }

  return result;
}

static inline void
load_drum_samples (const TurtleParser *parser, PcktDrum *drum,
                   PcktChannel channel, const SordNode *sound_node)
{
  SordIter *sample_it = sord_search (parser->model, sound_node,
                                     parser->uris.prop_sample, NULL, NULL);
  if (!sample_it)
    return;

  const char *basedir = pckt_kit_factory_get_basedir (parser->factory);

  for (; !sord_iter_end (sample_it); sord_iter_next (sample_it))
    {
      const SordNode *sample_node = sord_iter_get_node (sample_it, SORD_OBJECT);
      glob_t globbuf;
      if (!get_sample_pattern (parser, sample_node, &globbuf))
        continue;

      for (char **path = globbuf.gl_pathv; *path != NULL; ++path)
        {
          char *name = *path;
          PcktSample *sample = pckt_sample_factory_mono (name);
          if (!sample)
            continue;

          /* Strip base dir from name.  */
          if ((strstr (name, basedir) == name)
              && (strlen (name) > (strlen (basedir) + 1)))
            name += strlen (basedir) + 1;

          /* Add sample to drum.  */
          if (!pckt_drum_add_sample (drum, sample, channel, name))
            pckt_sample_free (sample);
        }

      globfree (&globbuf);
    }
  sord_iter_free (sample_it);
}

static inline PcktDrum *
load_drum_hit (const TurtleParser *parser, const SordNode *hit_node)
{
  PcktDrum *drum = NULL;
  SordIter *sound_it = sord_search (parser->model, hit_node,
                                    parser->uris.prop_sound, NULL, NULL);
  if (!sound_it)
    return drum;

  drum = pckt_drum_new ();
  for (; !sord_iter_end (sound_it); sord_iter_next (sound_it))
    {
      const SordNode *sound_node = sord_iter_get_node (sound_it, SORD_OBJECT);
      SordQuad pat = {
        sound_node,
        parser->uris.prop_type,
        parser->uris.class_sound,
        NULL
      };
      if (!sord_contains (parser->model, pat))
        continue;

      PcktChannel ch = get_sound_channel (parser, sound_node);
      if (ch == PCKT_NCHANNELS)
        continue; /* Invalid channel.  */

      float bleed = get_sound_bleed (parser, sound_node);
      pckt_drum_set_bleed (drum, ch, bleed);

      load_drum_samples (parser, drum, ch, sound_node);
    }
  sord_iter_free (sound_it);

  return drum;
}

static inline int8_t
get_drum_id (const TurtleParser *parser, const SordNode *drum_node)
{
  int8_t id = -1;
  SordNode *key = sord_get (parser->model, drum_node, parser->uris.prop_key,
                            NULL, NULL);
  if (key)
    {
      if (node_is_int (key))
        {
          int32_t i = node_to_int (key);
          if (i > 0 && i <= INT8_MAX)
            id = i;
        }
      sord_node_free (parser->world, key);
    }
  return id;
}

static inline void
load_kit_chokes (const TurtleParser *parser, PcktKit *kit, int8_t id,
                 const SordNode *drum_node)
{
  SordIter *it = sord_search (parser->model, drum_node,
                              parser->uris.prop_choke, NULL, NULL);
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
load_drum (const TurtleParser *parser, PcktKit *kit,
           const SordNode *drum_node)
{
  SordIter *it = sord_search (parser->model, NULL, parser->uris.prop_drum,
                              drum_node, NULL);
  if (!it)
    return;

  SordNode *name_node = sord_get (parser->model, drum_node,
                                  parser->uris.prop_name, NULL, NULL);
  const char *name = (name_node
                      ? (const char *) sord_node_get_string (name_node)
                      : NULL);
  PcktDrumMeta *meta = pckt_drum_meta_new (name);
  if (name_node)
    sord_node_free (parser->world, name_node);

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
        parser->uris.prop_type,
        parser->uris.class_drum_hit,
        NULL
      };
      if (!sord_contains (parser->model, pat))
        continue;

      int8_t id = get_drum_id (parser, hit_node);
      if (id < 0 || pckt_kit_get_drum (kit, id))
        continue; /* Invalid or occupied ID.  */

      PcktDrum *drum = load_drum_hit (parser, hit_node);
      if (drum)
        {
          pckt_drum_set_meta (drum, meta);
          pckt_kit_add_drum (kit, drum, id);
          load_kit_chokes (parser, kit, id, hit_node);
        }
    }

  sord_iter_free (it);
}

static PcktKit *
load_kit (const TurtleParser *parser, const SordNode *kit_node)
{
  PcktKit *kit = pckt_kit_new ();
  SordIter *it = sord_search (parser->model, NULL, parser->uris.prop_kit,
                              kit_node, NULL);
  if (!it)
    return kit; /* No drums found.  */

  for (; !sord_iter_end (it); sord_iter_next (it))
    {
      const SordNode *drum_node = sord_iter_get_node (it, SORD_SUBJECT);
      SordQuad pat = {
        drum_node,
        parser->uris.prop_type,
        parser->uris.class_drum,
        NULL
      };
      if (sord_contains (parser->model, pat))
        load_drum (parser, kit, drum_node);
    }
  sord_iter_free (it);

  return kit;
}

static PcktKit *
ttl_parser_load (PcktKitParserIface *iface, const PcktKitFactory *factory)
{
  (void) factory;

  PcktKit *kit = NULL;
  if (!iface)
    return kit;

  TurtleParser *parser = (TurtleParser *) iface;
  SordNode *kit_node = sord_get (parser->model, NULL, parser->uris.prop_type,
                                 parser->uris.class_kit, NULL);
  if (kit_node)
    {
      kit = load_kit (parser, kit_node);
      sord_node_free (parser->world, kit_node);
    }

  return kit;
}

static void
ttl_parser_free (PcktKitParserIface *iface, const PcktKitFactory *factory)
{
  (void) factory;

  if (!iface)
    return;

  TurtleParser *parser = (TurtleParser *) iface;
  sord_free (parser->model);
  sord_world_free (parser->world);
  free (parser);
}

PcktKitParserIface *
pckt_kit_parser_ttl_new (const PcktKitFactory *factory)
{
  TurtleParser *parser;
  PcktKitParserIface *iface;
  PcktStatus status;

  if (!factory)
    return NULL;

  parser = malloc (sizeof (TurtleParser));
  if (!parser)
    return NULL;

  memset (parser, 0, sizeof (TurtleParser));

  iface = (PcktKitParserIface *) parser;
  iface->load = ttl_parser_load;
  iface->free = ttl_parser_free;

  parser->factory = factory;

  status = init_sord (parser);
  if (status != PCKTE_SUCCESS)
    {
      free (parser);
      return NULL;
    }

  init_uris (parser);
  init_mics (parser);

  return iface;
}
