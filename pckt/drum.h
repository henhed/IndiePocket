#ifndef PCKT_DRUM_H
#define PCKT_DRUM_H 1

#include "pckt.h"
#include "sound.h"
#include "sample.h"

__BEGIN_DECLS

typedef struct PcktDrumImpl PcktDrum;

extern PcktDrum *pckt_drum_new ();
extern void pckt_drum_free (PcktDrum *);
extern bool pckt_drum_set_bleed (PcktDrum *, PcktChannel, float);
extern bool pckt_drum_add_sample (PcktDrum *, PcktSample *, PcktChannel);
extern bool pckt_drum_hit (const PcktDrum *, PcktSound *, float);

__END_DECLS

#endif /* ! PCKT_DRUM_H */
