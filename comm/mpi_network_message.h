/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_message.h
	@addtogroup network

 @{
 **/

#ifndef _MPI_NETWORK_MESSAGE_H
#define _MPI_NETWORK_MESSAGE_H

#include "comm.h"
#include "mpi_network.h"
#include "network_interface.h"

class mpi_network;

class mpi_network_message {
public:
//	OBJECT *from;
//	OBJECT *to;
	char256 to_name;
	char256 from_name;
//	char1024 message;
//	int16 buffer_size;
	uint64 ns3RecvTime;
	// could be private with Network & NI friended, package-level privacy
	//mpi_network_message *next, *prev;
//	network_interface *pTo, *pFrom;
	OBJECT *from;
	OBJECT *to;
	double size;
	char message[1024];
	int16 buffer_size;
	int64 start_time;
	int64 end_time;
	
	int send_message(network_interface *mnif, TIMESTAMP ts);
	int send_message(OBJECT *f, OBJECT *t, char *m, int len);
	int send_message(OBJECT *f, char *t, char *m, int len);
	int send_message();

	//int deliver();
public:
	static CLASS *oclass;
	mpi_network_message();
	mpi_network_message(MODULE *mod);
	mpi_network_message(char *serialized,size_t size,OBJECT *thisobj);
	int create();
	int isa(char *classname);
	int notify(int, PROPERTY *);

	char *serialize(uint32 *);
	int deserialize(char *, size_t,OBJECT*);

};

#endif // _MPI_NETWORK_MESSAGE_H