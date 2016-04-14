#ifndef PCKT_DRUM_H
#define PCKT_DRUM_H 1

#include "sample.h"

typedef enum
{
  PCKT_CH0 = 0, PCKT_CH1, PCKT_CH2, PCKT_CH3,
  PCKT_CH4, PCKT_CH5, PCKT_CH6, PCKT_CH7,
  PCKT_CH8, PCKT_CH9, PCKT_CH10, PCKT_CH11,
  PCKT_CH12, PCKT_CH13, PCKT_CH14, PCKT_CH15,
  PCKT_NCHANNELS
} pckt_channel_t;

typedef struct
{
  pckt_sample_t *samples[PCKT_NCHANNELS];
  float bleed[PCKT_NCHANNELS];
  size_t progress[PCKT_NCHANNELS];
} pckt_sound_t;

typedef struct pckt_drum_s pckt_drum_t;

extern pckt_drum_t * pckt_drum_new ();
extern void pckt_drum_free (pckt_drum_t *);
extern int pckt_drum_setbleed (pckt_drum_t *, pckt_channel_t, float);
extern int pckt_drum_addsample (pckt_drum_t *, pckt_sample_t *, pckt_channel_t);
extern int pckt_drum_hit (const pckt_drum_t *, pckt_sound_t *, float);
extern int pckt_process_sound (pckt_sound_t *, float **, size_t, unsigned int);

#endif /* ! PCKT_DRUM_H */
