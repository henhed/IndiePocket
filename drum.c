#include <string.h>
#include <math.h>
#include "drum.h"
#include "sample.h"

#define MAX_NUM_SAMPLES 16

struct pckt_drum_s
{
  pckt_sample_t *samples[PCKT_NCHANNELS][MAX_NUM_SAMPLES];
  size_t nsamples[PCKT_NCHANNELS];
  float bleed[PCKT_NCHANNELS];
};

pckt_drum_t *
pckt_drum_malloc ()
{
  pckt_drum_t *drum = malloc (sizeof (pckt_drum_t));
  if (drum)
    memset (drum, 0, sizeof (pckt_drum_t));
  return drum;
}

void
pckt_drum_free (pckt_drum_t *drum)
{
  if (!drum)
    return;
  pckt_sample_t *sample;
  pckt_channel_t ch;
  unsigned int i;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    {
      for (i = 0; i < MAX_NUM_SAMPLES; ++i)
        {
          sample = drum->samples[ch][i];
          if (sample)
            pckt_sample_free (sample);
        }
    }
  free (drum);
}

int
pckt_drum_setbleed (pckt_drum_t *drum, pckt_channel_t ch, float bleed)
{
  if (!drum || ch < PCKT_CH0 || ch >= PCKT_NCHANNELS || bleed < 0)
    return 0;
  drum->bleed[ch] = bleed;
  return 1;
}

int
pckt_drum_addsample (pckt_drum_t *drum, pckt_sample_t *sample,
                     pckt_channel_t ch)
{
  if (!drum || !sample || ch < PCKT_CH0 || ch >= PCKT_NCHANNELS
      || drum->nsamples[ch] >= MAX_NUM_SAMPLES)
    return 0;
  drum->samples[ch][drum->nsamples[ch]++] = sample;
  return 1;
}

int
pckt_drum_hit (const pckt_drum_t *drum, pckt_sound_t *sound, float force)
{
  if (!drum || !sound)
    return 0;

  memset (sound, 0, sizeof (pckt_sound_t));
  if (force <= 0)
    return 1;

  pckt_channel_t ch;
  unsigned int sample;
  size_t nsamples;
  float bleed;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    {
      nsamples = drum->nsamples[ch];
      if (nsamples == 0) // no samples for channel
        continue;
      bleed = drum->bleed[ch] * force;
      if (bleed <= 0) // only muted channels
        continue;
      sound->bleed[ch] = bleed;
      sample = roundf (force * (nsamples - 1));
      if (sample >= nsamples)
        sample = nsamples - 1;
      sound->samples[ch] = drum->samples[ch][sample];
    }

  return 1;
}
