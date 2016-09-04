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

#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <ctype.h>
#include <sys/stat.h>
#include <glob.h>
#include <math.h>
#include "kit_factory.h"
#include "kit.h"
#include "drum.h"
#include "sample.h"
#include "util.h"

typedef struct {
  char *key, *val;
} StringPair;

typedef enum {
  BFK_KICK = 0,
  BFK_SNARE,
  BFK_HIHAT,
  BFK_TOM1,
  BFK_TOM2,
  BFK_TOM3,
  BFK_CYM1,
  BFK_CYM2,
  BFK_CYM3,
  BFK_NUM_TYPES,
} BfkDrumType;

typedef enum {
  BFK_CH_OH1 = 0,
  BFK_CH_OH2,
  BFK_CH_ROOM1,
  BFK_CH_ROOM2,
  BFK_CH_PZM1,
  BFK_CH_PZM2,
  BFK_CH_SNARE1,
  BFK_CH_SNARE2,
  BFK_CH_KICK1,
  BFK_CH_KICK2,
  BFK_CH_DIRECT,
  BFK_NUM_CHANNELS
} BfkChannel;

static const PcktChannel channel_map[BFK_NUM_CHANNELS] = {
  PCKT_CH10,
  PCKT_CH11,
  PCKT_CH14,
  PCKT_CH15,
  PCKT_CH14,
  PCKT_CH15,
  PCKT_CH2,
  PCKT_CH3,
  PCKT_CH0,
  PCKT_CH1,
  PCKT_NCHANNELS
};

typedef struct {
  BfkDrumType id;
  PcktChannel channel;
  const char *bfk_key;
  const char *info_key;
} BfkDrumKeys;

static const BfkDrumKeys drum_key_map[BFK_NUM_TYPES] = {
  { BFK_KICK,  PCKT_NCHANNELS, "KICK",  "Kick"      },
  { BFK_SNARE, PCKT_NCHANNELS, "SNARE", "Snare"     },
  { BFK_HIHAT, PCKT_CH4,       "HIHAT", "Hihat"     },
  { BFK_TOM1,  PCKT_CH8,       "TOM1",  "Floor Tom" },
  { BFK_TOM2,  PCKT_CH7,       "TOM2",  "Mid Tom"   },
  { BFK_TOM3,  PCKT_CH6,       "TOM3",  "High Tom"  },
  { BFK_CYM1,  PCKT_CH12,      "CYM1",  "Cymbal 1"  },
  { BFK_CYM2,  PCKT_CH13,      "CYM2",  "Cymbal 2"  },
  { BFK_CYM3,  PCKT_CH5,       "CYM3",  "Cymbal 3"  }
};

typedef enum {
  BFK_KICK_NO_SNARE = 0,
  BFK_KICK_HIT,
  BFK_SNARE_HIT,
  BFK_SNARE_DRAG,
  BFK_SNARE_FLAM,
  BFK_SNARE_RIMSHOT,
  BFK_SNARE_SIDESTICK,
  BFK_HIHAT_CLOSED_TIP,
  BFK_HIHAT_CLOSED_SHANK,
  BFK_HIHAT_HALF_TIP,
  BFK_HIHAT_HALF_SHANK,
  BFK_HIHAT_OPEN_TIP,
  BFK_HIHAT_PEDAL,
  BFK_TOM1_HIT,
  BFK_TOM2_HIT,
  BFK_TOM3_HIT,
  BFK_CYM1_HIT,
  BFK_CYM2_HIT,
  BFK_CYM3_HIT,
  BFK_NUM_HIT_TYPES
} BfkDrumHitType;

typedef struct {
  BfkDrumHitType id;
  BfkDrumType drum;
  const char *name;
  int8_t midi_key;
  int8_t gate_key;
} BfkDrumHit;

static const BfkDrumHit drum_hit_info[BFK_NUM_HIT_TYPES] = {
  { BFK_KICK_NO_SNARE,      BFK_KICK,  "NoSnare", 35, -1 },
  { BFK_KICK_HIT,           BFK_KICK,  "Hit",     36, -1 },
  { BFK_SNARE_HIT,          BFK_SNARE, "Hit",     38, -1 },
  { BFK_SNARE_DRAG,         BFK_SNARE, "Drag",    39, -1 },
  { BFK_SNARE_FLAM,         BFK_SNARE, "Flam",    41, -1 },
  { BFK_SNARE_RIMSHOT,      BFK_SNARE, "Rim",     40, -1 },
  { BFK_SNARE_SIDESTICK,    BFK_SNARE, "SS",      37, -1 },
  { BFK_HIHAT_CLOSED_TIP,   BFK_HIHAT, "ClosedT", 42, -1 },
  { BFK_HIHAT_CLOSED_SHANK, BFK_HIHAT, "ClosedS", 48, -1 },
  { BFK_HIHAT_HALF_TIP,     BFK_HIHAT, "HalfT",   50, -1 },
  { BFK_HIHAT_HALF_SHANK,   BFK_HIHAT, "HalfS",   52, -1 },
  { BFK_HIHAT_OPEN_TIP,     BFK_HIHAT, "OpenT",   46, -1 },
  { BFK_HIHAT_PEDAL,        BFK_HIHAT, "Pedal",   44, -1 },
  { BFK_TOM1_HIT,           BFK_TOM1,  "Hit",     43, -1 },
  { BFK_TOM2_HIT,           BFK_TOM2,  "Hit",     45, -1 },
  { BFK_TOM3_HIT,           BFK_TOM3,  "Hit",     47, -1 },
  { BFK_CYM1_HIT,           BFK_CYM1,  "Hit",     49, 54 },
  { BFK_CYM2_HIT,           BFK_CYM2,  "Hit",     55, 56 },
  { BFK_CYM3_HIT,           BFK_CYM3,  "Hit",     51, 58 }
};

typedef struct {
  const BfkDrumKeys *keys;
  char *name;
  char *path;
  float gain;
} BfkDrumInfo;

typedef struct {
  PcktKitParserIface iface;
  const PcktKitFactory *factory;
  BfkDrumInfo drums[BFK_NUM_TYPES];
} BfkParser;

static inline bool
read_next_pair (FILE *fd, char delim, char **key_out, char **val_out)
{
  char line[1024];
  char *c, *key_start, *key_end, *val_start, *val_end;
  bool quoted;

  if (!fgets (line, sizeof (line), fd))
    return false;

  c = line;
  while (isspace (*c))
    ++c;

  if ((*c == '\0') || (*c == '#'))
    return read_next_pair (fd, delim, key_out, val_out);
  else if ((*c == delim) || !isprint (*c))
    return false;

  key_start = c;
  while ((*c != delim) && (*c != '\0'))
    ++c;
  key_end = c;
  while (isspace (*(key_end - 1)))
    --key_end;

  if (*c != delim)
    return false;
  else
    ++c;

  while (isspace (*c))
    ++c;

  quoted = (*c == '"');
  if (quoted)
    ++c;

  val_start = c;
  for (c = line + strlen (line); c > val_start; --c)
    {
      if ((quoted && (*c == '"'))
          || (!quoted && !isspace (*(c - 1))))
        break;
    }
  val_end = c;

  *key_end = '\0';
  *val_end = '\0';

  if (key_out)
    *key_out = strdup (key_start);

  if (val_out)
    *val_out = strdup (val_start);

  return true;
}

static StringPair **
file_get_pairs (const char *filename, char delim, size_t nlines)
{
  StringPair **pairs, **current;
  FILE *fd = fopen (filename, "r");

  if (!fd)
    return NULL;

  pairs = calloc (nlines + 1, sizeof (StringPair *));
  if (!pairs)
    {
      fclose (fd);
      return NULL;
    }

  current = pairs;
  while (nlines-- > 0)
    {
      StringPair pair;
      if (!read_next_pair (fd, delim, &pair.key, &pair.val))
        break;
      *current = malloc (sizeof (StringPair));
      memcpy (*current, &pair, sizeof (StringPair));
      ++current;
    }

  fclose (fd);

  return pairs;
}

static void
free_pairs (StringPair **pairs)
{
  if (!pairs)
    return;

  StringPair **pair = pairs;
  while (*pair)
    {
      if ((*pair)->key)
        free ((*pair)->key);
      if ((*pair)->val)
        free ((*pair)->val);
      free (*pair);
      ++pair;
    }

  free (pairs);
}

static char *
get_info_filename (const BfkParser *parser)
{
  const char *filename = pckt_kit_factory_get_filename (parser->factory);
  const char *ext = strrchr (filename, '.');
  if (!ext)
    return NULL;

  char *noext = strdup (filename);
  if (!noext)
    return NULL;

  *strrchr (noext, '.') = '\0';

  char *dotinfo = pckt_strdupf ("%s.info", noext);
  if (!dotinfo)
    {
      free (noext);
      return NULL;
    }

  free (noext);

  return dotinfo;
}

static char *
get_data_path (const BfkParser *parser, const char *path)
{
  int maxdepth = 5;
  char *basedir = NULL, *_basedir = NULL;
  char *relpath = NULL;
  char *abspath = NULL;

  _basedir = strdup (pckt_kit_factory_get_basedir (parser->factory));
  basedir = _basedir;
  if (!basedir)
    return NULL;

  relpath = pckt_strdupf ("Data%c%s%c", PCKT_DIR_SEP, path, PCKT_DIR_SEP);
  if (!relpath)
    {
      free (basedir);
      return NULL;
    }

  while (maxdepth-- > 0)
    {
      int err;
      struct stat st;
      abspath = pckt_strdupf ("%s%c%s", basedir, PCKT_DIR_SEP, relpath);
      err = stat (abspath, &st);
      if (!err && S_ISDIR (st.st_mode))
        break;
      free (abspath);
      abspath = NULL;
      basedir = dirname (basedir);
    }

  free (relpath);
  free (_basedir);

  return abspath;
}

static inline void
init_drum_gain (const BfkParser *parser, BfkDrumInfo *info)
{
  (void) parser;

  char *filename = pckt_strdupf ("%stweaks.txt", info->path);
  StringPair **tweaks = NULL;

  info->gain = 1.f;

  if (!filename)
    return;

  tweaks = file_get_pairs (filename, '=', 10);
  free (filename);
  if (!tweaks)
    return;

  for (StringPair **tweak = tweaks; *tweak != NULL; ++tweak)
    {
      if (strcmp ((*tweak)->key, "Gain")
          || !(*tweak)->val || !strlen ((*tweak)->val))
        continue;

      char *endptr = NULL;
      float db = pckt_strtof ((*tweak)->val, &endptr);
      while (isspace (*endptr))
        ++endptr;
      if (!db || (strstr (endptr, "dB") != endptr))
        continue;

      info->gain *= powf (powf (2, (1.f / 3)), db);

      break;
    }

  free_pairs (tweaks);
}

static bool
init_drum_info (BfkParser *parser)
{
  StringPair **paths = NULL;
  StringPair **names = NULL;
  const char *filename = pckt_kit_factory_get_filename (parser->factory);
  char *dotinfo;
  size_t ndrums = 0;

  paths = file_get_pairs (filename, '=', BFK_NUM_TYPES);
  if (!paths)
    return false;

  dotinfo = get_info_filename (parser);
  if (dotinfo)
    {
      names = file_get_pairs (dotinfo, ':', BFK_NUM_TYPES);
      free (dotinfo);
      dotinfo = NULL;
    }

  for (BfkDrumType t = 0; t < BFK_NUM_TYPES; ++t)
    {
      BfkDrumInfo *info = &parser->drums[t];
      info->keys = &drum_key_map[t];
      for (StringPair **pair = paths; *pair != NULL; ++pair)
        {
          if (!strcmp ((*pair)->key, info->keys->bfk_key)
              && (*pair)->val && strlen ((*pair)->val))
            {
              info->path = get_data_path (parser, pckt_fix_path ((*pair)->val));
              if (info->path)
                {
                  init_drum_gain (parser, info);
                  ++ndrums;
                }
              break;
            }
        }

      if (!names)
        continue;

      for (StringPair **pair = names; *pair != NULL; ++pair)
        {
          if (!strcmp ((*pair)->key, info->keys->info_key)
              && (*pair)->val && strlen ((*pair)->val))
            {
              info->name = strdup ((*pair)->val);
              break;
            }
        }
    }

  if (names)
    free_pairs (names);
  free_pairs (paths);

  return (ndrums > 0) ? true : false;
}

static inline void
load_drum_samples (const BfkParser *parser, PcktDrum *drum,
                   const char *filename, const BfkDrumInfo *info)
{
  (void) parser;

  size_t nchannels = 0;
  PcktSample **samples = pckt_sample_factory (filename, &nchannels);
  if (!samples)
    return;

  PcktSample *mapped[PCKT_NCHANNELS];
  memset (mapped, 0, sizeof (PcktSample *) * PCKT_NCHANNELS);

  for (uint8_t ch = 0; ch < nchannels; ++ch)
    {
      if (ch >= BFK_NUM_CHANNELS)
        {
          pckt_sample_free (samples[ch]);
          continue;
        }

      PcktChannel mch = ((ch == BFK_CH_DIRECT)
                         ? info->keys->channel
                         : channel_map[ch]);

      if (mch == PCKT_NCHANNELS)
        {
          /* Direct channel (10) is silent for Kick and Snare samples.  */
          pckt_sample_free (samples[ch]);
          continue;
        }
      else if (!mapped[mch])
        mapped[mch] = samples[ch];
      else
        {
          pckt_sample_merge (mapped[mch], samples[ch], 0.5, 0.5);
          pckt_sample_free (samples[ch]);
        }
    }

  for (PcktChannel ch = 0; ch < PCKT_NCHANNELS; ++ch)
    {
      if (!mapped[ch])
        continue;

      float bleed = 1.f;
      if ((info->keys->id == BFK_KICK && (ch == PCKT_CH0 || ch == PCKT_CH1))
          || (info->keys->id == BFK_SNARE && (ch == PCKT_CH2 || ch == PCKT_CH3))
          || ch == info->keys->channel)
        bleed = info->gain;

      if (pckt_drum_add_sample (drum, mapped[ch], ch, filename))
        pckt_drum_set_bleed (drum, ch, bleed);
      else
        pckt_sample_free (mapped[ch]);
    }

  free (samples);
}

static inline PcktDrum *
load_drum_hit (const BfkParser *parser, const BfkDrumInfo *info,
               const BfkDrumHit *bfk_hit)
{
  PcktDrum *drum;
  glob_t globbuf;
  char *globpat = pckt_strdupf ("%s%s%cmaster*.wav", info->path,
                                bfk_hit->name, PCKT_DIR_SEP);
  if (!globpat)
    return NULL;

  if (glob (globpat, 0, NULL, &globbuf) != 0)
    {
      free (globpat);
      return NULL;
    }

  drum = pckt_drum_new ();

  if (drum)
    {
      for (char **path = globbuf.gl_pathv; *path != NULL; ++path)
        load_drum_samples (parser, drum, *path, info);
    }

  globfree (&globbuf);
  free (globpat);

  return drum;
}

static inline void
get_drum_hit_chokers (const BfkDrumHit *hit, int8_t **chokers, size_t *nchokers)
{
  *nchokers = 0;
  *chokers = calloc (5, sizeof (int8_t));

  if (!*chokers)
    return;

  switch (hit->id)
    {
    case BFK_HIHAT_OPEN_TIP:
      (*chokers)[(*nchokers)++] = drum_hit_info[BFK_HIHAT_HALF_TIP].midi_key;
      (*chokers)[(*nchokers)++] = drum_hit_info[BFK_HIHAT_HALF_SHANK].midi_key;
      /* break intentially omitted.  */

    case BFK_HIHAT_HALF_TIP:
    case BFK_HIHAT_HALF_SHANK:
      (*chokers)[(*nchokers)++] = drum_hit_info[BFK_HIHAT_CLOSED_TIP].midi_key;
      (*chokers)[(*nchokers)++] = drum_hit_info[BFK_HIHAT_CLOSED_SHANK].midi_key;
      (*chokers)[(*nchokers)++] = drum_hit_info[BFK_HIHAT_PEDAL].midi_key;
      break;

    default:
      if (hit->gate_key >= 0)
        (*chokers)[(*nchokers)++] = hit->gate_key;
      break;
    }

  if (*nchokers == 0)
    {
      free (*chokers);
      *chokers = NULL;
    }
}

static PcktStatus
bfk_parser_load_metas (PcktKitParserIface *iface, PcktKitFactory *factory,
                       PcktKitFactoryDrumMetaCb callback)
{
  BfkParser *parser = (BfkParser *) iface;

  for (BfkDrumType t = 0; t < BFK_NUM_TYPES; ++t)
    {
      BfkDrumInfo *info = &parser->drums[t];
      if (info->path)
        {
          PcktDrumMeta *meta = pckt_drum_meta_new (info->name);
          if (meta)
            callback (factory, meta, info);
        }
    }

  return PCKTE_SUCCESS;
}

PcktStatus
bfk_parser_load_drums (PcktKitParserIface *iface, PcktDrumMeta *meta,
                       const void *handle, PcktKitFactoryDrumCb callback,
                       void *user_handle)
{
  BfkParser *parser = (BfkParser *) iface;
  const BfkDrumInfo *info = (const BfkDrumInfo *) handle;

  for (uint8_t i = 0; i < BFK_NUM_HIT_TYPES; ++i)
    {
      const BfkDrumHit *hit = &drum_hit_info[i];
      if (hit->drum == info->keys->id)
        {
          PcktDrum *drum = load_drum_hit (parser, info, hit);
          if (drum)
            {
              int8_t *chokers = NULL;
              size_t nchokers = 0;

              get_drum_hit_chokers (hit, &chokers, &nchokers);

              pckt_drum_set_meta (drum, meta);
              pckt_drum_normalize (drum);

              callback (user_handle, drum, hit->midi_key, chokers, nchokers);

              if (chokers)
                free (chokers);
            }
        }
    }

  return PCKTE_SUCCESS;
}

static void
bfk_parser_free (PcktKitParserIface *iface, const PcktKitFactory *factory)
{
  (void) factory;

  if (!iface)
    return;

  BfkParser *parser = (BfkParser *) iface;
  for (BfkDrumType t = 0; t < BFK_NUM_TYPES; ++t)
    {
      BfkDrumInfo *info = &parser->drums[t];
      if (info->name)
        free (info->name);
      if (info->path)
        free (info->path);
    }

  free (parser);
}

PcktKitParserIface *
pckt_kit_parser_bfk_new (const PcktKitFactory *factory)
{
  BfkParser *parser;
  PcktKitParserIface *iface;

  if (!factory)
    return NULL;

  parser = malloc (sizeof (BfkParser));
  if (!parser)
    return NULL;

  memset (parser, 0, sizeof (BfkParser));

  iface = (PcktKitParserIface *) parser;
  iface->load_metas = bfk_parser_load_metas;
  iface->load_drums = bfk_parser_load_drums;
  iface->free = bfk_parser_free;

  parser->factory = factory;

  if (!init_drum_info (parser))
    {
      free (parser);
      iface = NULL;
    }

  return iface;
}
