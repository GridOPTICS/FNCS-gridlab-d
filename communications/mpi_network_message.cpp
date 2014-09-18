/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file mpi_network_message.cpp
	@addtogroup mpi_network_message Performance network_message object
	@ingroup comm

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include "mpi_network_message.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* mpi_network_message::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
mpi_network_message::mpi_network_message(MODULE *mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"mpi_network_message",sizeof(mpi_network_message),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the mpi_network_message class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char256, "from", PADDR(from_name), PT_DESCRIPTION, "the network interface this message is being sent from",
			PT_char256, "to", PADDR(to_name), PT_DESCRIPTION, "the network interface this message is being sent to",
			PT_char1024, "message", PADDR(message), PT_DESCRIPTION, "the contents of the data being sent, whether binary, ASCII, or faked",
			PT_int16, "buffer_size", PADDR(buffer_size), PT_DESCRIPTION, "number of bytes held within the message buffer",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the mpi_network_message properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

mpi_network_message::mpi_network_message(char* serialized, size_t size,OBJECT *thisptr)
{
  this->deserialize(serialized,size,thisptr);
}



mpi_network_message::mpi_network_message(){
	from = to = 0;
	buffer_size = 0;
	memset(message, 0, 1024);
	memset(to_name, 0, 256);
	memset(from_name, 0, 256);

}

/*	send_message()
 *	In general, this function is used to prepare a network_object for serialization
 *	 and transport across a network that the generating interface is attached to.
 *	Throwbacks to the performance network involve inncluding subsecond transmission
 *	 time offsets and some localized message delivery completion values.
 */
int mpi_network_message::send_message(network_interface *nif, TIMESTAMP ts){
	char name[64];
	char *dana=gl_name(nif->destination, name, 63);
	printf("send_message: al sana %s\n",dana);
	return send_message(OBJECTHDR(nif), dana , nif->data_buffer, nif->buffer_size);
}

int mpi_network_message::send_message(OBJECT *f, OBJECT *t, char *m, int len){
	char namebuf[64];
	return send_message(f, gl_name(t, namebuf, 63), m, len);
}

int mpi_network_message::send_message(OBJECT *f, char *t, char *m, int len){
	from = f;
	gl_name(f, from_name, 255);
	to = 0;
	strcpy(to_name, t);
	memset(message,0,1024);
	strcpy(message, m); // should these be reality checked?
	buffer_size = len;
	// later serializes when it's time to feed through MPI
	return 1; // success
}

int mpi_network_message::send_message(){
	// check for from OBJECT
	if(from == 0){
		gl_error("send_message(): missing 'from' object");
		return 0;
	} else {
		gl_name(from, from_name, 255);
	}
	// check for to name or OBJECT
	if(to == 0){
		if(to_name[0] == 0){
			gl_error("send_message(): no 'to' object or destination name");
			return 0;
		} else {
			to = gl_get_object(to_name);
			// okay if null, assume foreign object
		}
	}
	// check for nonzero message
	if(message[0] == 0){
		// assuming ASCII data
		gl_warning("send_message(): data message is either missing, or binary");
	} else {
		// set buffer_size
		buffer_size = strlen(message);
	}

	return 1;
}

// create is called every time a new object is loaded
int mpi_network_message::create() 
{
	gl_error("mpi_network_message is an internally used class and should not be instantiated by a model");
	return 0;
}

int mpi_network_message::isa(char *classname){
	return strcmp(classname,"mpi_network_message")==0;
}

int mpi_network_message::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

// turn an mpi_net_msg into a long string that contains all of its data
char *mpi_network_message::serialize(uint32 *sz){
	char *buffer = 0;
	char name1[256], name2[256], szbuf[16];
	size_t name1sz, name2sz, szbufsz, datasz;

	if(0 == sz){
		gl_error("serialize(): size pointer is null");
		return 0;
	}

	// 'from' object should have been used to construct the message
	if(0 == gl_name(from, name1, 255)){
		gl_error("serialize(): unable to print name for 'from' object");
		return 0;
	}

	if(0 != to){
		if(0 == gl_name(to, name2, 255)){
			gl_error("serialize(): unable to print name for 'to' object");
			return 0;
		}
	} else if(0 != to_name[0]){
		strcpy(name2, to_name);
	} else{
		gl_error("serialize(): 'to' object not defined or named");
		return 0;
	}

	name1sz = strlen(name1);
	name2sz = strlen(name2);
	datasz = strlen(message);

	if(datasz < 1){
		// from a strlen() call, this is either an error or from a zero length string.
		gl_error("serialize(): data buffer has non-positive length");
		return 0;
	}

	// no reason to trust that it'll be using the WHOLE buffer
	sprintf(szbuf, "%i", datasz);
	szbufsz = strlen(szbuf);

	*sz = name1sz + name2sz + szbufsz + datasz + 4; // three spaces and a \0
	buffer = (char *)malloc(*sz);
	if(0 == buffer){
		gl_error("serialize(): malloc failure");
		return 0;
	}

	sprintf(buffer, "%s %s %s %s", name1, name2, szbuf, message);

	return buffer;
}

// take an input msg and parse it out into the mpi_net_msg members
int mpi_network_message::deserialize(char *msg, size_t sz, OBJECT *thisptr){
	char *buffer, *to_str, *from_str, *sz_str, *data_str, *ctxt;
	OBJECT *to_obj, *from_obj;
	int sz_res;
	int sz_data = 0;

	if(0 == msg){
		gl_error("deserialize(): null msg pointer");
		return 0;
	}
	if(sz < 1){
		gl_error("deserialize(): non-positive size value");
		return 0;
	}

	buffer = (char *)malloc(sz);
	if(0 == buffer){
		gl_error("deserialize(): malloc failure");
		return 0;
	}

	memcpy(buffer, msg, sz);
	from_str = strtok_r(buffer, " ,\t\r\n", &ctxt);
	to_str = strtok_r(0, " ,\t\r\n", &ctxt);
	sz_str = strtok_r(0, " ,\t\r\n", &ctxt);
	data_str = strtok_r(0, "\t\r\n", &ctxt); // clear it to the line

	if(0 == to_str){
		gl_error("deserialize(): to_str token null ('%s', '%s')", msg, buffer);
		gl_error(" + from_str = %s", from_str);
		gl_error(" + sz = %i", sz);
		free(buffer);
		return 0;
	}
	if(0 == sz_str){
		gl_error("deserialize(): sz_str token null");
		free(buffer);
		return 0;
	}
	if(0 == data_str){
		gl_error("deserialize(): data_str token null");
		free(buffer);
		return 0;
	}
	if(to_str[0]=='*'){ //dest bcast is me!
	
	  gl_name(thisptr,to_name,255);
	  to_obj=thisptr;
	}
	else{
	  strcpy(to_name, to_str);
	  to_obj = gl_get_object(to_str);
	}
	if(0 == to_obj){
		// we deserialize the message when we deliver it, so no 'to' means we can't deliver it
		gl_error("deserialize(): unable to locate object '%s', unable to deliver message", to_str);
		free(buffer);
		return 0;
	} else {
		to = to_obj;
	}

	strcpy(from_name, from_str);
	from_obj = gl_get_object(from_str);
	// okay if from not found, could be a foreign object
	from = from_obj;
	
//	sz_data = strlen(data_str);

//	memcpy(message, data_str, data_sz);
//	message[data_sz] = 0; // manual trim

	sz_res = strtol(sz_str, 0, 10);
	if(sz_res < 0){
		gl_error("deserialize(): mpi_network was knowingly delivered a negative-length message");
		free(buffer);
		return 0;
	}

	memcpy(message, data_str, sz_res);
	message[sz_res] = 0;
	
//	strcpy(message, data_str);
	buffer_size = sz_res;
	free(buffer);
	return 1; // success
}

//	shuffle message into 'to' nif queue.
/*int mpi_network_message::deliver(){
	//std::cout << "DELIVER!!!" << std::endl;
	if(this->to == 0){
		gl_error("mpi_network_message::deliver(): message lacks 'to' object");
		return 0;
	}

	if(gl_object_isa(to, market_network_interface::oclass->name)){
		market_network_interface *mnif = OBJECTDATA(to, market_network_interface);
		LOCK_OBJECT(to);
		this->next = mnif->inbox;
		mnif->inbox = this;
		UNLOCK_OBJECT(to);
	} else if(gl_object_isa(to, controller_network_interface::oclass->name)){
		controller_network_interface *cnif = OBJECTDATA(to, controller_network_interface);
		LOCK_OBJECT(to);
		this->next = cnif->inbox;
		cnif->inbox = this;
		UNLOCK_OBJECT(to);		
	} else {
		gl_error("mnmsg::deliver(): unsupported network_interface type, could not deliver message");
		return 0;
	}
	return 1;
}*/

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_mpi_network_message(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(mpi_network_message::oclass);
	if (*obj!=NULL)
	{
		mpi_network_message *my = OBJECTDATA(*obj,mpi_network_message);
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

EXPORT TIMESTAMP sync_mpi_network_message(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass){
	obj->clock = t1;
	return TS_NEVER;
}

EXPORT int isa_mpi_network_message(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,mpi_network_message)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT int notify_mpi_network_message(OBJECT *obj, int update_mode, PROPERTY *prop){
	mpi_network_message *my = OBJECTDATA(obj,mpi_network_message);
	return my->notify(update_mode, prop);
}
/**@}**/
