/**
 * \addtogroup netctrl
 * @{
 */

#include "netctrl-server.h"
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
	static node_table_entry_t * node;
	static netctrl_rsp_header_t *rsp;
	static uint16_t reqId;
	static int ret;
	rsp = (netctrl_rsp_header_t *)netctrl_sdata_buffer;
	reqId = req->id;
	normalize_req(req);

	if((node = node_table_get_node(
			(uip_ip6addr_t *)netctrl_get_nodeId())) != NULL) {
		PRINTF("  Node exists. Updating...\n");
		// Updates the node entry
		node_table_update_node(node, req->id, req->data);
		rsp->result = NETCTRL_RESPONSE_RESULT_REG_OK;
		ret = NETCTRL_RESPONSE_RESULT_REG_OK;
	} else {
		PRINTF("  Node doesnt exists.\n");
		node_table_entry_t * new_node = node_table_add_node((uip_ip6addr_t *)netctrl_get_nodeId(),
				netctrl_calc_node_hash(), req->id, req->data, req->eq_type);
		if(new_node != NULL) {
			PRINTF("  ** New Node added with hash %lu\n", (unsigned long)new_node->hash);
			rsp->result = NETCTRL_RESPONSE_RESULT_REG_OK;
			ret = NETCTRL_RESPONSE_RESULT_REG_OK;
		} else {
			PRINTF("  !!! Failed adding the new Node.\n");
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

	netctrl_send_messageto(uip_appdata, NETCTRL_RSP_HEADER_SIZE + rsp->conf_len);
	return ret;
}
/*---------------------------------------------------------------------------*/
static int handle_renew_request(netctrl_req_header_t *req) {
	static node_table_entry_t * node;
	static netctrl_rsp_header_t *rsp;
	uint16_t reqId;
	rsp = (netctrl_rsp_header_t *)netctrl_sdata_buffer;
	reqId = req->id;
	normalize_req(req);

	/* renew for an existing node? */
	if((node = node_table_get_node(
			(uip_ip6addr_t *)netctrl_get_nodeId())) == NULL) {
		PRINTF("  Renew request for an unknown Node. Dropping.\n");
		// Drop it!
		return NETCTRL_RESPONSE_RESULT_RENEW_FAILED;
	}
	PRINTF("  Node exists.\n");
	/* valid request id? */
	if((req->id != netctrl_next_id(node->entry_version)) &&
			(req->id != node->entry_version)) {
		PRINTF("  Invalid request Id. Dropping.\n");
		// Drop it!
		return NETCTRL_RESPONSE_RESULT_RENEW_FAILED;
	}

	PRINTF("  Updating Node and sending response.\n");
	node_table_update_node(node, req->id, req->data);
	rsp->result = NETCTRL_RESPONSE_RESULT_RENEW_OK;
	rsp->id = reqId;
	rsp->periodicity = netctrl_periodicity;
	rsp->conf_len = 0;

	netctrl_send_messageto(uip_appdata, NETCTRL_RSP_HEADER_SIZE);
	return NETCTRL_RESPONSE_RESULT_RENEW_OK;
}
/*---------------------------------------------------------------------------*/
void netctrl_server_init() {
	netctrl_server_init_network();
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
void netctrl_set_periodicity(uint8_t periodicity) {
	if(periodicity == 0)
		return;
	netctrl_periodicity = periodicity;
}

/**
 * @}
 */

