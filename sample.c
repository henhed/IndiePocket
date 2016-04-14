#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sample.h"

struct pckt_sample_s
{
  unsigned int rate;
  float *frames;
  size_t nframes;
  size_t realsize;
};

pckt_sample_t *
pckt_sample_new ()
{
  pckt_sample_t *sample = malloc (sizeof (pckt_sample_t));
  if (sample)
    {
      sample->rate = PCKT_SAMPLE_RATE_DEFAULT;
      sample->frames = NULL;
      sample->nframes = 0;
      sample->realsize = 0;
    }
  return sample;
}

void
pckt_sample_free (pckt_sample_t *sample)
{
  if (!sample)
    return;
  if (sample->frames)
    free (sample->frames);
  free (sample);
}

unsigned int
pckt_sample_rate (pckt_sample_t *sample, unsigned int rate)
{
  if (!sample)
    return 0;
  if (rate > 0)
    sample->rate = rate;
  return sample->rate;
}

size_t
pckt_sample_read (const pckt_sample_t *sample, float *frames, size_t nframes,
                  size_t offset, unsigned int rate)
{
  if (!sample || !frames || !nframes)
    return 0;

  if (rate == 0 || rate == sample->rate)
    {
      if (offset >= sample->nframes)
        return 0;
      else if (offset + nframes > sample->nframes)
        nframes = sample->nframes - offset;

      memcpy (frames, sample->frames + offset, sizeof (float) * nframes);
    }
  else // on-the-fly resampling using nearest-neighbor interpolation
    {
      float ratio = (float) sample->rate / rate;
      size_t realoffset = ratio * offset;
      unsigned int virtframe, realframe;
      for (virtframe = 0; virtframe < nframes; ++virtframe)
        {
          realframe = (virtframe * ratio) + realoffset;
          if (realframe >= sample->nframes)
            break;
          frames[virtframe] = sample->frames[realframe];
        }
      nframes = virtframe;
    }

  return nframes;
}

size_t
pckt_sample_write (pckt_sample_t *sample, const float *frames, size_t nframes)
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

int
pckt_resample (pckt_sample_t *sample, unsigned int rate)
{
  if (!sample || !rate)
    return 0;
  else if (sample->rate == rate || sample->nframes == 0)
    return 1;

  float ratio = (float) rate / sample->rate;
  size_t nframes = ratio * sample->nframes;
  float *frames = malloc (nframes * sizeof (float));
  if (!frames)
    return 0;

  unsigned int i, f1, f2;
  float pos, w1, w2;
  for (i = 0; i < nframes; ++i)
    {
      pos = ((float) i / nframes) * sample->nframes;
      f1 = floorf (pos); // lower source frame position
      f2 = ceilf (pos); // upper source frame position
      if (f1 == f2 || f2 >= sample->nframes)
        {
          frames[i] = sample->frames[f1];
          continue;
        }
      w1 = (float) f2 - pos; // lower source frame weight
      w2 = pos - (float) f1; // upper source frame weight
      frames[i]
        = sample->frames[f1] * w1
        + sample->frames[f2] * w2;
    }

  free (sample->frames);
  sample->frames = frames;
  sample->nframes = nframes;
  sample->realsize = nframes * sizeof (float);
  sample->rate = rate;

  return 1;
}
