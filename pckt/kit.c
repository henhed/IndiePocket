#include <stdlib.h>
#include <string.h>
#include "kit.h"

struct PcktKitImpl
{
  PcktDrum *drums[INT8_MAX + 1];
  bool chokemap[INT8_MAX + 1][INT8_MAX + 1];
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
  for (int8_t i = INT8_MAX; i >= 0; --i)
    {
      if (kit->drums[i])
        pckt_drum_free (kit->drums[i]);
    }
  free (kit);
}

int8_t
pckt_kit_add_drum (PcktKit *kit, PcktDrum *drum, int8_t id)
{
  if (!kit || !drum || id < 0)
    return -1;

  for (int8_t i = INT8_MAX - 1; i >= 0; --i)
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

  for (int8_t chokee = INT8_MAX; chokee >= 0; --chokee)
    {
      if (kit->drums[chokee] != NULL && kit->chokemap[choker][chokee] == true)
        pckt_soundpool_choke (pool, kit->drums[chokee]);
    }

  return true;
}
