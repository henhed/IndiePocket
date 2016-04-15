#include <sndfile.h>
#include "sample.h"

static int
load_sample (PcktSample *sample, SNDFILE *file, const SF_INFO *info)
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

PcktSample *
pckt_sample_factory (const char *filename)
{
  SF_INFO info;
  info.format = 0;
  SNDFILE *file = sf_open (filename, SFM_READ, &info);
  if (!file)
    return NULL;

  PcktSample *sample = pckt_sample_new ();
  pckt_sample_rate (sample, (unsigned int) info.samplerate);
  if (!load_sample (sample, file, &info))
    {
      pckt_sample_free (sample);
      sample = NULL;
    }

  sf_close (file);
  return sample;
}
