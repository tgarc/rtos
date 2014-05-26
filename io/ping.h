#ifndef PING_H
#define PING_H

#include "util/fifo.h"
#include "util/macros.h"

// for init/reading/writing need to know:
// peripheral
// port base
// pin
void 
ping_sample(INT pingID);
						
						
INT
ping_port_register(ULONG peripheral,
                   ULONG portBase,
                   ULONG pin,
									 void (*sampleHandler)(ULONG));

#endif
