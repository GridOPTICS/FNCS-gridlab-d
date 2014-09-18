#include "mpi_comm_test.h"


int test_mnmsg_send(){
	// need to test local->local send()
	// need to test local->foreign send()
	// in both cases, check from, from name, to name, buffer data, and buffer length
	int rv = 0;
	market_network_interface *local_mnif;
	controller_network_interface *local_cnif;
	mpi_network_message *mnmsg1, *mnmsg2, *mnmsg3;
	OBJECT *obj_local_mnif, *obj_local_cnif;
	char *local_mnif_name = "local_mnif",
		*local_cnif_name = "local_cnif",
		*frgn_cnif_name = "foreign_cnif";
	char *data_str = "price=0.07,avg=0.09,stdev=0.02";
	size_t data_sz = strlen(data_str);
	
	// build local market nif
	obj_local_mnif = (OBJECT *)malloc(sizeof(OBJECT) + sizeof(market_network_interface));
	if(0 == obj_local_mnif){
		gl_error("test_mnmsg_send: malloc failure");
		return 0;
	}
	local_mnif = OBJECTDATA(obj_local_mnif, market_network_interface);
	gl_set_object_name(obj_local_mnif, local_mnif_name);

	// build local controller nif
	obj_local_cnif = (OBJECT *)malloc(sizeof(OBJECT) + sizeof(controller_network_interface));
	if(0 == obj_local_cnif){
		gl_error("test_mnmsg_send: malloc failure");
		free(obj_local_mnif);
		return 0;
	}
	local_cnif = OBJECTDATA(obj_local_cnif, controller_network_interface);
	gl_set_object_name(obj_local_cnif, local_cnif_name);

	// build mpi_network_messages
	// mnmsg1 is local->local
	mnmsg1 = (mpi_network_message *)malloc(sizeof(mpi_network_message));
	if(0 == mnmsg1){
		gl_error("test_mnmsg_send: malloc failure");
		free(obj_local_mnif);
		free(obj_local_cnif);
		return 0;
	}
	memset(mnmsg1, 0, sizeof(mpi_network_message));
//	mnmsg1->from = obj_local_cnif;
//	mnmsg1->to = obj_local_mnif;
//	strcpy(mnmsg1->message, data_str);
//	mnmsg1->buffer_size = data_size;
	if(1 != mnmsg1->send_message(obj_local_cnif, obj_local_mnif, data_str, data_sz)){
		gl_error("test_mnmsg_send: error in send_message() for mnmsg1");
		++rv;
	}
	// mnmsg2 is local->foreign
	mnmsg2 = (mpi_network_message *)malloc(sizeof(mpi_network_message));
	if(0 == mnmsg2){
		gl_error("test_mnmsg_send: malloc failure");
		free(mnmsg1);
		free(obj_local_mnif);
		free(obj_local_cnif);
		return 0;
	}
	memset(mnmsg2, 0, sizeof(mpi_network_message));
//	mnmsg2->from = obj_local_mnif;
//	strcpy(mnmsg2->to_name, frgn_cnif_name);
//	strcpy(mnmsg2->message, data_str);
	mnmsg2->buffer_size = data_sz;
	if(1 != mnmsg2->send_message(obj_local_mnif, frgn_cnif_name, data_str, data_sz)){
		gl_error("test_mnmsg_send: error in send_message() for mnmsg2");
		++rv;
	}
	
	// mnmsg3 is malformed, with only a from_name and a to_name
	mnmsg3 = (mpi_network_message *)malloc(sizeof(mpi_network_message));
	if(0 == mnmsg3){
		gl_error("test_mnmsg_send: malloc failure");
		free(mnmsg2);
		free(mnmsg1);
		free(obj_local_mnif);
		free(obj_local_cnif);
		return 0;
	}
	memset(mnmsg3, 0, sizeof(mpi_network_message));
	strcpy(mnmsg3->to_name, local_mnif_name);
	strcpy(mnmsg3->from_name, frgn_cnif_name);
	strcpy(mnmsg3->message, data_str);
	mnmsg3->buffer_size = data_sz;

	// actually run send
	gl_output("The next call should report   'missing 'from' object'");
	if(0 != mnmsg3->send_message()){
		gl_error("test_mnmsg_send: negative test on mnmsg3 passed!");
		++rv;
	}

	if(rv == 0){
		gl_output("test_mnmsg_send(): passed");
	} else {
		gl_error("test_mnmsg_send(): failed with %i test errors", rv);
	}

	free(mnmsg3);
	free(mnmsg2);
	free(mnmsg1);
	free(obj_local_cnif);
	free(obj_local_mnif);
	
	return (rv == 0 ? 1 : 0);
}

int test_mnmsg_serialize(){
	mpi_network_message *mnmsg = 0;
	const char *rstr;
	uint32 sz = 0;
	OBJECT *token_obj;
	char *obj1name = "sample_obj_name";
	char *obj2name = "other_obj_name";
	char *objmsg = "price=0.07,avg=0.09,stdev=0.02";
	const int target_sz = 65;
	const char *target_msg="sample_obj_name other_obj_name 30 price=0.07,avg=0.09,stdev=0.02";

	token_obj = (OBJECT *)malloc(sizeof(OBJECT));
	memset(token_obj, 0, sizeof(OBJECT));
	token_obj->name = obj1name;

	mnmsg = (mpi_network_message *)malloc(sizeof(mpi_network_message));
	memset(mnmsg, 0, sizeof(mpi_network_message));
	mnmsg->from = token_obj;
	strcpy(mnmsg->to_name, obj2name);
	strcpy(mnmsg->message, objmsg);
	
	rstr = mnmsg->serialize(&sz);
//	gl_output("mnmsg serialized: '%s'", rstr);
//	gl_output("sz = %i", sz);
	if(target_sz != sz){
		gl_error("test_mnmsg_serialize(): sz does not match target (%i, expected %i)", sz, target_sz);
		free(mnmsg);
		free(token_obj);
		return 0;
	}
	if(0 != memcmp(target_msg, rstr, sz)){
		gl_error("test_mnmsg_serialize(): serialized message does not match target", sz, target_sz);
		gl_error("wanted: %s", target_msg);
		gl_error("found:  %s", rstr);
		free(mnmsg);
		free(token_obj);
		return 0;
	}

	gl_output("test_mnmsg_serialize(): passed");
	free(token_obj);
	free(mnmsg);

	return 1;
}

int test_mnmsg_deserialize(){
	OBJECT *to;
	const char *targ_name = "sample_obj_name";
	char *msg = "sample_obj_name other_obj_name 30 price=0.07,avg=0.09,stdev=0.02";
	char *targ_data = "price=0.07,avg=0.09,stdev=0.02";
	const size_t sz = 65;
	int rv = 0;

	//to = gl_create_object(market_network_interface::oclass);
	to = (OBJECT *)malloc(sizeof(OBJECT) + sizeof(market_network_interface));
	if(0 == to){
		gl_error("test_mnmsg_deserialize(): could not malloc object!");
		return 0;
	}
	memset(to, 0, sizeof(OBJECT) + sizeof(market_network_interface));
	if(0 == gl_set_object_name(to, "other_obj_name")){
		gl_error("test_mnmsg_deserialize(): could not set object name to 'other_obj_name'!");
		return 0;
	}

	mpi_network_message *mnmsg = (mpi_network_message *)malloc(sizeof(mpi_network_message));
	memset(mnmsg, 0, sizeof(mpi_network_message));

	rv = mnmsg->deserialize(msg, sz);

	if(rv != 1){
		gl_error("test_mnmsg_deserialize: failed to deserialize message");
		free(mnmsg);
		return 0;
	}
	if(mnmsg->to != to){
		gl_error("test_mnmsg_deserialize: did not correctly locate 'to' object!");
		free(mnmsg);
		return 0;
	}
	if(0 != strcmp(mnmsg->from_name, targ_name)){
		gl_error("test_mnmsg_deserialize: did not correctly parse source object name");
		free(mnmsg);
		return 0;
	}
	if(0 != strcmp(mnmsg->message, targ_data)){
		gl_error("test_mnmsg_deserialize: did not correctly parse test data");
		free(mnmsg);
		return 0;
	}
	gl_output("test_mnmsg_deserialize: passed");
	free(mnmsg);
	return 1;
}


// test mpi_network::attach()
int test_mnet_attach(){
	char *onif1_name = "net_if",
		*onif2_name = "market_if",
		*onif3_name = "controller_if",
		*onet_name = "mpi_net";
	OBJECT *onif1, *onif2, *onif3, *onet;
	network_interface *nif1;
	market_network_interface *nif2;
	controller_network_interface *nif3;
	mpi_network *net;
	int rv = 0;
	size_t osz = sizeof(OBJECT);
	size_t mnif_sz = sizeof(market_network_interface);

	// network_interface
	onif1 = (OBJECT *)malloc(sizeof(network_interface) + osz);
	if(0 == onif1){
		gl_error("test_mnet_attach(): malloc failure!");
		return 0;
	}
	memset(onif1, 0, osz + sizeof(network_interface));
	nif1 = OBJECTDATA(onif1, network_interface);
	nif1->create();
	onif1->name = onif1_name;

	// market_nif
//	onif2 = (OBJECT *)malloc(sizeof(market_network_interface) + osz);
	onif2 = (OBJECT *)malloc(mnif_sz + osz);
	memset(onif2, 0, osz + mnif_sz);
	nif2 = OBJECTDATA(onif2, market_network_interface);
	nif2->create();
	onif2->name = onif2_name;

	// controller_nif
	onif3 = (OBJECT *)malloc(sizeof(controller_network_interface) + osz); // breaks here
	memset(onif3, 0, sizeof(controller_network_interface) + osz);
	nif3 = OBJECTDATA(onif3, controller_network_interface);
	nif3->create();
	onif3->name = onif3_name;

	// mpi_network
	onet = (OBJECT *)malloc(sizeof(mpi_network) + osz);
	net = OBJECTDATA(onet, mpi_network);
	net->create();
	onet->name = onet_name;

	// attach network interfaces
	net->attach(nif1);
	net->attach(nif2);
	net->attach(nif3);
	
	// check that the interfaces were attached
	if(net->first_if != nif1){
		gl_error("test_mnet_attach(): stock network interface did not attach");
		free(onif1);
		free(onif2);
		free(onif3);
		free(onet);
		return rv;
	}
	if(nif2->net != net){
		gl_error("test_mnet_attach(): market network interface did not attach");
		free(onif1);
		free(onif2);
		free(onif3);
		free(onet);
		return rv;
	}
	if(net->last_if != nif3){
		gl_error("test_mnet_attach(): controller network interface did not attach");
		free(onif1);
		free(onif2);
		free(onif3);
		free(onet);
		return rv;
	}

	gl_output("test_mnet_attach(): passed");
	// cleanup
	rv = 1;
	free(onif1);
	free(onif2);
	free(onif3);
	free(onet);
	return rv;
}



int test_mnif_send_buffer(){
	gl_output("test_mnif_send_buffer not written");
	return 1; 
}


int test_mnif_check_buffer(){
	gl_output("test_mnif_send_buffer not written");
	return 1; 
}

int test_mnif_check_write_msg(){
	OBJECT *obj_market, *obj_mnif;
	market_network_interface *mnif;
	PROPERTY *market_id_prop;
	CLASS *market_class;
	char *market_name = "name_of_market_obj";
	char *market_prop_name = "market_id";
	int64 *mid_ptr;
	int rv = 0;

	// build faux market
	obj_market = (OBJECT *)malloc(sizeof(int64) + sizeof(OBJECT));
	if(obj_market == 0){
		gl_error("test_mnif_check_write_msg(): malloc failure");
		return 0;
	}
	memset(obj_market, 0, sizeof(int64) + sizeof(OBJECT));
	gl_set_object_name(obj_market, market_name);
	// build market class
	market_class = (CLASS *)malloc(sizeof(CLASS));
	if(market_class == 0){
		gl_error("test_mnif_check_write_msg(): malloc failure");
		return 0;
	}
	// build market prop
	market_id_prop = (PROPERTY *)malloc(sizeof(PROPERTY));
	memset(market_id_prop, 0, sizeof(PROPERTY));
	strcpy(market_id_prop->name, market_prop_name);
	market_id_prop->ptype = PT_int64;
	market_id_prop->addr = 0;
	market_id_prop->next = 0;
	market_class->pmap = market_id_prop;
	// build market_nif
	obj_mnif = (OBJECT *)malloc(sizeof(market_network_interface) + sizeof(OBJECT));
	memset(obj_mnif, 0, sizeof(market_network_interface) + sizeof(OBJECT));
	mnif = OBJECTDATA(obj_mnif, market_network_interface);
	obj_mnif->parent = obj_market;
	mnif->write_init = true;
	/*if(0 == mnif->init(obj_market)){
		// need to init market to set property targets
		gl_error("test_mnif_check_write_msg(): mnif init() failure");
		return 0;
	}*/

	mnif->market_id_prop = market_id_prop;
	mid_ptr = OBJECTDATA(obj_market, int64);
	

	// check initial mktid = 0 case
	*mid_ptr = 0;

	if(!mnif->check_write_msg()){
		gl_error("test_mnif_check_write_msg(): first buffer check failed");
		++rv;
	}

	// check mktid = 12 case
	*mid_ptr = 12;
	if(!mnif->check_write_msg()){
		gl_error("test_mnif_check_write_msg(): second buffer check failed");
		++rv;
	}
	// check 'still = 12' case
	if(mnif->check_write_msg()){
		gl_error("test_mnif_check_write_msg(): third buffer check failed");
		++rv;
	}
	// check = 14 case
	*mid_ptr = 14;
	if(!mnif->check_write_msg()){
		gl_error("test_mnif_check_write_msg(): fourth buffer check failed");
		++rv;
	}
	free(obj_mnif);
	free(market_id_prop);
	free(obj_market);

	if(rv == 0){
		gl_output("test_mnif_check_write_msg() passed");
	} else {
		gl_error("test_mnif_check_write_msg() failed with %i errors", rv);
	}
	return (rv == 0 ? 1 : 0);
}

int test_mnif_send_update(){
	gl_output("test_mnif_send_update not written");
	return 1; 
}

int test_mnif_handle_inbox(){
	gl_output("test_mnif_handle_inbox not written");
	return 1; 
}

int test_mnif_process_bid(){
	gl_output("test_mnif_process_bid not written");
	return 1; 
}


int test_mnif_send_rsp(){
	gl_output("test_mnif_send_rsp not written");
	return 1; 
}


int test_cnif_check_buffer(){ return 1; }
int test_cnif_handle_inbox(){ return 1; }
int test_cnif_send_bid(){ return 1; }
int test_cnif_recv_response(){ return 1; }
int test_cnif_recv_update(){ return 1; }
int test_cnif_recv_init(){ return 1; }

// mpi_network tests

// test that messages in mpi_network-> ...
/***  MPI_NETWORK::COMMIT DOES NOT HANDLE MESSAGE DELIVERY YET  ***/
int test_mnet_deliver(){
	char *onif1_name = "net_if",
		*onif2_name = "market_if",
		*onif3_name = "controller_if",
		*onet_name = "mpi_net";
	OBJECT *onif1, *onif2, *onif3, *onet;
	network_interface *nif1;
	market_network_interface *nif2;
	controller_network_interface *nif3;
	mpi_network *net;
	int rv = 0;
	size_t osz = sizeof(OBJECT);

	gl_warning("test_mnet_deliver not yet complete");

	// network_interface
	onif1 = (OBJECT *)malloc(sizeof(network_interface) + osz);
	if(0 == onif1){
		gl_error("test_mnet_deliver: malloc failure!");
		return 0;
	}
	nif1 = OBJECTDATA(onif1, network_interface);
	nif1->create();
	onif1->name = onif1_name;

	// market_nif
	onif2 = (OBJECT *)malloc(sizeof(market_network_interface) + osz);
	nif2 = OBJECTDATA(onif2, market_network_interface);
	nif2->create();
	onif2->name = onif2_name;

	// controller_nif
	onif3 = (OBJECT *)malloc(sizeof(controller_network_interface) + osz);
	nif3 = OBJECTDATA(onif3, controller_network_interface);
	nif3->create();
	onif3->name = onif3_name;

	// mpi_network
	onet = (OBJECT *)malloc(sizeof(mpi_network) + osz);
	net = OBJECTDATA(onet, mpi_network);
	net->create();
	onet->name = onet_name;

	// attach network interfaces
	net->attach(nif1);
	net->attach(nif2);
	net->attach(nif3);
	
	/* TODO: write messages, run 'deliver messages' method in Network */
	//gl_output("test_mnet_deliver(): passed");

	// cleanup
	rv = 1;
	free(onif1);
	free(onif2);
	free(onif3);
	free(onet);
	return rv;
}

int test_mnet_retreive(){ return 1; }
// EOF
