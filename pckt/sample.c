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
#include "sample.h"

typedef size_t (*PcktInterpolator) (const float *, size_t,
                                    float *, size_t,
                                    float);

struct PcktSampleImpl
{
  uint32_t rate;
  float *frames;
  size_t nframes;
  size_t realsize;
  PcktInterpolator interpolator;
};

PcktSample *
pckt_sample_new ()
{
  PcktSample *sample = malloc (sizeof (PcktSample));
  if (sample)
    {
      sample->rate = PCKT_SAMPLE_RATE_DEFAULT;
      sample->frames = NULL;
      sample->nframes = 0;
      sample->realsize = 0;
      sample->interpolator = NULL;
    }
  return sample;
}

void
pckt_sample_free (PcktSample *sample)
{
  if (!sample)
    return;
  if (sample->frames)
    free (sample->frames);
  free (sample);
}

uint32_t
pckt_sample_rate (PcktSample *sample, uint32_t rate)
{
  if (!sample)
    return 0;
  if (rate > 0)
    sample->rate = rate;
  return sample->rate;
}

static inline size_t
interpolate_constant (const float *src, size_t src_size,
                      float *dest, size_t dest_size,
                      float ratio)
{
  uint32_t i, frame;
  for (i = 0; i < dest_size; ++i)
    {
      frame = (i * ratio);
      if (frame >= src_size)
        break;
      dest[i] = src[frame];
    }
  return i;
}

static inline size_t
interpolate_linear (const float *src, size_t src_size,
                    float *dest, size_t dest_size,
                    float ratio)
{
  uint32_t i, f1, f2;
  float pos, w1, w2;
  for (i = 0; i < dest_size; ++i)
    {
      pos = (float) i * ratio;
      f1 = floorf (pos); /* Lower source frame position.  */
      f2 = ceilf (pos); /* Upper source frame position.  */
      if (f1 >= src_size)
        break;
      else if (f1 == f2 || f2 >= src_size)
        {
          dest[i] = src[f1];
          continue;
        }
      w1 = (float) f2 - pos; /* Lower source frame weight.  */
      w2 = pos - (float) f1; /* Upper source frame weight.  */
      dest[i] = (src[f1] * w1) + (src[f2] * w2);
    }
  return i;
}

bool
pckt_sample_set_interpolation (PcktSample *sample, PcktInterpolation intrpl)
{
  if (!sample)
    return false;

  switch (intrpl)
    {
    case PCKT_INTRPL_NONE:
      sample->interpolator = NULL;
      break;
    case PCKT_INTRPL_CONSTANT:
      sample->interpolator = interpolate_constant;
      break;
    case PCKT_INTRPL_LINEAR:
      sample->interpolator = interpolate_linear;
      break;
    default:
      return false;
    }

  return true;
}

size_t
pckt_sample_read (const PcktSample *sample, float *frames, size_t nframes,
                  size_t offset, uint32_t rate)
{
  if (!sample || !frames || !nframes)
    return 0;

  if ((rate == 0) || (rate == sample->rate) || !sample->interpolator)
    {
      if (offset >= sample->nframes)
        return 0;
      else if (offset + nframes > sample->nframes)
        nframes = sample->nframes - offset;

      memcpy (frames, sample->frames + offset, sizeof (float) * nframes);
    }
  else
    {
      float ratio = (float) sample->rate / rate;
      offset *= ratio;
      if (ratio <= 0 || offset >= sample->nframes)
        return 0;

      nframes = sample->interpolator (sample->frames + offset,
                                      sample->nframes - offset,
                                      frames, nframes, ratio);
    }

  return nframes;
}

size_t
pckt_sample_write (PcktSample *sample, const float *frames, size_t nframes)
{
  if (!sample)
    return 0;
  else if (!frames || !nframes)
    return sample->nframes;

  size_t minsize = sizeof (float) * (sample->nframes + nframes);
  if (sample->realsize < minsize)
    {
      if (sample->realsize == 0)
        sample->realsize = 1;
      while (sample->realsize < minsize)
        sample->realsize <<= 1;
      sample->frames = realloc (sample->frames, sample->realsize);
    }

  if (sample->frames)
    {
      memcpy (sample->frames + sample->nframes,
              frames,
              sizeof (float) * nframes);
      sample->nframes += nframes;
    }
  else
    sample->nframes = 0;

  return sample->nframes;
}

bool
pckt_sample_resize (PcktSample *sample, size_t nframes)
{
  if (!sample)
    return false;
  else if (nframes == sample->nframes)
    return true;

  sample->realsize = sizeof (float) * nframes;
  sample->frames = realloc (sample->frames, sample->realsize);
  if (!sample->frames && nframes > 0)
    {
      sample->realsize = 0;
      sample->nframes = 0;
      return false;
    }
  else if (nframes < sample->nframes)
    sample->nframes = nframes;

  return true;
}

bool
pckt_resample (PcktSample *sample, uint32_t rate)
{
  if (!sample || !rate)
    return false;
  else if (sample->rate == rate || sample->nframes == 0)
    return true;

  float ratio = (float) rate / sample->rate;
  size_t nframes = ratio * sample->nframes;
  float *frames = malloc (nframes * sizeof (float));
  if (!frames)
    return false;

  memset (frames, 0, nframes * sizeof (float));
  pckt_sample_read (sample, frames, nframes, 0, rate);

  free (sample->frames);
  sample->frames = frames;
  sample->nframes = nframes;
  sample->realsize = nframes * sizeof (float);
  sample->rate = rate;

  return true;
}
