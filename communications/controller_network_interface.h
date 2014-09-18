/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file controller_network_interface.h
	@addtogroup controller_network

 @{
 **/

#ifndef _CONTROLLER_NETWORK_INTERFACE_H
#define _CONTROLLER_NETWORK_INTERFACE_H

#include <string.h>

#include "comm.h"
#include "network.h"
#include "mpi_network.h"
#include "mpi_network_message.h"
#include "network_interface.h"
#include "integrator.h"
#include <objectcomminterface.h>

class mpi_network_message;
class mpi_network;

class controller_network_interface : public network_interface
{
public:

private:
	bool write_msg;
public:
	static CLASS *oclass;
	static CLASS *pclass;
	static controller_network_interface *defaults;

	controller_network_interface(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	//int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);

	int on_message();
	int check_buffer();
	
	void handle_inbox(TIMESTAMP, mpi_network_message *);
	
	char dst_name[257];
	char256 my_name;
	
	mpi_network *pNetwork;
	//mpi_network_message *outbox;
	//mpi_network_message *inbox;
private:
	bool check_write_msg();
	int send_bid_request();
	int process_market_init();
	int process_bid_response();
	int process_market_update();

	// values in the parent object, for reading, not writing
	PROPERTY *bid_price_prop;
	PROPERTY *bid_quant_prop;
	PROPERTY *obj_state_prop;
	PROPERTY *bid_id_prop;
	PROPERTY *bid_ready_prop;
	char256 parent_name;
	char256 market_name;
	sim_comm::ObjectCommInterface *myInterface;
	int check_proxy_mode();
	
};

#endif // _CONTROLLER_NETWORK_INTERFACE_H
