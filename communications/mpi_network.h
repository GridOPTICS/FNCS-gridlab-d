/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file mpi_network.h
	@addtogroup network

 @{
 **/

#ifndef USE_MPI
#define USE_MPI
#endif

#ifdef USE_MPI
#ifndef _MPI_NETWORK_CLASS_H
#define _MPI_NETWORK_CLASS_H

#include "comm.h"
#include "network.h"
#include "mpi_network_message.h"

#undef max
#undef min
#include <vector>
#include "network_interface.h"
#include "market_network_interface.h"
#include "controller_network_interface.h"

class mpi_network_message;
class market_network_interface;
class controller_network_interface;

class mpi_network : public network
{
public:
	int64	interval;
	int64	reply_time;
	int32	mpi_target;
	int32	broadCastCount;
	network_interface *first_if, *last_if;
private:
	TIMESTAMP last_time, next_time, their_time;
	int numberOfSend, numberOfReceived;
	int scrape_messages();
	int send_comm_message(mpi_network_message *given);
public:
	static CLASS *oclass;
	mpi_network(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	int notify(int, PROPERTY *);
	int precommit(TIMESTAMP t1);
	//void sendMessage(mpi_network_message* msg);
	std::vector<mpi_network_message*> *get_comm_messages(TIMESTAMP currentSimTime);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int attach(network_interface *);
};

#endif // _NETWORK_CLASS_H
#endif // USE_MPI
/**@}**/
