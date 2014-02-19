
#ifndef _MPI_COMM_TEST_H_
#define _MPI_COMM_TEST_H_

#include "mpi_network.h"
#include "controller_network_interface.h"
#include "market_network_interface.h"
#include "mpi_network_message.h"

int test_mnet_deliver();
int test_mnet_retreive();
int test_mnet_attach();

int test_mnif_check_write_msg();
int test_mnif_check_buffer();
int test_mnif_send_update();
int test_mnif_process_bid();
int test_mnif_handle_inbox();
int test_mnif_send_buffer();
int test_mnif_send_rsp();

int test_cnif_check_buffer();
int test_cnif_handle_inbox();
int test_cnif_send_bid();
int test_cnif_recv_response();
int test_cnif_recv_update();
int test_cnif_recv_init();

int test_mnmsg_send();
int test_mnmsg_serialize();
int test_mnmsg_deserialize();

#endif

// EOF
