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
#include <math.h>
#include "drum.h"
#include "sample.h"

#define MAX_NUM_SAMPLES 32
#define TWELFTH_ROOT_OF_TWO 1.05946309435929526

typedef struct {
  PcktSample *sample;
  char *name;
} PcktDrumSample;

struct PcktDrumImpl
{
  const PcktDrumMeta *meta;
  PcktDrumSample samples[PCKT_NCHANNELS][MAX_NUM_SAMPLES];
  size_t nsamples[PCKT_NCHANNELS];
  float bleed[PCKT_NCHANNELS];
  float overlap;
};

struct PcktDrumMetaImpl
{
  char *name;
  float tuning;
  float dampening;
  float expression;
};

PcktDrum *
pckt_drum_new ()
{
  PcktDrum *drum = malloc (sizeof (PcktDrum));
  if (drum)
    memset (drum, 0, sizeof (PcktDrum));
  return drum;
}

void
pckt_drum_free (PcktDrum *drum)
{
  if (!drum)
    return;

  for (PcktChannel ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    {
      for (uint8_t i = 0; i < drum->nsamples[ch]; ++i)
        {
          if (drum->samples[ch][i].sample)
            pckt_sample_free (drum->samples[ch][i].sample);

          if (drum->samples[ch][i].name)
            free (drum->samples[ch][i].name);
        }
    }

  free (drum);
}

bool
pckt_drum_set_bleed (PcktDrum *drum, PcktChannel ch, float bleed)
{
  if (!drum || ch < PCKT_CH0 || ch >= PCKT_NCHANNELS || bleed < 0)
    return false;
  drum->bleed[ch] = bleed;
  return true;
}

bool
pckt_drum_set_meta (PcktDrum *drum, const PcktDrumMeta *meta)
{
  if (!drum)
    return false;
  drum->meta = meta;
  return true;
}

bool
pckt_drum_set_sample_overlap (PcktDrum *drum, float overlap)
{
  if (!drum || overlap < 0)
    return false;
  drum->overlap = overlap;
  return true;
}

static int
drum_sample_cmp (const void *lhs, const void *rhs)
{
  PcktDrumSample *a = (PcktDrumSample *) lhs;
  PcktDrumSample *b = (PcktDrumSample *) rhs;
  if (!a->name)
    return b->name ? -1 : 0;
  else if (!b->name)
    return 1;
  else
    return strcmp (a->name, b->name);
}

bool
pckt_drum_add_sample (PcktDrum *drum, PcktSample *sample, PcktChannel ch,
                      const char *name)
{
  if (!drum || !sample || ch < PCKT_CH0 || ch >= PCKT_NCHANNELS
      || drum->nsamples[ch] >= MAX_NUM_SAMPLES)
    return false;

  PcktDrumSample *ds = &drum->samples[ch][drum->nsamples[ch]++];
  ds->sample = sample;
  if (!name)
    ds->name = NULL;
  else
    {
      ds->name = malloc (strlen (name) + 1);
      strcpy (ds->name, name);
    }

  qsort (drum->samples[ch], drum->nsamples[ch], sizeof (PcktDrumSample),
         drum_sample_cmp);

  return true;
}

static inline PcktSample *
get_sample_for_hit (const PcktDrum *drum, PcktChannel ch, float force,
                    float random)
{
  size_t nsamples = drum->nsamples[ch];
  if (nsamples == 0)
    return NULL;
  else if (nsamples == 1)
    return drum->samples[ch][0].sample;

  uint8_t index;
  float probability = 0;
  float scale = 1.f + drum->overlap;
  float weights[nsamples];
  float weight_sum = 0;
  float width = 1.f / nsamples;

  /* Gather sample weights based on range and proximity to given FORCE.  */
  for (index = 0; index < nsamples; ++index)
    {
      float center = (index * width) + (width / 2);
      float range = (width / 2) * scale;
      float weight = (range - fabsf (center - force)) / range;
      if (weight > 0)
        {
          weight_sum += weight;
          weights[index] = weight;
        }
      else
        weights[index] = 0;
    }

  /* Test RANDOM against each weighed samples cumulative probability.  */
  for (index = 0; index < nsamples; ++index)
    {
      if (weights[index] <= 0)
        continue;

      probability += weights[index] / weight_sum;

      if (random <= probability)
        break;
    }

  if (index >= nsamples)
    index = nsamples - 1;

  return drum->samples[ch][index].sample;
}

bool
pckt_drum_hit (const PcktDrum *drum, PcktSound *sound, float force)
{
  if (!drum || !sound || !pckt_sound_clear (sound))
    return false;

  sound->source = drum;
  if (force <= 0)
    return true;

  if (drum->meta && drum->meta->expression != 0)
    {
      /* Raise FORCE to the power of EXP, where INF > EXP > 1 for negative
         expression values and 1 > EXP > 0 for positive expression values.
         The .8 factor of the exponent is arbitrarily chosen as more extreme
         values doesn't sound useful.  */
      float exp = 1.f - fabs (drum->meta->expression * 0.8);
      if (drum->meta->expression < 0)
        exp = 1.f / exp;

      force = powf (force, exp);
    }

  PcktChannel ch;
  float bleed;
  float random = (float) rand () / RAND_MAX;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    {
      bleed = drum->bleed[ch] * force;
      if (bleed <= 0) /* Channel is muted.  */
        continue;
      sound->bleed[ch] = bleed;
      sound->samples[ch] = get_sample_for_hit (drum, ch, force, random);
    }

  sound->impact = force;

  if (drum->meta && drum->meta->tuning != 0)
    sound->pitch = powf (TWELFTH_ROOT_OF_TWO, drum->meta->tuning);

  if (drum->meta && drum->meta->dampening > 0 && drum->meta->dampening <= 1)
    {
      sound->smoothness = drum->meta->dampening;
      /* Stiffness is arbitrarily squared to scale nicely with smoothness.  */
      sound->stiffness = drum->meta->dampening * drum->meta->dampening;
    }

  return true;
}

PcktDrumMeta *
pckt_drum_meta_new (const char *name)
{
  PcktDrumMeta *meta = malloc (sizeof (PcktDrumMeta));
  if (meta)
    {
      memset (meta, 0, sizeof (PcktDrumMeta));
      if (name)
        {
          size_t len = strlen (name);
          meta->name = malloc (len + 1);
          if (meta->name)
            {
              meta->name[len] = '\0';
              strncpy (meta->name, name, len);
            }
          else
            {
              free (meta);
              meta = NULL;
            }
        }
    }
  return meta;
}

void
pckt_drum_meta_free (PcktDrumMeta *meta)
{
  if (meta)
    {
      if (meta->name)
        free (meta->name);
      free (meta);
    }
}

const char *
pckt_drum_meta_get_name (const PcktDrumMeta *meta)
{
  return (meta && meta->name) ? meta->name : NULL;
}

float
pckt_drum_meta_get_tuning (const PcktDrumMeta *meta)
{
  return meta ? meta->tuning : 0;
}

bool
pckt_drum_meta_set_tuning (PcktDrumMeta *meta, float tuning)
{
  if (!meta)
    return false;
  meta->tuning = tuning;
  return true;
}

float
pckt_drum_meta_get_dampening (const PcktDrumMeta *meta)
{
  return meta ? meta->dampening : 0;
}

bool
pckt_drum_meta_set_dampening (PcktDrumMeta *meta, float dampening)
{
  if (!meta || dampening < 0 || dampening > 1)
    return false;
  meta->dampening = dampening;
  return true;
}

float
pckt_drum_meta_get_expression (const PcktDrumMeta *meta)
{
  return meta ? meta->expression : 0;
}

bool
pckt_drum_meta_set_expression (PcktDrumMeta *meta, float expression)
{
  if (!meta || expression < -1 || expression > 1)
    return false;
  meta->expression = expression;
  return true;
}
