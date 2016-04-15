#ifndef PCKT_SAMPLE_H
#define PCKT_SAMPLE_H 1

#include <stdlib.h>

#define PCKT_SAMPLE_RATE_DEFAULT 44100

typedef struct PcktSampleImpl PcktSample;

extern PcktSample *pckt_sample_new ();
extern void pckt_sample_free (PcktSample *);
extern unsigned int pckt_sample_rate (PcktSample *, unsigned int);
extern size_t pckt_sample_read (const PcktSample *, float *, size_t, size_t,
                                unsigned int);
extern size_t pckt_sample_write (PcktSample *, const float *, size_t);
extern int pckt_resample (PcktSample *, unsigned int);
extern PcktSample *pckt_sample_factory (const char *);

#endif /* ! PCKT_SAMPLE_H */
