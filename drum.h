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
} PcktChannel;

typedef struct
{
  PcktSample *samples[PCKT_NCHANNELS];
  float bleed[PCKT_NCHANNELS];
  size_t progress[PCKT_NCHANNELS];
} PcktSound;

typedef struct PcktDrumImpl PcktDrum;

extern PcktDrum *pckt_drum_new ();
extern void pckt_drum_free (PcktDrum *);
extern int pckt_drum_set_bleed (PcktDrum *, PcktChannel, float);
extern int pckt_drum_add_sample (PcktDrum *, PcktSample *, PcktChannel);
extern int pckt_drum_hit (const PcktDrum *, PcktSound *, float);
extern int pckt_process_sound (PcktSound *, float **, size_t, unsigned int);

#endif /* ! PCKT_DRUM_H */
