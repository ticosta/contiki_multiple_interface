#include "netctrl.h"
#include "node-table.h"
#include "netctrl-platform.h"

#include <stdint.h>


#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define netctrl_next_id(current_id)  ((current_id) + 1)

static int initialized = 0;
uint8_t netctrl_periodicity = NETCTRL_DEFAULT_PERIODICITY;

/*---------------------------------------------------------------------------*/
static int netctrl_set_rsp_conf(char *conf_ptr) {
	// TODO: For future work
	return 0;
}
/*---------------------------------------------------------------------------*/
static void normalize_req(netctrl_req_header_t *req) {
	req->id = ntohs(req->id);
	req->data = ntohl(req->data);
}
/*---------------------------------------------------------------------------*/
static int handle_reg_request(netctrl_req_header_t *req) {
	static netctrl_node_t * node;
	static netctrl_rsp_header_t *rsp;
	static uint16_t reqId;
	static int ret;
	rsp = (netctrl_rsp_header_t *)netctrl_sdata_buffer;
	reqId = req->id;
	normalize_req(req);

	if((node = node_table_get_node(
			(uip_ip6addr_t *)netctrl_get_nodeId())) != NULL) {
		// Updates the node entry
		node_table_update_node(node, req->id, req->data);
		rsp->result = NETCTRL_RESPONSE_RESULT_REG_OK;
		ret = NETCTRL_RESPONSE_RESULT_REG_OK;
	} else {
		int ret = node_table_add_node((uip_ip6addr_t *)netctrl_get_nodeId(),
				netctrl_calc_node_hash(), req->id, req->data, req->eq_type);
		if(ret == 0) {
			rsp->result = NETCTRL_RESPONSE_RESULT_REG_OK;
			ret = NETCTRL_RESPONSE_RESULT_REG_OK;
		} else {
			rsp->result = NETCTRL_RESPONSE_RESULT_REG_FAILED;
			ret = NETCTRL_RESPONSE_RESULT_REG_FAILED;
		}
	}

	rsp->id = reqId;
	rsp->periodicity = netctrl_periodicity;
	/* If reg has failed, dont send configurations */
	rsp->conf_len = (rsp->id == NETCTRL_RESPONSE_RESULT_REG_OK) ?
			netctrl_set_rsp_conf((char *)netctrl_rsp_conf(rsp)) :
			0;

	netctrl_send_message(uip_appdata, sizeof(netctrl_rsp_header_t) + rsp->conf_len);
	return ret;
}
/*---------------------------------------------------------------------------*/
static int handle_renew_request(netctrl_req_header_t *req) {
	static netctrl_node_t * node;
	static netctrl_rsp_header_t *rsp;
	uint16_t reqId;
	rsp = (netctrl_rsp_header_t *)netctrl_sdata_buffer;
	reqId = req->id;
	normalize_req(req);

	/* renew for an existing node? */
	if((node = node_table_get_node(
			(uip_ip6addr_t *)netctrl_get_nodeId())) == NULL) {
		// Drop it!
		return NETCTRL_RESPONSE_RESULT_RENEW_FAILED;
	}
	/* valid request id? */
	if((req->id != netctrl_next_id(node->entry_version)) &&
			(req->id != node->entry_version)) {
		// Drop it!
		return NETCTRL_RESPONSE_RESULT_RENEW_FAILED;
	}

	node_table_update_node(node, req->id, req->data);
	rsp->result = NETCTRL_RESPONSE_RESULT_RENEW_OK;
	rsp->id = reqId;
	rsp->periodicity = netctrl_periodicity;
	rsp->conf_len = 0;

	netctrl_send_message(uip_appdata, sizeof(netctrl_rsp_header_t));
	return NETCTRL_RESPONSE_RESULT_RENEW_OK;
}
/*---------------------------------------------------------------------------*/
void netctrl_init() {
	netctrl_init_network();
  //
  node_table_init(NETCTRL_IDLETIME_MAX_VALUE);
  initialized = 1;
}
/*---------------------------------------------------------------------------*/
int netctrl_server_handle_net_event() {
	if(!initialized) {
		PRINTF("!!! netctrl_init MUST be called before calling other functions!\n");
		return NETCTRL_NOT_INITIALIZED;
	}
	netctrl_req_header_t * hdr = (netctrl_req_header_t *)netctrl_data_buffer;

	PRINTF("Data received: %d\n", uip_len);
	PRINTF("id: %02x\n", ntohs(hdr->id));
	PRINTF("type: %02x\n", hdr->type);
	PRINTF("eq_type: %02x\n", hdr->eq_type);
	PRINTF("Data: \n");
#if DEBUG
	int i;
	char * c = (char *)&(hdr->data);
	for(i = sizeof(hdr->data); i > 0; i--) {
		PRINTF("%c", c[0]);
		c++;
	}
	PRINTF("\n");
#endif /* DEBUG */


	switch(hdr->type) {
	case NETCTRL_REQUEST_TYPE_REG:
		PRINTF("Handling a Registration request.\n");
		return handle_reg_request(hdr);
	case NETCTRL_REQUEST_TYPE_RENEW:
		PRINTF("Handling a Renew request.\n");
		return handle_renew_request(hdr);
	}

	return NETCTRL_INVALID_REQUEST;
}
/*---------------------------------------------------------------------------*/
void netctrl_client_handle_net_event() {
	// FIXME Currently the implementation don't use the configurations feature, thus,
	//       don't need to worrie abount the response size.
	//       In the future, need to check the MSS configured in the system and use a local buffer
	//       or limit the configuration size to de MSS.

	// TODO
}
/*---------------------------------------------------------------------------*/
void netctrl_set_periodicity(uint8_t periodicity) {
	if(periodicity == 0)
		return;
	netctrl_periodicity = periodicity;
}

