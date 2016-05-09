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

#ifndef PCKT_SOUND_H
#define PCKT_SOUND_H 1

#include "pckt.h"
#include "sample.h"

#define PCKT_CHOKE_TIME .5f
#define PCKT_STIFF_HL .02f

__BEGIN_DECLS

typedef enum
{
  PCKT_CH0 = 0, PCKT_CH1, PCKT_CH2, PCKT_CH3,
  PCKT_CH4, PCKT_CH5, PCKT_CH6, PCKT_CH7,
  PCKT_CH8, PCKT_CH9, PCKT_CH10, PCKT_CH11,
  PCKT_CH12, PCKT_CH13, PCKT_CH14, PCKT_CH15,
  PCKT_NCHANNELS
} PcktChannel;

typedef struct
{
  PcktSample *samples[PCKT_NCHANNELS];
  float bleed[PCKT_NCHANNELS];
  size_t progress[PCKT_NCHANNELS];
  float tail[PCKT_NCHANNELS];
  float impact;
  float pitch;
  float smoothness;
  float stiffness;
  float variance;
  bool choke;
  const void *source;
} PcktSound;

typedef struct PcktSoundPoolImpl PcktSoundPool;

extern PcktSoundPool *pckt_soundpool_new (size_t);
extern void pckt_soundpool_free (PcktSoundPool *);
extern PcktSound *pckt_soundpool_at (PcktSoundPool *, uint32_t);
extern PcktSound *pckt_soundpool_get (PcktSoundPool *, const void *);
extern bool pckt_soundpool_choke (PcktSoundPool *, const void *);
extern bool pckt_soundpool_clear (PcktSoundPool *);
extern bool pckt_sound_clear (PcktSound *);
extern int32_t pckt_sound_process (PcktSound *, float **, size_t, uint32_t);

__END_DECLS

#endif /* ! PCKT_SOUND_H */
