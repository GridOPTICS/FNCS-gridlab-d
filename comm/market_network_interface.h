/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file market_network_interface.h
	@addtogroup network

 @{
 **/

#ifndef _MARKET_NETWORK_INTERFACE_H
#define _MARKET_NETWORK_INTERFACE_H

#include "comm.h"
#include "network.h"
#include "network_message.h"
#include "network_interface.h"
#include "mpi_network_message.h"
#include <integrator.h>
#include <objectcomminterface.h>
#include <message.h>

class network;
class network_message;
class mpi_network;
class mpi_network_message;

class market_network_interface : network_interface
{
public:
	//char data_buffer[256];
	char256 avg_prop_name;
	char256 stdev_prop_name;
	char256 market_id_prop_name;
	char256 adj_price_prop_name;
	//int32 curr_buffer_size;
//	PROPERTY *target;
//	network_interface *next;
	//mpi_network_message *inbox;
	//mpi_network_message *outbox;
	mpi_network *pNetwork;
//	TIMESTAMP next_msg_time;
//protected:
	bool check_write_msg();
	void send_buffer_msg();
	int64 bid_res;	// these should be per-message and not
	int64 bid_rv;	//  done at the object level
//private:
	int process_bid(mpi_network_message *);
	int send_bid_response(char* dst, int64 bid_res, int64 bid_rv); // part of p_b?
	int send_market_update();

	int64 last_market_id;
	bool write_msg;
	bool write_init;
	FUNCTIONADDR bid_func;
	FUNCTIONADDR gm4t_func;
	PROPERTY *market_id_prop;
	PROPERTY *avg_prop;
	PROPERTY *stdev_prop;
	PROPERTY *period_prop;
	PROPERTY *adj_price_prop;
	PROPERTY *price_prop;
	PROPERTY *unit_prop;
	PROPERTY *pricecap_prop;
	double *pAdjPriceProp;


	sim_comm::ObjectCommInterface *myInterface;
public:
	static CLASS *oclass;
	static CLASS *pclass;
	static market_network_interface *defaults;
	
	market_network_interface(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	//int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	double *get_double(OBJECT *obj, char *name);

	int on_message();
	int check_buffer();
	//TIMESTAMP handle_inbox(TIMESTAMP);
	//mpi_network_message *handle_inbox(TIMESTAMP, mpi_network_message *);
	//unsigned int count_outbound();
};

#endif // _NETWORK_INTERFACE_H
