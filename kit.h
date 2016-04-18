#ifndef PCKT_KIT_H
#define PCKT_KIT_H 1

#include <stdint.h>
#include "pckt.h"
#include "drum.h"

__BEGIN_DECLS

typedef struct PcktKitImpl PcktKit;

extern PcktKit *pckt_kit_new ();
extern void pckt_kit_free (PcktKit *);
extern int8_t pckt_kit_add_drum (PcktKit *, PcktDrum *, int8_t);
extern PcktDrum *pckt_kit_get_drum (const PcktKit *, int8_t);
extern PcktKit *pckt_kit_factory (const char *);

__END_DECLS

#endif /* ! PCKT_KIT_H */
