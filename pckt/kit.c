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

#include <stdlib.h>
#include <string.h>
#include "kit.h"

#define MAX_NUM_DRUMS (INT8_MAX + 1)

struct PcktKitImpl
{
  PcktDrum *drums[MAX_NUM_DRUMS];
  PcktDrumMeta *drum_metas[MAX_NUM_DRUMS];
  bool chokemap[MAX_NUM_DRUMS][MAX_NUM_DRUMS];
};

PcktKit *
pckt_kit_new ()
{
  PcktKit *kit = malloc (sizeof (PcktKit));
  if (kit)
    memset (kit, 0, sizeof (PcktKit));
  return kit;
}

void
pckt_kit_free (PcktKit *kit)
{
  if (!kit)
    return;
  for (int8_t i = MAX_NUM_DRUMS - 1; i >= 0; --i)
    {
      if (kit->drums[i])
        pckt_drum_free (kit->drums[i]);
      if (kit->drum_metas[i])
        pckt_drum_meta_free (kit->drum_metas[i]);
    }
  free (kit);
}

int8_t
pckt_kit_add_drum (PcktKit *kit, PcktDrum *drum, int8_t id)
{
  if (!kit || !drum || id < 0)
    return -1;

  for (int8_t i = MAX_NUM_DRUMS - 1; i >= 0; --i)
    {
      if (kit->drums[i] == drum)
        return i;
    }

  kit->drums[id] = drum;
  return id;
}

PcktDrum *
pckt_kit_get_drum (const PcktKit *kit, int8_t id)
{
  if (kit && id >= 0)
    return kit->drums[id];
  return NULL;
}

int8_t
pckt_kit_get_drum_meta_id (const PcktKit *kit, const PcktDrumMeta *meta)
{
  if (!kit || !meta)
    return -1;

  for (int8_t i = 0, j = MAX_NUM_DRUMS - 1; j >= 0; ++i, --j)
    {
      if (kit->drum_metas[i] == meta)
        return i;
    }

  return -1;
}

int8_t
pckt_kit_add_drum_meta (PcktKit *kit, PcktDrumMeta *meta)
{
  if (!kit || !meta)
    return -1;

  for (int8_t i = 0, j = MAX_NUM_DRUMS - 1; j >= 0; ++i, --j)
    {
      if (kit->drum_metas[i] == meta)
        return i;
      else if (!kit->drum_metas[i])
        {
          kit->drum_metas[i] = meta;
          return i;
        }
    }

  return -1;
}

PcktDrumMeta *
pckt_kit_get_drum_meta (const PcktKit *kit, int8_t index)
{
  if (kit && index >= 0)
    return kit->drum_metas[index];
  return NULL;
}

PcktDrumMeta *
pckt_kit_next_drum_meta (const PcktKit *kit, const PcktDrumMeta *prev)
{
  if (!kit)
    return NULL;

  PcktDrumMeta *next = NULL;

  for (int8_t i = 0, j = MAX_NUM_DRUMS - 1; j >= 0; ++i, --j)
    {
      if (!kit->drum_metas[i])
        continue;
      else if (!prev)
        {
          next = kit->drum_metas[i];
          break;
        }
      else if (prev == kit->drum_metas[i])
        prev = NULL;
    }

  return next;
}

bool
pckt_kit_set_choke (PcktKit *kit, int8_t choker, int8_t chokee, bool choke)
{
  if (!kit || choker < 0 || chokee < 0)
    return false;
  kit->chokemap[choker][chokee] = choke;
  return true;
}

bool
pckt_kit_choke_by_id (const PcktKit *kit, PcktSoundPool *pool, int8_t choker)
{
  if (!kit || !pool || choker < 0)
    return false;

  for (int8_t chokee = MAX_NUM_DRUMS - 1; chokee >= 0; --chokee)
    {
      if (kit->drums[chokee] != NULL && kit->chokemap[choker][chokee] == true)
        pckt_soundpool_choke (pool, kit->drums[chokee]);
    }

  return true;
}
