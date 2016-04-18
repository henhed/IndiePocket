#ifndef PCKT_H
#define PCKT_H 1

#ifndef __BEGIN_DECLS
# ifdef __cplusplus
#  define __BEGIN_DECLS extern "C" {
#  define __END_DECLS }
# else
#  define __BEGIN_DECLS
#  define __END_DECLS
# endif
#endif

#ifndef __cplusplus
# include <stdbool.h>
#endif

#include <stdint.h>

__BEGIN_DECLS

typedef enum {
  PCKTE_SUCCESS = 0,
  PCKTE_UNKNOWN
} PcktStatus;

__END_DECLS

#endif /* ! PCKT_H */
