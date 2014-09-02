/** $Id $
	Copyright (C) 2012 Battelle Memorial Institute
	@file controller_network_interface.cpp
	@addtogroup controller_network_interface Performance controller_network_interface object
	@ingroup comm
	@author Matt Hauer

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "controller_network_interface.h"


//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* controller_network_interface::oclass = NULL;
CLASS* controller_network_interface::pclass = NULL;
controller_network_interface *controller_network_interface::defaults = NULL;

// the constructor registers the class and properties and sets the defaults
controller_network_interface::controller_network_interface(MODULE *mod) : network_interface(mod)
{
	// first time init
	memset(this, 0, sizeof(controller_network_interface));
	if (oclass==NULL)
	{
		pclass = network_interface::oclass;
		// register the class definition
		oclass = gl_register_class(mod,"controller_network_interface",sizeof(controller_network_interface),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the controller_network_interface class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "network_interface",
			PT_char256, "destination_name", PADDR(dst_name),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the controller_network_interface properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
		defaults = this;
	}
}

// create is called every time a new object is loaded
int controller_network_interface::create() 
{
	memcpy(this, defaults, sizeof(controller_network_interface));
	return 1;
}

int controller_network_interface::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	OBJECT **m;

	// input validation checks
	// * parent is a controller
	if(0 == parent){
		gl_error("init(): no parent object");
		return 0;
	}
	if(!gl_object_isa(parent, "controller", "market")){
		gl_error("init(): parent is not a market:controller");
		return 0;
	}
	/*if(!gl_object_isa(to, "mpi_network", "comm")){
		GL_THROW("network interface is connected to a non-network object");
	} else {
		pNetwork = OBJECTDATA(to, mpi_network);
	}*/
	
	// verify 'destination' object OR 'destination_name' is set
	
	if(destination != 0){
		if(0 == gl_name(destination, dst_name, 255)){
			// shouldn't happen, but better to check than not to
			gl_error("unable to set destination_name to destination object!");
			return 0;
		}
	}
	if(0 == dst_name[0]){
		gl_error("controller_network_interface does not have a destination object or name");
		return 0;
	}
		


	// snag properties for value exchange
	bid_price_prop = gl_get_property(parent, "bid_price");
	bid_quant_prop = gl_get_property(parent, "bid_quantity");
	obj_state_prop = gl_get_property(parent, "bid_state");
//	bid_id_prop = gl_get_property(parent, "bid_id");
	bid_id_prop = gl_get_property(parent, "proxy_bid_id");
	bid_ready_prop = gl_get_property(parent, "proxy_bid_ready");
	gl_name_object(hdr, parent_name, 255);
	m = (*callback->objvarname.object_var)(parent, "market");
	gl_name_object(*m, market_name, 255);

	//register with fncs
	char name[64];
	gl_name_object(hdr,name,63);
	myInterface=sim_comm::Integrator::getCommInterface(name);

	// success
	return 1;
}

int controller_network_interface::isa(char *classname){
	return strcmp(classname,"controller_network_interface")==0;
}

TIMESTAMP controller_network_interface::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	return TS_NEVER; 
}

TIMESTAMP controller_network_interface::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP rv = TS_NEVER;
	while(this->myInterface->hasMoreMessages()){
	  sim_comm::Message *msg=this->myInterface->getNextInboxMessage();
	  if(msg->getFrom().compare(dst_name)!=0){
		  gl_output("Message from an unknown market ",msg->getFrom().c_str());
		  delete msg;
		  continue;
	  }
	  mpi_network_message *nm=new mpi_network_message((char*)msg->getData(),msg->getSize(),obj);
	  handle_inbox(t1,nm);
	  delete nm;
	  delete msg;
	}
	return rv; 
}

TIMESTAMP controller_network_interface::postsync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	check_buffer();
	return TS_NEVER; 
}


/**
 * ni::commit() is where data is pulled from its parent, copied into the interface,
 *	and written to a message to be sent to another interface.
 */
TIMESTAMP controller_network_interface::commit(TIMESTAMP t1, TIMESTAMP t2){
	
	return TS_NEVER;
}

/*
//return my->notify(update_mode, prop);
int controller_network_interface::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	if(update_mode == NM_POSTUPDATE)
		if(strcmp(prop->name, "buffer") == 0){
			// new data has been received
			// NOTE: controller_network should update 'buffer_size' before updating 'buffer'
			// * lock parent's destination property
			// * memcpy(pTarget, data_buffer, buffer_size)
			// * memset(data_buffer, 0, buffer_size)
			// * buffer_size = 0;
			;
		}
	}
	return 1;
}*/

// there isn't a good way to set_value with straight binary data, so going about it using
//	direct sets, and 'notify'ing with a follow-up function call.
int controller_network_interface::on_message(){
	return 0;
}

bool controller_network_interface::check_write_msg(){
	OBJECT *obj = OBJECTHDR(this);
	int64 *pMarketId;
	char _buffer[64];
	
	if(0 == gl_get_value_by_name(obj->parent, "proxy_bid_ready", _buffer, 63)){
		gl_error("unable to read proxy_bid_ready from parent controller!");
		return false;
	}
	
	if(0 == strcmp(_buffer, "TRUE")){
		write_msg = true;
		
	} else {
		write_msg = false;
	}

	return write_msg;
}

/* check if it's time to send a message and poll the parent's target property for updated data */
int controller_network_interface::check_buffer(){

	write_msg = false;

	if(true == check_write_msg()){
		send_bid_request();
	}

	return 1;
}


/**
 *	nm		- the message being processed (can be null)
 *	return	- the next message in the stack that is not ready to be processed (punted for later)
 *
 *	handle_inbox recursively checks to see if a message in the inbox is ready to be received,
 *		based on its 'arrival time', a function of the transmission time and controller_network latency.
 *		Although messages are arranged in a stack, the function traverses to the end of the
 *		stack and works its way back up, returning the new stack as it processes through
 *		the messages.  It is possible that not all the messages marked as 'delivered' are
 *		ready to be processed at this point in time.
 */
void controller_network_interface::handle_inbox(TIMESTAMP t1, mpi_network_message *nm){
	const char market_init_str[] = "market INIT ",
				 market_upd_str[] = "market ",
				 bid_rsp_str[] = "bid_rsp ";
	OBJECT *my = OBJECTHDR(this);
	
	
	int res = 0;
	
	//std::cout << "Handle inbox!!" << std::endl;
	// update interface, copy from nm
	// ** or process the message header, then route as appropriate to the functions below
	//if(t1 >= nm->rx_done_sec){
		LOCK_OBJECT(my);
		this->curr_buffer_size = nm->buffer_size;
		memset(data_buffer, 0, 1024);
		memcpy(this->data_buffer, nm->message, nm->buffer_size);
		UNLOCK_OBJECT(my);

		gl_output("cnif::h-inbox()@%lli msg (%i) == '%s'", t1, nm->buffer_size, data_buffer);
		// read first word as type indicator
		if(0 == strncmp(data_buffer, market_init_str, strlen(market_init_str))){
			gl_output("cnif->process_market_init()");
			res = this->process_market_init();
			if(0 == res){
				gl_error("handle_inbox(): error when processing market_init");
				
			}
			
		} else if(0 == strncmp(data_buffer, market_upd_str, strlen(market_upd_str))){
			gl_output("cnif->process_market_update()");
			res = this->process_market_update();
			if(0 == res){
				gl_error("handle_inbox(): error when processing market_update");
				
			}
			
		} else if(0 == strncmp(data_buffer, bid_rsp_str, 8)){
			gl_output("cnif->process_bid_response()");
			res = this->process_bid_response();
			if(0 == res){
				gl_error("handle_inbox(): error when processing bid_response");
				
			}
			
		} else {
			gl_error("handle_inbox(): unrecognized message type in '%s'", data_buffer);
			
		}
		
	/*} else {
		gl_output("Handle inbox did not process the message %ld %ld %s",t1, nm->rx_done_sec,nm->message);
		return nm;
	}*/
}

// the only 'outbound' message
int controller_network_interface::send_bid_request(){
	// bid [price] [quantity] [bid_id (0 if none yet)] [state {on, off, unk}]
	char buffer[64], namestr[32];
	char *message;
	double *p, *q;
	int64 *k, *id;
	enumeration *st;
	int rv = 0;
	OBJECT *obj;

	obj = OBJECTHDR(this);
	// check that parent is in BM_PROXY
	rv = gl_get_value_by_name(obj->parent, "bid_mode", buffer, 64);
	if(0 == rv){
		gl_error("send_bid_request(): unable to check parent bid_mode");
		return 0;
	}
	if(0 != strcmp(buffer, "PROXY")){ // see controller.cpp
		gl_error("send_bid_request(): parent controller is not in PROXY bid_mode");
		return 0;
	}

	// check that parent has received initial market information
	rv = gl_get_value_by_name(obj->parent, "proxy_state", buffer, 64);
	if(0 == rv){
		gl_error("send_bid_request(): unable to check parent proxy_state");
		return 0;
	}
	if(0 != strcmp(buffer, "READY")){ // see controller.cpp
		gl_error("send_bid_request(): parent controller proxy_state is not READY");
		return 0;
	}

	// 	having started to send the bid, flip the flag saying that a bid is ready to send.
	if(0 == wrap_svbn("send_bid_request", "proxy_bid_ready", "FALSE"))	return 0;
	
	// retreive 'bid_price', 'bid_quantity', [object state: ON/OFF/UNKNOWN], 'bid_id'*
	if( 0 == (p = gl_get_double(obj->parent, bid_price_prop))){
		gl_error("unable to read bid_price_prop for %s", gl_name(obj, namestr, 31));
		return 0;
	}
	if( 0 == (q = gl_get_double(obj->parent, bid_quant_prop))){
		gl_error("unable to read bid_quant_prop for %s", gl_name(obj, namestr, 31));
		return 0;
	}
	if( 0 == (st = gl_get_enum(obj->parent, obj_state_prop))){
		gl_error("unable to read obj_state_prop for %s", gl_name(obj, namestr, 31));
		return 0;
	}
	if( 0 == (k = gl_get_int64(obj->parent, bid_id_prop))){
		gl_error("unable to read bid_id_prop for %s", gl_name(obj, namestr, 31));
		return 0;
	}
	if( 0 == (id = gl_get_int64_by_name(obj->parent, "proxy_market_id"))){
		gl_error("unable to read proxy_market_id for %s", gl_name(obj, namestr, 31));
		return 0;
	}
	// serialize
	message = (char *)malloc(256);
	gl_output("%s.CNIF::SEND() = bidreq %lli %lg %lg %i %lli %lli", gl_name(obj, namestr, 31), gl_globalclock, *p, *q, *st, *k, *id);
	sprintf(message, "bidreq %lli %lg %lg %i %lli %lli", gl_globalclock, *p, *q, *st, *k, *id);
	// outbox/send
	mpi_network_message *nm =new mpi_network_message(); //(mpi_network_message *)malloc(sizeof(mpi_network_message));
	//memset(nm, 0, sizeof(mpi_network_message)); //class should be allocted with new, class allocation with malloc is undefined behavour C++ spec.
	nm->send_message(obj, dst_name, message, (int)strlen(message));
	gl_output("CNIF::SEND(to '%s') = '%s'", dst_name, message);
	// lock interface while touching outbox
	free(message);
	LOCK_OBJECT(obj);
	uint32 size;
	char* buffer2=nm->serialize(&size);
	sim_comm::Message *msg2=new sim_comm::Message(nm->from_name,nm->to_name,gl_globalclock,(uint8_t *)buffer2,size);
	this->myInterface->clear();
	this->myInterface->send(msg2);
	UNLOCK_OBJECT(obj);
	delete nm;
	delete[] buffer2;
	return 1;
}

// market_nif should send this early on, needed to set up the controller
int controller_network_interface::process_market_init(){
	int rv = 0;
	OBJECT *obj;
	char buffer[64];
	char curr_time_str[64];
	// "market init [id] [period] [price_cap] [avg] [stdev] [price] [unit]"
	//	- 'init' is a literal, as there should not yet be any market activity.
	char *lead_str, *time_str, *id_str, *period_str, *cap_str, *avg_str, 
		*stdev_str, *price_str, *unit_str, *saveptr;
	char *nowstr = "NOW";
	
	obj = OBJECTHDR(this);

	// deserialize message
//	gl_output("tokenizing '%s'", this->data_buffer);
	lead_str =		strtok_r(this->data_buffer, " \r\t\n", &saveptr);
	if(0 != strcmp(lead_str, "market")){
		gl_error("process_market_init()");
		return 0;
	}
	time_str =		strtok_r(0, " \r\t\n", &saveptr);
//	gl_output("time_str = '%s'", time_str);
	id_str =		strtok_r(0, " \r\t\n", &saveptr); // ID likely useless here...
//	gl_output("id_str = '%s'", id_str);
	period_str =	strtok_r(0, " \r\t\n", &saveptr);
//	gl_output("period_str = '%s'", period_str);
	cap_str =		strtok_r(0, " \r\t\n", &saveptr);
//	gl_output("cap_str = '%s'", cap_str);
	avg_str =		strtok_r(0, " \r\t\n", &saveptr);
//	gl_output("avg_str = '%s'", avg_str);
	stdev_str =		strtok_r(0, " \r\t\n", &saveptr);
//	gl_output("stdev_str = '%s'", stdev_str);
	price_str =		strtok_r(0, " \r\t\n", &saveptr);
//	gl_output("price_str = '%s'", price_str);
//	unit_str =		strtok_r(0, " \r\t\n", &saveptr);
	
	// check that parent is in BM_PROXY
	rv = gl_get_value_by_name(obj->parent, "bid_mode", buffer, 64);
	if(0 == rv){
		gl_error("process_market_init(): unable to check parent bid_mode");
		return 0;
	}
	if(0 != strcmp(buffer, "PROXY")){ // see controller.cpp
		gl_error("process_market_init(): parent controller is not in PROXY bid_mode");
		return 0;
	}

	// check that parent is not initialized
	rv = gl_get_value_by_name(obj->parent, "proxy_state", buffer, 64);
	if(0 == rv){
		gl_error("process_market_init(): unable to check parent proxy_state");
		return 0;
	}
	if(0 != strcmp(buffer, "INIT")){ // see controller.cpp
		gl_error("process_market_init(): parent controller is not in PROXY bid_mode");
		return 0;
	}

//	nowstr = (char *)malloc(8);
//	strcpy(nowstr, "NOW");
	// copy values to parent
	//	- init_price, period, unit, price_cap, avg, stdev
	//gl_set_value_by_name(parent, "proxy_init_price", init_price_str);
//	if(0 == wrap_svbn("process_market_init", "proxy_market_id", id_str))	return 0;
	if(0 == wrap_svbn("process_market_init", "proxy_market_id", id_str))	return 0;
	if(0 == wrap_svbn("process_market_init", "proxy_market_period", period_str))	return 0;
//	if(0 == wrap_svbn("process_market_init", "proxy_market_unit", unit_str))	return 0;
	if(0 == wrap_svbn("process_market_init", "proxy_price_cap", cap_str))	return 0;
	if(0 == wrap_svbn("process_market_init", "proxy_avg", avg_str))	return 0;
	if(0 == wrap_svbn("process_market_init", "proxy_stdev", stdev_str))	return 0;
//	gl_output("setting proxy_clear_time to '%s' (%x)", nowstr, nowstr);
	if(0 == wrap_svbn("process_market_init", "proxy_clear_time", nowstr))	return 0;

	return 1;
}

// feedback for send_bid_request
int controller_network_interface::process_bid_response(){
	int rv = 0;
	OBJECT *obj = 0;
	char buffer[64];
	// "bid_rsp [rv] [bid_id]"
	char *lead_str, *ts_str, *rv_str, *id_str, *saveptr;
	

	obj = OBJECTHDR(this);

	// deserialize into ID# & return code
	lead_str =	strtok_r(this->data_buffer, " \r\t\n", &saveptr);
	if(0 != strcmp(lead_str, "bid_rsp")){
		gl_error("");
		return 0;
	}
	// check that parent is in BM_PROXY
	rv = gl_get_value_by_name(obj->parent, "bid_mode", buffer, 64);
	if(0 == rv){
		gl_error("process_bid_response(): unable to check parent bid_mode");
		return 0;
	}
	if(0 != strcmp(buffer, "PROXY")){ // see controller.cpp
		gl_error("process_bid_response(): parent controller is not in PROXY bid_mode");
		return 0;
	}

	ts_str =	strtok_r(0, " \r\t\n", &saveptr);
	rv_str =	strtok_r(0, " \r\t\n", &saveptr);
	id_str =	strtok_r(0, " \r\t\n", &saveptr);
	
	//if(0 == wrap_svbn("process_market_update", "proxy_clear_time", nowstr))	return 0;
	if(0 == wrap_svbn("process_bid_response", "proxy_bid_retval", rv_str))	return 0;
	if(0 == wrap_svbn("process_bid_response", "proxy_bid_id", id_str))	return 0;
	//	- this shall trigger a callback in the auction to check the return value
	//		and ...?
	return 1;
}

// periodic market information updates
int controller_network_interface::process_market_update(){
	int rv = 0;
	OBJECT *obj;
	char buffer[64];
	// "market [clear_time] [id] [period] [cap] [avg] [stdev] [price] [unit]"
	char *lead_str, *time_str, *id_str, *period_str, *cap_str, *avg_str, 
		*stdev_str, *price_str, *unit_str, *saveptr;

	obj = OBJECTHDR(this);

	// deserialize message, note type
	lead_str = strtok_r(this->data_buffer, " \r\t\n", &saveptr);
	if(0 != strcmp(lead_str, "market")){
		gl_error("");
		return 0;
	}
	time_str =		strtok_r(0, " \r\t\n", &saveptr);
	id_str =		strtok_r(0, " \r\t\n", &saveptr);
	period_str = 	strtok_r(0, " \r\t\n", &saveptr); // unused
	cap_str =		strtok_r(0, " \r\t\n", &saveptr);
	avg_str =		strtok_r(0, " \r\t\n", &saveptr);
	stdev_str =		strtok_r(0, " \r\t\n", &saveptr);
	price_str =		strtok_r(0, " \r\t\n", &saveptr);
//	unit_str =		strtok_r(0, " \r\t\n", &saveptr); // unused

	// check that parent is in BM_PROXY
	rv = gl_get_value_by_name(obj->parent, "bid_mode", buffer, 64);
	if(0 == rv){
		gl_error("process_market_update(): unable to check parent bid_mode");
		return 0;
	}
	if(0 != strcmp(buffer, "PROXY")){ // see controller.cpp
		gl_error("process_market_update(): parent controller is not in PROXY bid_mode");
		return 0;
	}
	// copy price, frac, cap, avg, stdev
	if(0 == wrap_svbn("process_market_update", "proxy_market_id", id_str))		return 0;
        if(0 == wrap_svbn("process_market_init", "proxy_market_period",period_str))	return 0;
	if(0 == wrap_svbn("process_market_update", "proxy_price_cap", cap_str))		return 0;
	if(0 == wrap_svbn("process_market_update", "proxy_avg", avg_str))			return 0;
	if(0 == wrap_svbn("process_market_update", "proxy_stdev", stdev_str))		return 0;
	if(0 == wrap_svbn("process_market_update", "proxy_clear_price", price_str))	return 0;
	// copy clear time
	if(0 == wrap_svbn("process_market_update", "proxy_clear_time", time_str))	return 0;
	//	** the controller will update its control using notify_proxy_clear_time **
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller_network_interface(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(controller_network_interface::oclass);
	if (*obj!=NULL)
	{
		controller_network_interface *my = OBJECTDATA(*obj,controller_network_interface);
		gl_set_parent(*obj,parent);
		try {
			my->create();
		}
		catch (const char *msg)
		{
			gl_error("%s::%s.create(OBJECT **obj={name='%s', id=%d},...): %s", (*obj)->oclass->module->name, (*obj)->oclass->name, (*obj)->name, (*obj)->id, msg);
			return 0;
		}
		return 1;
	}
	return 0;
}

EXPORT int init_controller_network_interface(OBJECT *obj)
{
	controller_network_interface *my = OBJECTDATA(obj,controller_network_interface);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_controller_network_interface(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,controller_network_interface)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_controller_network_interface(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	controller_network_interface *my = OBJECTDATA(obj,controller_network_interface);
	try {
		TIMESTAMP t2 = TS_NEVER;
		if(pass == PC_BOTTOMUP){
			t2 = my->sync(obj->clock, t1);
		} else if(pass == PC_POSTTOPDOWN){
			t2 = my->postsync(obj->clock,t1);
		} else if(pass == PC_PRETOPDOWN){
			t2 = my->presync(obj->clock, t1);
		}
		obj->clock = t1;
		return t2;
	}
	catch (char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.sync(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		return 0;
	}
	catch (const char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.sync(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		return 0;
	}
}

EXPORT TIMESTAMP commit_controller_network_interface(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	controller_network_interface *my = OBJECTDATA(obj,controller_network_interface);
	return my->commit(t1, t2);
}

//EXPORT int notify_controller_network_interface(OBJECT *obj, int update_mode, PROPERTY *prop){
//	controller_network_interface *my = OBJECTDATA(obj,controller_network_interface);
//	return my->notify(update_mode, prop);
//}
/**@}**/
