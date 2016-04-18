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

#ifdef _WIN32
# define DIR_SEP '\\'
# define BAD_DIR_SEP '/'
#else
# define DIR_SEP '/'
# define BAD_DIR_SEP '\\'
#endif

typedef struct {
  const char *base_dir;
  struct {
    SordNode *class_drum;
    SordNode *class_kit;
    SordNode *class_mic;
    SordNode *class_sample;
    SordNode *class_sound;
    SordNode *prop_bleed;
    SordNode *prop_channel;
    SordNode *prop_file;
    SordNode *prop_kit;
    SordNode *prop_key;
    SordNode *prop_mic;
    SordNode *prop_sample;
    SordNode *prop_sound;
    SordNode *prop_type;
  } uris;
  const SordNode *mics[PCKT_NCHANNELS];
} KitFactory;

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

static inline PcktChannel
get_sound_channel (const SordNode *sound_node, SordModel *sord,
                   const KitFactory *factory)
{
  PcktChannel ch = PCKT_NCHANNELS;
  SordNode *mic = sord_get (sord, sound_node, factory->uris.prop_mic, NULL,
                            NULL);
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

  sord_node_free (sord_get_world (sord), mic);
  return ch;
}

static inline float
get_sound_bleed (const SordNode *sound_node, SordModel *sord,
                 const KitFactory *factory)
{
  float bleed = 1;
  SordNode *bleed_node = sord_get (sord, sound_node, factory->uris.prop_bleed,
                                   NULL, NULL);
  if (bleed_node)
    {
      if (node_is_float (bleed_node))
        bleed = node_to_float (bleed_node);
      sord_node_free (sord_get_world (sord), bleed_node);
    }

  if (bleed > 1)
    bleed = 1;
  else if (bleed < 0)
    bleed = 0;
  return bleed;
}

static inline PcktSample *
load_sample (const SordNode *file_node, const KitFactory *factory)
{
  if (sord_node_get_type (file_node) != SORD_LITERAL)
    return NULL;

  const char *filename = (const char *) sord_node_get_string (file_node);
  size_t flen = strlen (filename), dlen = strlen (factory->base_dir);
  char localized[flen + 1]; // + \0
  char fullpath[dlen + 1 + flen + 1]; // + DIR_SEP + \0
  char *dirsep;

  if (flen == 0)
    return NULL;

  strcpy (localized, filename);
  while ((dirsep = strchr (localized, BAD_DIR_SEP)))
    *dirsep = DIR_SEP;

  sprintf (fullpath, "%s%c%s", factory->base_dir, DIR_SEP, localized);
  return pckt_sample_factory (fullpath);
}

static inline void
load_drum_samples (PcktDrum *drum, PcktChannel channel,
                   const SordNode *sound_node, SordModel *sord,
                   const KitFactory *factory)
{
  SordIter *sample_it = sord_search (sord, sound_node,
                                     factory->uris.prop_sample, NULL, NULL);
  if (!sample_it)
    return;

  for (; !sord_iter_end (sample_it); sord_iter_next (sample_it))
    {
      const SordNode *sample_node = sord_iter_get_node (sample_it, SORD_OBJECT);
      SordIter *file_it = sord_search (sord, sample_node,
                                       factory->uris.prop_file, NULL, NULL);
      if (!file_it)
        continue;

      for (; !sord_iter_end (file_it); sord_iter_next (file_it))
        {
          const SordNode *file_node = sord_iter_get_node (file_it, SORD_OBJECT);
          PcktSample *sample = load_sample (file_node, factory);
          if (sample)
            pckt_drum_add_sample (drum, sample, channel);
        }        
      sord_iter_free (file_it);
    }
  sord_iter_free (sample_it);
}

static inline PcktDrum *
load_drum (const SordNode *drum_node, SordModel *sord,
           const KitFactory *factory)
{
  PcktDrum *drum = NULL;
  SordIter *sound_it = sord_search (sord, drum_node, factory->uris.prop_sound,
                                    NULL, NULL);
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
      if (!sord_contains (sord, pat))
        continue;

      PcktChannel ch = get_sound_channel (sound_node, sord, factory);
      if (ch == PCKT_NCHANNELS)
        continue; // invalid channel

      float bleed = get_sound_bleed (sound_node, sord, factory);
      pckt_drum_set_bleed (drum, ch, bleed);

      load_drum_samples (drum, ch, sound_node, sord, factory);
    }
  sord_iter_free (sound_it);

  return drum;
}

static inline int8_t
get_drum_id (const SordNode *drum_node, SordModel *sord,
             const KitFactory *factory)
{
  int8_t id = -1;
  SordNode *key = sord_get (sord, drum_node, factory->uris.prop_key, NULL,
                            NULL);
  if (key)
    {
      if (node_is_int (key))
        {
          int32_t i = node_to_int (key);
          if (i > 0 && i <= INT8_MAX)
            id = i;
        }
      sord_node_free (sord_get_world (sord), key);
    }
  return id;
}

static PcktKit *
load_kit (const SordNode *kit_node, SordModel *sord, const KitFactory *factory)
{
  PcktKit *kit = pckt_kit_new ();
  SordIter *it = sord_search (sord, NULL, factory->uris.prop_kit, kit_node,
                              NULL);
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
      if (sord_contains (sord, pat))
        {
          int8_t id = get_drum_id (drum_node, sord, factory);
          if (id < 0 || pckt_kit_get_drum (kit, id))
            continue; // invalid or occupied ID
          PcktDrum *drum = load_drum (drum_node, sord, factory);
          if (drum)
            pckt_kit_add_drum (kit, drum, id);
        }
    }
  sord_iter_free (it);

  return kit;
}

static void
init_mics (SordModel *sord, KitFactory *factory)
{
  PcktChannel ch;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    factory->mics[ch] = NULL;

  SordIter *mic_it = sord_search (sord, NULL, factory->uris.prop_type,
                                  factory->uris.class_mic, NULL);
  if (!mic_it)
    return; // no mics defined

  for (; !sord_iter_end (mic_it); sord_iter_next (mic_it))
    {
      const SordNode *mic_node = sord_iter_get_node (mic_it, SORD_SUBJECT);
      SordNode *ch_node = sord_get (sord, mic_node, factory->uris.prop_channel,
                                    NULL, NULL);
      if (ch_node)
        {
          if (node_is_int (ch_node))
            {
              int32_t i = node_to_int (ch_node);
              if (i >= PCKT_CH0 && i < PCKT_NCHANNELS)
                factory->mics[i] = mic_node;
            }
          sord_node_free (sord_get_world (sord), ch_node);
        }
    }
  sord_iter_free (mic_it);
}

static SordModel *
load_sord (FILE *fd, const uint8_t *filename)
{
  SordWorld *world = sord_world_new ();
  SordModel *sord = sord_new (world, SORD_OPS | SORD_SPO, 0);
  SerdURI  base_uri = SERD_URI_NULL;
  SerdNode root = serd_node_new_file_uri (filename, NULL, &base_uri, 0);
  SerdEnv *env = serd_env_new (&root);
  SerdReader *reader = sord_new_reader (sord, env, SERD_TURTLE, NULL);
  SerdStatus status = serd_reader_read_file_handle (reader, fd, filename);
  serd_reader_free (reader);
  serd_env_free (env);
  serd_node_free (&root);

  if (status > SERD_FAILURE)
    {
      sord_free (sord);
      sord_world_free (world);
      return NULL;
    }

  return sord;
}

PcktKit *
pckt_kit_factory (const char *filename)
{
  PcktKit *kit = NULL;
  FILE *fd = fopen (filename, "rb");
  if (!fd)
    return kit;

  char *abspath = realpath (filename, NULL);
  if (!abspath)
    return kit;

  SordModel *sord = load_sord (fd, (uint8_t *) abspath);
  fclose (fd);
  if (!sord)
    {
      free (abspath);
      return kit;
    }

  SordWorld *world = sord_get_world (sord);
  KitFactory factory = {
    dirname (abspath),
    {
      sord_new_uri (world, (const uint8_t *) URI_PCKT "Drum"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "Kit"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "Mic"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "Sample"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "Sound"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "bleed"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "channel"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "file"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "kit"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "key"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "mic"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "sample"),
      sord_new_uri (world, (const uint8_t *) URI_PCKT "sound"),
      sord_new_uri (world, (const uint8_t *) URI_SNTX "type")
    },
    {}
  };
  init_mics (sord, &factory);

  SordNode *kit_node = sord_get (sord, NULL, factory.uris.prop_type,
                                 factory.uris.class_kit, NULL);
  if (kit_node)
    {
      kit = load_kit (kit_node, sord, &factory);
      sord_node_free (world, kit_node);
    }

  sord_free (sord);
  sord_world_free (world);
  free (abspath);

  return kit;
}
