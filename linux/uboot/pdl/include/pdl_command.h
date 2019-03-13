#ifndef _PDL_COMMAND_H_
#define _PDL_COMMAND_H_
#include "config.h"
#include "packet.h"


#if defined(CONFIG_RDA_PDL1)
#include "../pdl-1/pdl1_command.h"
#elif defined(CONFIG_RDA_PDL2)
#include "../pdl-2/pdl2_command.h"
#else
#error "no valid pdl"
#endif

#endif
