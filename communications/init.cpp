/** $Id: init.cpp,v 1.32 2008/01/09 22:25:08 d3p988 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file init.cpp
	@addtogroup residential Residential loads (residential)
	@ingroup modules
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "comm.h"

#include "network.h"
#include "mpi_network.h"
#include "controller_network_interface.h"
#include "market_network_interface.h"
#include "mpi_comm_test.h"
#include "transmission_interface.h"
// default static variables here

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	new network(module);
	new network_interface(module);
#ifdef USE_MPI
	//new mpi_network(module);
	new controller_network_interface(module);
	new market_network_interface(module);
	new transmissioncom(module);
#endif

	/* always return the first class registered */
	return network::oclass;
}

CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}

// test function in comm/test.cpp
/**@}**/
