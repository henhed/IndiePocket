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

#include <sndfile.h>
#include <string.h>
#include "sample.h"

static bool
load_sample (PcktSample *sample, SNDFILE *file, const SF_INFO *info)
{
  if (!sample || !file)
    return false;

  if (!pckt_resample (sample, (uint32_t) info->samplerate))
    return false;

  int32_t f, c;
  size_t nframes = 4096;
  float mono[nframes];
  float interleaved[nframes * info->channels];
  sf_count_t nread;
  while ((nread = sf_readf_float (file, interleaved, nframes)) > 0)
    {
      memset (mono, 0, nread * sizeof (float));
      for (f = 0; f < nread; ++f)
        {
          for (c = 0; c < info->channels; ++c)
            mono[f] += interleaved[(f * info->channels) + c];
          mono[f] /= info->channels;
        }
      pckt_sample_write (sample, mono, nread);
    }

  return true;
}

PcktSample *
pckt_sample_factory (const char *filename)
{
  SF_INFO info;
  info.format = 0;
  SNDFILE *file = sf_open (filename, SFM_READ, &info);
  if (!file)
    return NULL;

  PcktSample *sample = pckt_sample_new ();
  pckt_sample_rate (sample, (uint32_t) info.samplerate);
  pckt_sample_resize (sample, (size_t) info.frames);
  pckt_sample_set_interpolation (sample, PCKT_INTRPL_LINEAR);
  if (!load_sample (sample, file, &info))
    {
      pckt_sample_free (sample);
      sample = NULL;
    }

  sf_close (file);
  return sample;
}
