#ifndef PCKT_SAMPLE_H
#define PCKT_SAMPLE_H 1

#include <stdlib.h>
#include "pckt.h"

#define PCKT_SAMPLE_RATE_DEFAULT 44100

__BEGIN_DECLS

typedef struct PcktSampleImpl PcktSample;

extern PcktSample *pckt_sample_new ();
extern void pckt_sample_free (PcktSample *);
extern uint32_t pckt_sample_rate (PcktSample *, uint32_t);
extern size_t pckt_sample_read (const PcktSample *, float *, size_t, size_t,
                                uint32_t);
extern size_t pckt_sample_write (PcktSample *, const float *, size_t);
extern bool pckt_resample (PcktSample *, uint32_t);
extern PcktSample *pckt_sample_factory (const char *);

__END_DECLS

#endif /* ! PCKT_SAMPLE_H */
