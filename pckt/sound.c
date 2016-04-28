#include <string.h>
#include "sound.h"

struct PcktSoundPoolImpl {
  PcktSound *sounds;
  size_t nsounds;
};

PcktSoundPool *
pckt_soundpool_new (size_t poolsize)
{
  PcktSoundPool *pool = malloc (sizeof (PcktSoundPool));
  if (pool)
    {
      pool->nsounds = poolsize;
      if (poolsize > 0)
        {
          pool->sounds = malloc (poolsize * sizeof (PcktSound));
          if (pool->sounds)
            pckt_soundpool_clear (pool);
          else
            {
              free (pool);
              pool = NULL;
            }
        }
    }
  return pool;
}

void
pckt_soundpool_free (PcktSoundPool *pool)
{
  if (pool)
    {
      if (pool->sounds)
        free (pool->sounds);
      free (pool);
    }
}

PcktSound *
pckt_soundpool_at (PcktSoundPool *pool, uint32_t index)
{
  if (pool && index < pool->nsounds)
    return pool->sounds + index;
  return NULL;
}

PcktSound *
pckt_soundpool_get (PcktSoundPool *pool, const void *source)
{
  if (!pool || pool->nsounds == 0)
    return NULL;

  PcktSound *sound = NULL;
  for (uint32_t i = 0; i < pool->nsounds; ++i)
    {
      bool is_dead = true;
      for (PcktChannel ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
        {
          if (pool->sounds[i].bleed[ch] > 0)
            {
              is_dead = false;
              break;
            }
        }
      if (is_dead)
        {
          sound = pool->sounds + i;
          break; /* Steal silent sounds first.  */
        }
      else if (!sound || (sound->source != source
                          && pool->sounds[i].source == source))
        {
          /* Sound is first in loop or has source priority over previous
             candidate (steal sound from given drum if exists). */
          sound = pool->sounds + i;
        }
      else if ((sound->source != source || pool->sounds[i].source == source)
               && pool->sounds[i].variance < sound->variance)
        {
          /* Sound has equal or greater source priorty and lower variance
             (steal the quietest sound, preferably from the given drum).  */
          sound = pool->sounds + i;
        }
    }

  return sound;
}

bool
pckt_soundpool_choke (PcktSoundPool *pool, const void *source)
{
  if (!pool || !source || pool->nsounds == 0)
    return false;
  for (uint32_t i = 0; i < pool->nsounds; ++i)
    {
      if (pool->sounds[i].source == source)
        pool->sounds[i].choke = true;
    }
  return true;
}

bool
pckt_soundpool_clear (PcktSoundPool *pool)
{
  if (!pool)
    return false;

  for (uint32_t i = 0; i < pool->nsounds; ++i)
    pckt_sound_clear (pool->sounds + i);

  return true;
}

bool
pckt_sound_clear (PcktSound *sound)
{
  if (!sound)
    return false;

  for (PcktChannel ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    {
      sound->samples[ch] = NULL;
      sound->bleed[ch] = 0;
      sound->progress[ch] = 0;
    }
  sound->impact = 0;
  sound->variance = 0;
  sound->choke = false;
  sound->source = NULL;

  return true;
}

int32_t
pckt_sound_process (PcktSound *sound, float **out, size_t nframes,
                    uint32_t rate)
{
  if (!sound || !out || !nframes)
    return 0;

  float frame, buffer[nframes];
  float k, sum[PCKT_NCHANNELS], sum2[PCKT_NCHANNELS]; /* For variance.  */
  size_t nread, nreadmax = 0;
  /* Calculate decay rate if sound is choked and a frame rate was given.  */
  bool choke = sound->choke && (sound->impact > 0);
  float decay = ((choke && rate > 0)
                 ? (sound->impact / (PCKT_CHOKE_TIME * rate))
                 : 0);
  PcktChannel ch;
  uint32_t i;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    {
      sum[ch] = 0;
      sum2[ch] = 0;

      if (!sound->samples[ch] || !out[ch] || sound->bleed[ch] <= 0)
        continue;

      /* Read NFRAMES frames from sample into BUFFER.  */
      nread = pckt_sample_read (sound->samples[ch], buffer, nframes,
                                sound->progress[ch], rate);
      sound->progress[ch] += nread;

      if (choke && rate == 0)
        /* Claculate per-sample decay rate if sound is choked and no frame rate
           was given.  */
        decay = sound->impact / (PCKT_CHOKE_TIME
                                 * pckt_sample_rate (sound->samples[ch], 0));

      /* Write buffered frames to output.  */
      k = buffer[0] * sound->bleed[ch];
      for (i = 0; i < nread; ++i)
        {
          if (decay >= sound->bleed[ch])
            sound->bleed[ch] = 0;
          else if (decay > 0)
            sound->bleed[ch] -= decay;
          frame = buffer[i] * sound->bleed[ch];
          out[ch][i] += frame;
          sum[ch] += frame - k;
          sum2[ch] += (frame - k) * (frame - k);
        }
      if (nread < nframes)
        {
          /* Mute channel if we're out of frames.  */
          sound->bleed[ch] = 0;
          /* Sum remainder for variance.  */
          sum[ch] -= k * (nframes - nread);
          sum2[ch] += k * k * (nframes - nread);
        }
      if (nread > nreadmax)
        nreadmax = nread;
    }

  /* Calculate average variance.  */
  sound->variance = 0;
  for (ch = PCKT_CH0; ch < PCKT_NCHANNELS; ++ch)
    sound->variance += (sum2[ch] - (sum[ch] * sum[ch]) / nframes) / nframes;
  sound->variance /= PCKT_NCHANNELS;

  return (int32_t) nreadmax;
}
