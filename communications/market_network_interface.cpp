/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file market_network_interface.cpp
	@addtogroup market_network_interface Performance market_network_interface object
	@ingroup comm

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "market_network_interface.h"
#include "integrator.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* market_network_interface::oclass = NULL;
CLASS *market_network_interface::pclass = NULL;
market_network_interface *market_network_interface::defaults = NULL;

// the constructor registers the class and properties and sets the defaults
market_network_interface::market_network_interface(MODULE *mod) : network_interface(mod)
{
	
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"market_network_interface",sizeof(market_network_interface),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the market_network_interface class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
		else
			oclass->trl = TRL_CONCEPT;
		pclass = network_interface::oclass;
		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "network_interface",
			PT_char256, "average_price_prop", PADDR(avg_prop_name),
			PT_char256, "stdev_price_prop", PADDR(stdev_prop_name),
			PT_char256, "adjust_price_prop", PADDR(adj_price_prop_name),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the market_network_interface properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
		defaults = this;
		memset(this, 0, sizeof(market_network_interface));
	}
}

// create is called every time a new object is loaded
int market_network_interface::create() 
{
	adj_price_prop_name = NULL;
	broadcast = false;
	cnif_count = 0;
	int szof = sizeof(market_network_interface);
	return 1;
}

int market_network_interface::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	// input validation checks

	if(0 == parent){
		GL_THROW("network interface does not have a parent object");
	}
	/*if(to == 0){
		GL_THROW("network interface is not connected to a network");
	}*/
	/*if(!gl_object_isa(to, "mpi_network", "comm")){
		GL_THROW("network interface is connected to a non-network object");
	} else {
		pNetwork = OBJECTDATA(to, mpi_network);
	}*/

	// operational variable defaults

	//pNetwork->attach(this);

	// SPECIFC TO MNIF

	// get market_id prop from parent
	market_id_prop = gl_get_property(parent, "market_id");
	if(market_id_prop == 0){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find property \"market_id\" in parent '%s'", name);
		return 0;
	}
	avg_prop = gl_get_property(parent, avg_prop_name);
	if(avg_prop == 0){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find average price property \"%s\" in parent '%s'", avg_prop_name.get_string(), name);
		return 0;
	}
	stdev_prop = gl_get_property(parent, stdev_prop_name);
	if(stdev_prop == 0){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find stdev property \"%s\" in parent '%s'", stdev_prop_name.get_string(), name);
		return 0;
	}
	if(adj_price_prop_name[0] != '\0'){
		adj_price_prop = gl_get_property(parent, adj_price_prop_name);
		if(adj_price_prop == 0){
			char name[64];
			gl_name_object(parent, name, 63);
			gl_error("init(): unable to find the adjust price property \"%s\" in parent '%s'", adj_price_prop_name.get_string(), name);
			return 0;
		}
	}
	pAdjPriceProp = gl_get_double(parent, adj_price_prop);
	if(pAdjPriceProp == NULL){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to access adjust price property \"%s\" in parent '%s'", adj_price_prop_name.get_string(), name);
		return 0;
	}
	price_prop = gl_get_property(parent, "current_market.clearing_price");
	if(price_prop == 0){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find clearing price property \"current_market.clearing_price\" in parent '%s'", name);
		return 0;
	}
	unit_prop = gl_get_property(parent, "unit");
	if(unit_prop == 0){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find unit property \"unit\" in parent '%s'", name);
		return 0;
	}
	pricecap_prop = gl_get_property(parent, "price_cap");
	if(pricecap_prop == 0){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find property \"price_cap\" in parent '%s'", name);
		return 0;
	}
	period_prop = gl_get_property(parent, "period");
	if(period_prop == 0){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find property \"dPeriod\" in parent '%s'", name);
		return 0;
	}

	gm4t_func = gl_get_function(parent, "get_market_for_time");
	if(0 == gm4t_func){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find function 'get_market_for_time' in parent object '%s'", name);
		return 0;
	}

	// get bid function from parent
	bid_func = gl_get_function(parent, "submit_bid_state");
	if(0 == bid_func){
		char name[64];
		gl_name_object(parent, name, 63);
		gl_error("init(): unable to find function 'submit_bid' in parent object '%s'", name);
		return 0;
	}

	// set values to send market init message to any/all subscribers
	write_msg = true;
	write_init = true;

	//register to fncs
	char name[64];
	gl_name_object(hdr,name,63);
	myInterface=sim_comm::Integrator::getCommInterface(name);

	FINDLIST *cnifs;
	int index = 0;
	OBJECT *obj = 0;
	cnifs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"controller_network_interface",FT_END);
	if(cnifs == NULL){
		gl_warning("No controller_network_interface objects were found. No Broadcast messages will be sent.");
		broadcast = false;
	} else {
		pCnif = (OBJECT **)gl_malloc(cnifs->hit_count*sizeof(OBJECT*));
		if(pCnif == NULL || cnifs->hit_count == 0){
			gl_error("Failed to allocate controller_network_interface array.");
			return 0;
		} else {
                        broadcast = true;
			cnif_count = cnifs->hit_count;
			while((obj = gl_find_next(cnifs, obj)) != NULL && index < cnif_count){
				pCnif[index] = obj;
				++index;
			}
		}
	}

	return 1;
}

int market_network_interface::isa(char *classname){
	return strcmp(classname,"market_network_interface")==0;
}

TIMESTAMP market_network_interface::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	return TS_NEVER; 
}

double *market_network_interface::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}

TIMESTAMP market_network_interface::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	const char bid_req_str[] = "bidreq ";
	const char trans_req_str[] = "adjprice ";
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP rv = TS_NEVER;
	int res;
	while(this->myInterface->hasMoreMessages()){
	  sim_comm::Message *msg=this->myInterface->getNextInboxMessage();
	  if(0 == strncmp((char*)msg->getData(), "sellprice", 9)){
		  sscanf((char*)msg->getData(), "sellprice %lf", pAdjPriceProp);
		  rv = t1;
	  }else{
		  mpi_network_message *nm=new mpi_network_message((char*)msg->getData(),msg->getSize(),obj);
		  
		  this->curr_buffer_size = nm->buffer_size;
		  memset(data_buffer, 0, 1024);
		  memcpy(this->data_buffer, nm->message, nm->buffer_size);

		  gl_verbose("mnif:h-inbox()@%li msg (%i) == '%s'", t1, nm->buffer_size, data_buffer);
		  // read first word as type indicator
		  if(0 == strncmp(data_buffer, bid_req_str, 7)){
			  res = this->process_bid(nm);
			  if(0 == res){
				  gl_error("handle_inbox(): error when processing process_bid");	 
			  }
		  } else {
			gl_error("handle_inbox(): unrecognized message type in '%s'", data_buffer);
		
	  	  }
		delete nm;
	  }
	  
	  delete msg;
	  
	}
	return rv;
}

TIMESTAMP market_network_interface::postsync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	check_buffer();
	return TS_NEVER; 
}


/**
 * ni::commit() is where data is pulled from its parent, copied into the interface,
 *	and written to a message to be sent to another interface.
 */
TIMESTAMP market_network_interface::commit(TIMESTAMP t1, TIMESTAMP t2){
	
	return 1;
}

// there isn't a good way to set_value with straight binary data, so going about it using
//	direct sets, and 'notify'ing with a follow-up function call.
int market_network_interface::on_message(){
	return 0;
}

bool market_network_interface::check_write_msg(){
	OBJECT *obj = OBJECTHDR(this);
	int64 *pMarketId;
	
	pMarketId = gl_get_int64(obj->parent, market_id_prop);

	if(write_init){
//		write_init = false;
		write_msg = true;
		last_market_id = *pMarketId;
		last_message_send_time = gl_globalclock;
	} else {
		// send updates iff the market ID changes
		if(0 == pMarketId){
			gl_error("check_write_msg(): unable to get market_id_prop from parent");
		}
		if(*pMarketId != last_market_id){
			// market_id updated, prep to send a message
			write_msg = true;
			last_market_id = *pMarketId;
			last_message_send_time = gl_globalclock;
		} else {
			write_msg = false;
		}
	}
	return write_msg;
}

/*void market_network_interface::send_buffer_msg(){
	OBJECT *obj = OBJECTHDR(this);
	void *b = (target->addr);
	void *c = ((void *)((obj->parent)?((obj->parent)+1):NULL));
	void *d = (void *)((int64)c + (int64)b);

	curr_buffer_size = buffer_size;
	memset(data_buffer, 0, buffer_size);
	memcpy(data_buffer, (void *)(d), buffer_size);
	mpi_network_message *nm = new mpi_network_message();// (mpi_network_message *)malloc(sizeof(mpi_network_message));
	//memset(nm, 0, sizeof(network_message));
	nm->send_message(this, gl_globalclock); // this hooks the message in to the network
	nm->next = outbox;
	outbox = nm;
	last_message_send_time = gl_globalclock;
}*/

/* check if it's time to send a message and poll the parent's target property for updated data */
int market_network_interface::check_buffer(){
	
	write_msg = false;
	//void *a = OBJECTDATA((void),obj->parent);

	if(broadcast == true && true == check_write_msg()){
		send_market_update();	
	}

	return 1;
}

/*TIMESTAMP market_network_interface::handle_inbox(TIMESTAMP t1){
	// process the messages in the inbox, in FIFO fashion, even though it's been stacked up.
	mpi_network_message *rv = handle_inbox(t1, inbox);
	//delete inbox;
	
	inbox = rv;
	next_msg_time = TS_NEVER;
	return next_msg_time;
}*/

/**
 *	nm		- the message being processed (can be null)
 *	return	- the next message in the stack that is not ready to be processed (punted for later)
 *
 *	handle_inbox recursively checks to see if a message in the inbox is ready to be received,
 *		based on its 'arrival time', a function of the transmission time and network latency.
 *		Although messages are arranged in a stack, the function traverses to the end of the
 *		stack and works its way back up, returning the new stack as it processes through
 *		the messages.  It is possible that not all the messages marked as 'delivered' are
 *		ready to be processed at this point in time.
 */
/*mpi_network_message *market_network_interface::handle_inbox(TIMESTAMP t1, mpi_network_message *nm){
	const char bid_req_str[] = "bidreq ";
	OBJECT *my = OBJECTHDR(this);
	mpi_network_message *rv = 0;
	int res = 0;

	if(nm != 0){ // it's been stacked, process in reverse order
		rv = handle_inbox(t1, nm->next);
		if (rv != nm->next){
			delete nm->next;
			//free(nm->next);
			nm->next = rv;
		}
	} else {
		return 0;
	}
	// update interface, copy from nm
	if(t1 >= nm->rx_done_sec){
		LOCK_OBJECT(my);
		this->curr_buffer_size = nm->buffer_size;
		memset(data_buffer, 0, 1024);
		memcpy(this->data_buffer, nm->message, nm->buffer_size);
		UNLOCK_OBJECT(my);

		gl_output("mnif:h-inbox()@%li msg (%i) == '%s'", t1, nm->buffer_size, data_buffer);
		// read first word as type indicator
		if(0 == strncmp(data_buffer, bid_req_str, 7)){
			res = this->process_bid(nm);
			if(0 == res){
				gl_error("handle_inbox(): error when processing process_bid");
				return 0;
			}
			return nm->next;
		} else {
			gl_error("handle_inbox(): unrecognized message type in '%s'", data_buffer);
			return 0;
		}
	} else {
		return nm;
	}
}*/

int market_network_interface::process_bid(mpi_network_message *mnmsg){
	int rv = 0, stateval, i;
	int result;
	OBJECT *obj, from_obj;
//	char buffer[64];
	// "bidreq [timestamp] [price] [quant] [state] [bid_id]"
	char *lead_str, *time_str, *price_str, *quant_str, *state_str, *bid_str, *saveptr, *idstr;
	double price, quant;
	int64 bid, timeat, m_id;
	int state_test;
	int64 sent_for_market;
	int64 market_id, *m_id_ptr;
	int64 bid_res, bid_rv;
	const struct {
		char * name;
		int value;
	} statemap[] = {
		{"ON", 2},
		{"OFF", 1},
		{"UNKNOWN", 0}
	}; // see bid_state in market.h
	obj = OBJECTHDR(this);

	gl_verbose("mnif::proc_bid() ~ '%s'", mnmsg->message);
	// deserialize message
	lead_str =		strtok_r(this->data_buffer, " \r\t\n", &saveptr);
	if(0 != strcmp(lead_str, "bidreq")){
		gl_error("process_bid(): did not recognize leading token");
		return 0;
	}
	time_str =		strtok_r(0, " \r\t\n", &saveptr);
	price_str =		strtok_r(0, " \r\t\n", &saveptr);
	quant_str =		strtok_r(0, " \r\t\n", &saveptr);
	state_str =		strtok_r(0, " \r\t\n", &saveptr); // {ON, OFF, UNKNOWN}, see controller.cpp - "bid_state"
	bid_str =		strtok_r(0, " \r\t\n", &saveptr);
	idstr =		strtok_r(0, " \r\t\n", &saveptr);

	// copy to local values
	//sscanf(price_str, "%lg %lg %*s %li", &price, &quant, &bid);
	timeat = strtoll(time_str, NULL, 10);
	if(timeat < -1){
		gl_error("process_bid(): bid time not usable");
		return 0;
	}
	price = strtod(price_str, NULL);
	if(isnan(price)){
		gl_error("process_bid(): price token is NaN");
		return 0;
	}
	quant = strtod(quant_str, NULL);
	if(isnan(quant)){
		gl_error("process_bid(): quantity token is NaN");
		return 0;
	}
	bid = strtoll(bid_str, NULL, 10);
	if(bid < -1){
		gl_error("process_bid(): bid ID not usable");
		return 0;
	}
	m_id = strtoll(idstr, NULL, 10);
	if(bid < -1){
		gl_error("process_bid(): market ID not usable");
		return 0;
	}
	
	if(0 != state_str){
		if(0 != isdigit(state_str[0])){
			state_test = atoi(state_str);
			stateval = state_test;
		} else {
			for(i = 0; i < 3; ++i){
				if(0 == strcmp(statemap[i].name, state_str)){
					stateval = statemap[i].value;
					break;
				}
			}
			if(3 == i){
				gl_error("process_bid(): unrecognized state value");
				return 0;
			}
		}
	}

	// check if we're still in the same market we sent this bid request foreign
	sent_for_market = (*gm4t_func)(obj->parent, timeat);
	if(sent_for_market == -1){
//		bid_rv = -3; // late bid
//		bid_res = 0;
		gl_verbose("process_bid(): late bid, sending (0, -3)");
		rv = send_bid_response(mnmsg->from_name, 0, -3);
		return rv;
	}
	m_id_ptr = gl_get_int64_by_name(obj->parent, "market_id");
	if(*m_id_ptr > m_id){
		gl_verbose("process_bid(): late bid, sending (0, -3) [catch two]");
		rv = send_bid_response(mnmsg->from_name, 0, -3);
		return rv;
	}
	// "else"
	// sanity checked inputs, make the call
	if(mnmsg->from != 0){
		result = (*(bid_func))(obj->parent, mnmsg->from, quant, price, stateval, bid);
	} else {
		result = (*(bid_func))(obj->parent, obj, quant, price, stateval, bid);
	}
	bid_rv = result;
	gl_verbose("bid_func: %g, %g, %i, %li -> %i", quant, price, stateval, bid, result);
	if(bid_rv < 0){
		bid_res = 0;
	} else {
		bid_res = bid_rv;
	}
	// send_bid_response
	rv = send_bid_response(mnmsg->from_name, bid_res, bid_rv); // foreign sender, name only, even if they are local
	return rv;
}

int market_network_interface::send_bid_response(char *dst, int64 bid_res, int64 bid_rv){
	char *message;

	mpi_network_message *mpinm;
	OBJECT *obj = OBJECTHDR(this);
	size_t len;
	int rv = 0;
	if(0 == dst){
		gl_error("send_bid_response(): no destination specified (null ptr)");
		return 0;
	}
	// inputs are local variables, no need to fetch them.
	// straights results do not need sanity-checking?
	// inputs are "bid_rv" and "bid_res"
	message = (char *)malloc(256);
	sprintf(message, "bid_rsp %li %li %li", gl_globalclock, bid_rv, bid_res);
	len = strlen(message);
	// place on outbox
	mpinm = new mpi_network_message();
	rv = mpinm->send_message(OBJECTHDR(this), dst, message, (int)len);
	free(message);
	// need to lock this interface while touching outbox
	LOCK_OBJECT(obj);
	uint32 size;
	char* buffer=mpinm->serialize(&size);
	sim_comm::Message *msg=new sim_comm::Message(mpinm->from_name,mpinm->to_name,gl_globalclock,(uint8_t*)buffer,size);
	this->myInterface->send(msg);
	delete[] buffer;
	UNLOCK_OBJECT(obj);
	delete mpinm;
	return 1;
}

int market_network_interface::send_market_update(){
	OBJECT *obj;
	char timestr[64];
	char *pUnit;
	double period;
	double pricecap, avg, stdev, price;
	double *pPricecap, *pAvg, *pStdev, *pPrice, *pPeriod;
	int index = 0;
	
	memset(timestr,0,64);
	if(true == write_init){
		// write 'init' instead of timestamp
		strcpy(timestr,"INIT");
		//sprintf(timestr, "INIT");
		write_init = false;
	} else {
		sprintf(timestr, "%lu", gl_globalclock);
	}
	obj = OBJECTHDR(this);
	pAvg = gl_get_double(obj->parent, avg_prop);
	if(0 == pAvg){
		gl_error("send_market_update(): unable to access average price prop '%s'", avg_prop_name.get_string());
		return 0;
	}
	pStdev = gl_get_double(obj->parent, stdev_prop);
	if(0 == pStdev){
		gl_error("send_market_update(): unable to access market ID prop '%s'", stdev_prop_name.get_string());
		return 0;
	}
	pPeriod = gl_get_double(obj->parent, period_prop);
	if(0 == pPeriod){
		gl_error("send_market_update(): unable to access market period prop 'period'");
		return 0;
	}
	pPricecap = gl_get_double(obj->parent, pricecap_prop);
	if(0 == pPricecap){
		gl_error("send_market_update(): unable to access price cap prop 'price_cap'");
		return 0;
	}
	pPrice = gl_get_double(obj->parent, price_prop);
	if(0 == pPrice){
		gl_error("send_market_update(): unable to access price prop 'current_market.clearing_price'");
		return 0;
	}
	//printf("%lf\n",*pPrice);
	pUnit = gl_get_string(obj->parent, unit_prop);
	if(0 == pUnit){
		gl_error("send_market_update(): unable to access unit prop 'unit'");
		return 0;
	}

	// market ID already checked and updated as part of check_write_msg()
	memset(data_buffer,0,256);
	avg = *pAvg;
	stdev = *pStdev;
	period = *pPeriod;
	price = *pPrice;
	pricecap = *pPricecap;
	

	// "market INIT/[time] [id] [period] [price_cap] [avg] [stdev] [price] [unit]"
	//sprintf(data_buffer, "market %s %"FMT_INT64" %f %f %f %f %s", timestr, last_market_id, 
	//	period, pricecap, avg, stdev, price, pUnit[0] == 0 ? "" : pUnit);
	/*printf("market %s %lu %f %f %f %f %f\n", timestr, last_market_id,
                period, pricecap, avg, stdev, price);*/
	sprintf(data_buffer, "market %s %lu %f %f %f %f %f", timestr, last_market_id, 
                period, pricecap, avg, stdev, price);
	// build message object and add to the stack
	curr_buffer_size = (int32)strlen(data_buffer);
#if 0
	mpi_network_message *nm =new mpi_network_message(); //(mpi_network_message *)malloc(sizeof(mpi_network_message));
	//memset(nm, 0, sizeof(mpi_network_message));
	nm->send_message(obj, "*", data_buffer, curr_buffer_size);
	gl_verbose("MNIF::SEND() = '%s'", data_buffer);
	// lock interface while touching outbox
	LOCK_OBJECT(obj);
	uint32 size;
	char* buffer=nm->serialize(&size);
	sim_comm::Message *msg2=new sim_comm::Message(nm->from_name,nm->to_name,gl_globalclock,(uint8_t *)buffer,size);
	this->myInterface->send(msg2);
	UNLOCK_OBJECT(obj);
	delete[] buffer;
	delete nm;
#else
	for( int index=0; index < cnif_count; index++) {
		mpi_network_message *nm =new mpi_network_message(); //(mpi_network_message *)malloc(sizeof(mpi_network_message));
		//memset(nm, 0, sizeof(mpi_network_message));
		nm->send_message(obj, pCnif[index]->name, data_buffer, curr_buffer_size);
		gl_verbose("MNIF::SEND() = '%s'", data_buffer);
		// lock interface while touching outbox
		LOCK_OBJECT(obj);
		uint32 size;
		char* buffer=nm->serialize(&size);
		sim_comm::Message *msg2=new sim_comm::Message(nm->from_name,nm->to_name,gl_globalclock,(uint8_t *)buffer,size);
		this->myInterface->send(msg2);
		UNLOCK_OBJECT(obj);
		delete[] buffer;
		delete nm;
	}
#endif
	write_msg = false;
	return 1;
}



//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_market_network_interface(OBJECT **obj, OBJECT *parent)
{
#if 0
	*obj = gl_create_object(market_network_interface::oclass);
	if (*obj!=NULL)
	{
		market_network_interface *my = OBJECTDATA(*obj,market_network_interface);
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
#endif
	try
	{
		*obj = gl_create_object(market_network_interface::oclass);
		if (*obj!=NULL)
		{
			market_network_interface *my = OBJECTDATA(*obj,market_network_interface);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(market_network_interface);
}

EXPORT int init_market_network_interface(OBJECT *obj)
{
	market_network_interface *my = OBJECTDATA(obj,market_network_interface);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_market_network_interface(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,market_network_interface)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_market_network_interface(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	market_network_interface *my = OBJECTDATA(obj,market_network_interface);
	try {
		TIMESTAMP t2 = TS_NEVER;
		if(pass == PC_BOTTOMUP){
			t2 = my->sync(obj->clock, t1);
		} else if(pass == PC_POSTTOPDOWN){
			t2 = my->postsync(obj->clock, t1);
		} else if(pass == PC_PRETOPDOWN){
			t2 = my->presync(obj->clock, t1);
		}
		obj->clock = t1;
		return t2;
	}
	catch (const char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.init(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		return 0;
	}
}

EXPORT TIMESTAMP commit_market_network_interface(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	market_network_interface *my = OBJECTDATA(obj,market_network_interface);
	return my->commit(t1, t2);
}
