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

#ifndef PCKT_DRUM_H
#define PCKT_DRUM_H 1

#include "pckt.h"
#include "sound.h"
#include "sample.h"

__BEGIN_DECLS

typedef struct PcktDrumImpl PcktDrum;
typedef struct PcktDrumMetaImpl PcktDrumMeta;

extern PcktDrum *pckt_drum_new ();
extern void pckt_drum_free (PcktDrum *);
extern bool pckt_drum_set_bleed (PcktDrum *, PcktChannel, float);
extern bool pckt_drum_set_meta (PcktDrum *, const PcktDrumMeta *);
extern bool pckt_drum_set_sample_overlap (PcktDrum *, float);
extern bool pckt_drum_add_sample (PcktDrum *, PcktSample *, PcktChannel,
                                  const char *);
extern bool pckt_drum_hit (const PcktDrum *, PcktSound *, float);
extern PcktDrumMeta *pckt_drum_meta_new (const char *);
extern void pckt_drum_meta_free (PcktDrumMeta *);
extern const char *pckt_drum_meta_get_name (const PcktDrumMeta *);
extern float pckt_drum_meta_get_tuning (const PcktDrumMeta *);
extern bool pckt_drum_meta_set_tuning (PcktDrumMeta *, float);
extern float pckt_drum_meta_get_dampening (const PcktDrumMeta *);
extern bool pckt_drum_meta_set_dampening (PcktDrumMeta *, float);
extern float pckt_drum_meta_get_expression (const PcktDrumMeta *);
extern bool pckt_drum_meta_set_expression (PcktDrumMeta *, float);

__END_DECLS

#endif /* ! PCKT_DRUM_H */
