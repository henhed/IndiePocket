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

#ifndef PCKT_KIT_H
#define PCKT_KIT_H 1

#include <stdint.h>
#include "pckt.h"
#include "drum.h"
#include "sound.h"

#define PCKT_KIT_EACH_DRUM_META(kit, iter)                              \
  for (PcktDrumMeta *(iter) = pckt_kit_next_drum_meta ((kit), NULL);    \
       (iter) != NULL;                                                  \
       (iter) = pckt_kit_next_drum_meta ((kit), (iter)))

__BEGIN_DECLS

typedef struct PcktKitImpl PcktKit;

extern PcktKit *pckt_kit_new ();
extern void pckt_kit_free (PcktKit *);
extern int8_t pckt_kit_add_drum (PcktKit *, PcktDrum *, int8_t);
extern PcktDrum *pckt_kit_get_drum (const PcktKit *, int8_t);
extern int8_t pckt_kit_add_drum_meta (PcktKit *, PcktDrumMeta *);
extern PcktDrumMeta *pckt_kit_get_drum_meta (const PcktKit *, int8_t);
extern PcktDrumMeta *pckt_kit_next_drum_meta (const PcktKit *,
                                              const PcktDrumMeta *);
extern bool pckt_kit_set_choke (PcktKit *, int8_t, int8_t, bool);
extern bool pckt_kit_choke_by_id (const PcktKit *, PcktSoundPool *, int8_t);

__END_DECLS

#endif /* ! PCKT_KIT_H */
