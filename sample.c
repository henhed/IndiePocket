#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include "sample.h"

struct pckt_sample_s
{
  unsigned int rate;
  float *frames;
  size_t nframes;
  size_t realsize;
};

pckt_sample_t *
pckt_sample_malloc ()
{
  pckt_sample_t *sample = malloc (sizeof (pckt_sample_t));
  sample->rate = PCKT_SAMPLE_RATE_DEFAULT;
  sample->frames = NULL;
  sample->nframes = 0;
  sample->realsize = 0;
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
                  size_t offset)
{
  if (!sample || !frames || !nframes || offset >= sample->nframes)
    return 0;

  if (offset + nframes > sample->nframes)
    nframes = sample->nframes - offset;

  memcpy (frames, sample->frames + offset, sizeof (float) * nframes);
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
  if (!sample)
    return 0;
  else if (sample->rate == rate || sample->nframes == 0)
    return 1;

  return 0;
}

static int
_pckt_sample_load (pckt_sample_t *sample, SNDFILE *file, const SF_INFO *info)
{
  if (!sample || !file)
    return 0;

  if (!pckt_resample (sample, (unsigned int) info->samplerate))
    return 0;

  int f, c;
  size_t nframes = 4096;
  float mono[nframes];
  float interleaved[nframes * info->channels];
  sf_count_t nread;
  while ((nread = sf_readf_float (file, interleaved, nframes)) > 0)
    {
      for (f = 0; f < nread; ++f)
        {
          mono[f] = interleaved[f * info->channels];
          for (c = 1; c < info->channels; ++c)
            mono[f] += interleaved[f + c];
          mono[f] /= info->channels;
        }
      pckt_sample_write (sample, mono, nread);
    }

  return 1;
}

pckt_sample_t *
pckt_sample_open (const char *filename)
{
  SF_INFO info;
  info.format = 0;
  SNDFILE *file = sf_open (filename, SFM_READ, &info);
  if (!file)
    return NULL;

  pckt_sample_t *sample = pckt_sample_malloc ();
  pckt_sample_rate (sample, (unsigned int) info.samplerate);
  if (!_pckt_sample_load (sample, file, &info))
    {
      pckt_sample_free (sample);
      sample = NULL;
    }

  sf_close (file);
  return sample;
}
