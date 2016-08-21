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

#ifndef PCKT_SAMPLE_H
#define PCKT_SAMPLE_H 1

#include <stdlib.h>
#include "pckt.h"

#define PCKT_SAMPLE_RATE_DEFAULT 44100

__BEGIN_DECLS

typedef struct PcktSampleImpl PcktSample;
typedef enum {
  PCKT_INTRPL_NONE = 0,
  PCKT_INTRPL_CONSTANT,
  PCKT_INTRPL_LINEAR
} PcktInterpolation;

extern PcktSample *pckt_sample_new ();
extern void pckt_sample_free (PcktSample *);
extern uint32_t pckt_sample_rate (PcktSample *, uint32_t);
extern bool pckt_sample_set_interpolation (PcktSample *, PcktInterpolation);
extern size_t pckt_sample_read (const PcktSample *, float *, size_t, size_t,
                                uint32_t);
extern size_t pckt_sample_write (PcktSample *, const float *, size_t);
extern bool pckt_sample_resize (PcktSample *, size_t);
extern bool pckt_sample_merge (PcktSample *, const PcktSample *, float, float);
extern float pckt_sample_normalize (PcktSample *);
extern bool pckt_resample (PcktSample *, uint32_t);
extern PcktSample *pckt_sample_factory_mono (const char *);
extern PcktSample **pckt_sample_factory (const char *, size_t *);

__END_DECLS

#endif /* ! PCKT_SAMPLE_H */
