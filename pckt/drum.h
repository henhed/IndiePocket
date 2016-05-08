#ifndef PCKT_DRUM_H
#define PCKT_DRUM_H 1

#include "pckt.h"
#include "sound.h"
#include "sample.h"

__BEGIN_DECLS

typedef struct PcktDrumImpl PcktDrum;
typedef struct PcktDrumMetaImpl PcktDrumMeta;

extern PcktDrum *pckt_drum_new ();
extern void pckt_drum_free (PcktDrum *);
extern bool pckt_drum_set_bleed (PcktDrum *, PcktChannel, float);
extern bool pckt_drum_set_meta (PcktDrum *, const PcktDrumMeta *);
extern bool pckt_drum_add_sample (PcktDrum *, PcktSample *, PcktChannel);
extern bool pckt_drum_hit (const PcktDrum *, PcktSound *, float);
extern PcktDrumMeta *pckt_drum_meta_new (const char *);
extern void pckt_drum_meta_free (PcktDrumMeta *);
extern const char *pckt_drum_meta_get_name (const PcktDrumMeta *);
extern float pckt_drum_meta_get_tuning (const PcktDrumMeta *);
extern bool pckt_drum_meta_set_tuning (PcktDrumMeta *, float);

__END_DECLS

#endif /* ! PCKT_DRUM_H */
