/** $Id: test.cpp,v 1.32 2012/25/04 12:02:47 d3p988 Exp $
	Copyright (C) 2012 Battelle Memorial Institute
	@file test.cpp
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

// default static variables here

EXPORT int module_test(int argc, char *argv[]){
	int rv = 1;
	// mpi_network_message
	rv &= test_mnmsg_send();
	rv &= test_mnmsg_serialize();
	rv &= test_mnmsg_deserialize();

	// market_network_interface
	rv &= test_mnif_check_write_msg();
//	rv &= test_mnif_check_buffer();
	rv &= test_mnif_send_buffer();
	rv &= test_mnif_send_update();
	rv &= test_mnif_handle_inbox();
	rv &= test_mnif_process_bid();
	rv &= test_mnif_send_rsp();
	
	// controller_network_interface

	// mpi_network
	rv &= test_mnet_attach();
	rv &= test_mnet_deliver();
	return rv;
}

// EOF
