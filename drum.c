#include <string.h>
#include <math.h>
#include "drum.h"
#include "sample.h"

#define MAX_NUM_SAMPLES 16

struct PcktDrumImpl
{
  PcktSample *samples[PCKT_NCHANNELS][MAX_NUM_SAMPLES];
  size_t nsamples[PCKT_NCHANNELS];
  float bleed[PCKT_NCHANNELS];
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
  PcktSample *sample;
  PcktChannel ch;
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
pckt_drum_set_bleed (PcktDrum *drum, PcktChannel ch, float bleed)
{
  if (!drum || ch < PCKT_CH0 || ch >= PCKT_NCHANNELS || bleed < 0)
    return 0;
  drum->bleed[ch] = bleed;
  return 1;
}

int
pckt_drum_add_sample (PcktDrum *drum, PcktSample *sample, PcktChannel ch)
{
  if (!drum || !sample || ch < PCKT_CH0 || ch >= PCKT_NCHANNELS
      || drum->nsamples[ch] >= MAX_NUM_SAMPLES)
    return 0;
  drum->samples[ch][drum->nsamples[ch]++] = sample;
  return 1;
}

int
pckt_drum_hit (const PcktDrum *drum, PcktSound *sound, float force)
{
  if (!drum || !sound)
    return 0;

  memset (sound, 0, sizeof (PcktSound));
  if (force <= 0)
    return 1;

  PcktChannel ch;
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

int
pckt_process_sound (PcktSound *sound, float **out, size_t nframes,
                    unsigned int rate)
{
  if (!sound || !out || !nframes)
    return 0;

  PcktChannel ch;
  float buffer[nframes];
  size_t nread, nreadmax = 0;
  unsigned int frame;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    {
      if (!sound->samples[ch] || !out[ch] || sound->bleed[ch] <= 0)
        continue;
      nread = pckt_sample_read (sound->samples[ch], buffer, nframes,
                                sound->progress[ch], rate);
      sound->progress[ch] += nread;
      for (frame = 0; frame < nread; ++frame)
        out[ch][frame] += buffer[frame] * sound->bleed[ch];
      if (nread < nframes) // mute channel if we're out of frames
        sound->bleed[ch] = 0;
      if (nread > nreadmax)
        nreadmax = nread;
    }

  return (int) nreadmax;
}
