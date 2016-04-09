#ifndef PCKT_SAMPLE_H
#define PCKT_SAMPLE_H 1

#define PCKT_SAMPLE_RATE_DEFAULT 44100

typedef struct pckt_sample_s pckt_sample_t;

extern pckt_sample_t * pckt_sample_malloc ();
extern pckt_sample_t * pckt_sample_open (const char *);
extern void pckt_sample_free (pckt_sample_t *);
extern unsigned int pckt_sample_rate (pckt_sample_t *, unsigned int);
extern size_t pckt_sample_read (const pckt_sample_t *, float *, size_t, size_t);
extern size_t pckt_sample_write (pckt_sample_t *, const float *, size_t);
extern int pckt_resample (pckt_sample_t *, unsigned int);

#endif /* ! PCKT_SAMPLE_H */
