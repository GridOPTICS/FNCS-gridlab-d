/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network.cpp
	@addtogroup mpi_network MPI network object controller
	@ingroup comm

 @{
 **/

#ifndef USE_MPI
#define USE_MPI
#endif

#ifdef USE_MPI

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
#include "mpi_network.h"


//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* mpi_network::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
mpi_network::mpi_network(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"mpi_network",sizeof(mpi_network),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the mpi_network class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_int64, "interval", PADDR(interval),
			PT_int32, "mpi_target", PADDR(mpi_target),
			PT_int64, "reply_time", PADDR(reply_time), 
			PT_int32, "broadCastCount", PADDR(broadCastCount),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

// create is called every time a new object is loaded
int mpi_network::create() 
{
	memset(this, 0, sizeof(mpi_network));
	mpi_target = 1;
	interval = 60;
	reply_time = last_time = next_time = their_time = 0;
	numberOfReceived = numberOfSend = 0;
	return 1;
}

int mpi_network::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);

	// input validation checks
	if(mpi_target < 0){
		gl_error("mpi_network init(): target is negative");
		return 0;
	}
	if(mpi_target == 0){
		gl_error("mpi_network init(): target is 0, which is assumed to be GridLAB-D");
		return 0;
	}
	
	if(interval < 1){
		gl_error("mpi_network init(): interval is not greater than zero");
		return 0;
	}

	// success
	this->comm = CommGldNs3<mpi_network_message>::CommGldNs3Factory();
	return 1;
}

int mpi_network::send_comm_message(mpi_network_message* given)
{
    return comm->sendMsg(given);
}

int mpi_network::isa(char *classname){
	return (strcmp(classname,"mpi_network")==0 || network::isa(classname));
}

TIMESTAMP mpi_network::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	MPI_Status status;
	int rv = 0;

	// first iteration for this object?
	if(t0 == 0){
		next_time = t1;
		next_time += interval; //for first time step always sync in second 1 later
		//printf("Allowed to continue until %d\n because it is initial time\n",next_time);
		return next_time;
	}

	if(next_time > t1){//reiteration no update in the time steps.
		return next_time;
	}
	
	if(scrape_messages()>0){
	  next_time += interval; //for first time step always sync in second 1 later
	  printf("Allowed to continue until %d\n because I'm going to send\n",next_time);
	  return next_time;
	}
	
	if( numberOfReceived < numberOfSend){ //we didn't receive all the send messages
	    next_time += interval; //for first time step always sync in second 1 later
	    printf("Allowed to continue until %d\n because I'm expecting more messages s=%d r=%d\n",next_time,numberOfSend,numberOfReceived);
	    return next_time;
	}
	//printf("Allowed to continue until %d\n because I have all messages\n",next_time);
	//we got all the messages we have sent, the sim can progress normally.
	//return TS_NEVER;
	next_time += interval; //for first time step always sync in second 1 later
	printf("Allowed to continue until %d\n for no reason\n",next_time);
	return next_time;
	  
}

int mpi_network::precommit(TIMESTAMP t1){
	next_time = last_time = t1;

	#ifdef REALLYUSEMPI	
		MPI_Ssend((void *)&last_time, 1, MPI_LONG_LONG, mpi_target, 1, MPI_COMM_WORLD);
		MPI_Ssend((void *)&last_time, 1, MPI_LONG_LONG, mpi_target, 1, MPI_COMM_WORLD);
		printf("Sent next sycn time %d\n",t1);
	#endif
	
	mpi_network_message *msg;
	std::vector<mpi_network_message *> *recv_vect;
	recv_vect = this->get_comm_messages(t1); // t2 is immutable, we WILL advance to t2.
	if(recv_vect == 0){
		gl_error("mpi_network::commit(): get_comm_messages() failed");
		return TS_INVALID;
	}
	if(recv_vect->empty()){
		// short circuit, nothing to do
		//std::cout << "EMPTY!!!" << std::endl;
		delete recv_vect;
		return TS_NEVER;
	}
	//std::cout << "NOT EMPTY!!" << std::endl; 
	// route messages
	std::vector<mpi_network_message *>::iterator recv_itr;
	// for each message,
	for(recv_itr = recv_vect->begin(); recv_itr != recv_vect->end(); ++recv_itr){	
		// have we deserialized? -yes.
		// is the destination valid? -checked in deserialize()
		// enqueue in appropriate object
		msg = *recv_itr;
		//gl_output("mpi_net::cmt() ~ delivering '%s'", msg->message);
		if(0 == msg->deliver()){
			// too late to do anything about it?
			// already reported an error.
			;
		}
		else{
		  this->numberOfReceived++;
		}
	}
	delete recv_vect;
	return 1;
}

// returns 0-n on success, -1 on error
int mpi_network::scrape_messages(){
	int rv = 0;
	int out_msg_ct = 0;
	network_interface *nif = first_if;
	OBJECT *nifobj;

	for(nif = first_if; nif != 0; nif = nif->next){
		// crude to use isa and (cast), but we can't use virtual tables,
		//	we can't use member function pointers & inheritence, and we
		//	can't/won't use untyped function pointers.
		nifobj = OBJECTHDR(nif);
		if(gl_object_isa(nifobj, market_network_interface::oclass->name)){
			market_network_interface *mnif = (market_network_interface *)nif;
			rv = mnif->check_buffer();
			if(mnif->has_outbound())
				out_msg_ct += mnif->count_outbound();
		} else if(gl_object_isa(nifobj, controller_network_interface::oclass->name)){
			controller_network_interface *cnif = (controller_network_interface *)nif;
			rv = cnif->check_buffer();
			if(cnif->has_outbound())
				out_msg_ct += cnif->count_outbound();
		} else if(gl_object_isa(nifobj, network_interface::oclass->name)){
			rv = nif->check_buffer();
			if(nif->has_outbound())
				out_msg_ct += nif->count_outbound();
		} else {
			gl_error("mpi_network::scrape_messages(): unsupported network_interface type was attached");
			return -1;
		}
		if(rv != 1){
			gl_error("mpi_network::scrape_messages(): error when checking for outbound messages");
			return -1;
		}
	}
	
	return out_msg_ct;
}


//EXPORT int commit_network(OBJECT *obj){
TIMESTAMP mpi_network::commit(TIMESTAMP t1, TIMESTAMP t2){
	OBJECT *obj = OBJECTHDR(this);
	OBJECT *nifobj;
	TIMESTAMP t0 = obj->clock;
	int rv = 0;
	int out_msg_ct = 0;
	mpi_network_message *msg = 0;
	char namestr[64];
	
	//int timedif=t2-t1;
	#ifdef REALLYUSEMPI	
		MPI_Ssend((void *)&t1, 1, MPI_LONG_LONG, mpi_target, 1, MPI_COMM_WORLD);
		MPI_Ssend((void *)&t2, 1, MPI_LONG_LONG, mpi_target, 1, MPI_COMM_WORLD);
		printf("Sent next sycn time %d %d\n",t1,t2);
	#endif
		
	out_msg_ct = scrape_messages();
	if(out_msg_ct == -1){
		return TS_INVALID;
	} else if(out_msg_ct > 0){
		// send messages
		//std::cout << gl_globalclock << ": Out_msg_ct is " << out_msg_ct << std::endl;
		mpi_network_message *mnmsg = 0;
		network_interface *nif;
		market_network_interface *mnif;
		controller_network_interface *cnif;
		for(nif = first_if; nif != 0; nif = nif->next){
			nifobj = OBJECTHDR(nif);
			LOCK_OBJECT(nifobj);
			if(gl_object_isa(nifobj, market_network_interface::oclass->name)){
				market_network_interface *mnif = (market_network_interface *)nif;
				mpi_network_message *msg = mnif->outbox;
				
				while(msg!=0){
				  
				  mpi_network_message *tmp=msg;
				  msg=msg->next;
				  mnif->outbox = msg;
				  tmp->next=0;
				  gl_verbose("[cmt]: market:%i ~ %s", nifobj->id, tmp->message.get_string());
				  if(0 != send_comm_message(tmp)){
						gl_name(nifobj, namestr, 63);
						gl_error("problem sending a message from %s", namestr);
					} else {
						// no problem, is using negative logic
						if(tmp->to_name[0]=='*'){
						  this->numberOfSend+=broadCastCount;
						}
						else
						  this->numberOfSend++;
						printf("Sent %d\n",numberOfSend);
					}
				  delete tmp;
				}
				/*for(msg = mnif->outbox; msg != 0; msg = msg->next){
					gl_verbose("[cmt]: market:%i ~ %s", nifobj->id, msg->message);
					mnif->outbox = msg->next;
					msg->next = 0;
					if(0 != send_comm_message(msg)){
						gl_name(nifobj, namestr, 63);
						gl_error("problem sending a message from %s", namestr);
					} else {
						// no problem, is using negative logic
					}
				}*/
			} else if(gl_object_isa(nifobj, controller_network_interface::oclass->name)){
				controller_network_interface *cnif = (controller_network_interface *)nif;
				mpi_network_message *msg = cnif->outbox;
				while(msg!=0){
				  mpi_network_message *tmp=msg;
				  msg=msg->next;
				  cnif->outbox = msg;
				  tmp->next=NULL;
				  //std::cout << "controller:" << nifobj->id << " ~ " << tmp->message << std::endl;
				  gl_output("[cmt]: controller:%i ~ %s", nifobj->id, tmp->message.get_string());
				  if(0 != send_comm_message(tmp)){
						gl_name(nifobj, namestr, 63);
						gl_error("problem sending a message from %s", namestr);
					} else {
						// no problem, is using negative logic
						if(tmp->to_name[0]=='*'){
						  this->numberOfSend+=broadCastCount;
						}
						else
						  this->numberOfSend++;
						printf("Sent %d\n",numberOfSend);
					}
				  delete tmp;
				}
				
				/*for(msg = cnif->outbox; msg != 0; msg = msg->next){
					gl_verbose("[cmt]: controller:%i ~ %s", nifobj->id, msg->message);
					cnif->outbox = msg->next;
					msg->next = 0;
					send_comm_message(msg);
				}*/
			} else if(gl_object_isa(nifobj, network_interface::oclass->name)){
				//std::cout << "network" << std::endl;
				; // nif uses net_msg and not mpi_net_msg, so is not technically supported.
			} else {
				gl_error("mpi_network::commit(): unsupported network_interface type was found when gathering messages to send");
				UNLOCK_OBJECT(nifobj);
				return -1;
			}
			UNLOCK_OBJECT(nifobj);
		}
	} else {
		// any special short-circuits when nothing needs to be sent?
		;
	}
	
	// send each message via MPI to NS3

	// @TODO (either here or precommit)
	// receive messages
	
	
	/*#ifdef REALLYUSEMPI	
		MPI_Ssend((void *)&t2, 1, MPI_LONG_LONG, mpi_target, 1, MPI_COMM_WORLD);
		printf("Sent next sycn time %d\n",t1);
	#endif*/
	
	return TS_NEVER;
}

//return my->notify(update_mode, prop);
int mpi_network::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

int mpi_network::attach(network_interface *nif){
	if(nif == 0){
		gl_error("mpi_network::attach(): null network_interface parameter");
		return 0;
	}
	if(!nif->isa("network_interface")){
		char name[64];
		gl_name(OBJECTHDR(nif), name, 63);
		gl_error("mpi_network::attach(): cannot attach '%s' of class '%s'", name, OBJECTHDR(nif)->oclass->name);
		return 0;
	}
	if(first_if == 0){
		first_if = nif;
		last_if = nif;
		nif->next = 0;
	} else {
		last_if->next = nif;
		last_if = nif;
		nif->next = 0;
	}
	nif->net = this;
	return 1;
}

// returns a new vector with the messages popped off the recv queue.  vector
//	may be empty if no messages were received.
std::vector<mpi_network_message*> *mpi_network::get_comm_messages(TIMESTAMP currentSimTime)
{
    TIMESTAMP prevTick=currentSimTime-1;
    
    std::vector<mpi_network_message*> *toReturn;
  
    bool cont=true;
    
	toReturn = new std::vector<mpi_network_message *>();
	if(toReturn == 0){
		gl_error("get_comm_messages(): could not create new vector");
		return 0;
	}

	do{
		mpi_network_message *msg=this->comm->peekMsgWithTimeOut(0); 
		if(msg==NULL){
			//std::cout << "null geldi!!" << std::endl;
			cont = false;
			break;
		}
		//std::cout << "message ns3RecvTime" << msg->ns3RecvTime << std::endl;
		//f(msg->ns3RecvTime>prevTick && msg->ns3RecvTime <= currentSimTime){
			this->comm->getMsgWithTimeOut(0); //pop the message from queue, already got it above
			toReturn->push_back(msg);
			//std::cout << "pushed!!" << std::endl;
		//} else {
		//	// leave it if we haven't advanced the clock to the message recv time yet
		//	cont=false;
		//	break;
		//}
	} while(cont);
    
    return toReturn;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_mpi_network(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(mpi_network::oclass);
	if (*obj!=NULL)
	{
		mpi_network *my = OBJECTDATA(*obj,mpi_network);
		gl_set_parent(*obj,parent);
		try {
			my->create();
		}
		catch (char *msg)
		{
			gl_error("%s::%s.create(OBJECT **obj={name='%s', id=%d},...): %s", (*obj)->oclass->module->name, (*obj)->oclass->name, (*obj)->name, (*obj)->id, msg);
			return 0;
		}
		return 1;
	}
	return 0;
}

EXPORT int init_mpi_network(OBJECT *obj)
{
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_mpi_network(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,mpi_network)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_mpi_network(OBJECT *obj, TIMESTAMP t1)
{
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	try {
		TIMESTAMP t2 = my->sync(obj->clock, t1);
		//obj->clock = t1; // update in commit
		return t2;
	}
	catch (char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.init(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		return 0;
	}
}

EXPORT TIMESTAMP precommit_mpi_network(OBJECT *obj, TIMESTAMP t1){
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	TIMESTAMP rv = my->precommit(t1);
//	obj->clock = gl_globalclock;
	return rv;
}

EXPORT TIMESTAMP commit_mpi_network(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	TIMESTAMP rv = my->commit(t1, t2);
	obj->clock = gl_globalclock;
	return rv;
}

EXPORT int notify_mpi_network(OBJECT *obj, int update_mode, PROPERTY *prop){
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	return my->notify(update_mode, prop);
}

#endif
// USE_MPI
/**@}**/
