// iggyexpruntime.h stub - all functions are inline no-ops

#ifndef __RAD_INCLUDE_IGGYEXPRUNTIME_H__
#define __RAD_INCLUDE_IGGYEXPRUNTIME_H__

#include "rrCore.h"

#define IDOC

RADDEFSTART

#ifndef __RAD_HIGGYEXP_
#define __RAD_HIGGYEXP_
typedef void * HIGGYEXP;
#endif

#define IGGYEXP_MIN_STORAGE  1024   IDOC

IDOC inline HIGGYEXP RADLINK IggyExpCreate(char *ip_address, S32 port, void *storage, S32 storage_size_in_bytes) { (void)ip_address; (void)port; (void)storage; (void)storage_size_in_bytes; return 0; }
IDOC inline void  RADLINK IggyExpDestroy(HIGGYEXP p) { (void)p; }
IDOC inline rrbool RADLINK IggyExpCheckValidity(HIGGYEXP p) { (void)p; return 0; }

RADDEFEND

#endif//__RAD_INCLUDE_IGGYEXPRUNTIME_H__
