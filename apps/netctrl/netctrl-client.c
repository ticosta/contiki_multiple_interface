/**
 * \addtogroup netctrl
 * @{
 */

#include <limits.h>

#include "netctrl-client.h"


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

/** Machine state. */
static int state = NETCTRL_CLIENT_UNREGISTERED;
/** Incremental Id for requests. */
static uint16_t request_id = NETCTRL_CLIENT_INITIAL_REQ_ID;
/** Control the number of renew retries. */
static uint8_t renew_retries = 0;
/** */
static uint8_t periodicity;
/*---------------------------------------------------------------------------*/
static netctrl_time_t netctrl_registration_stage(int event, netctrl_req_header_t * req) {
	PRINTF("On registration stage.\n");
	switch(event) {
	case NETCTRL_CLIENT_EVT_TIMER:
		PRINTF("  Sending reg. request.\n");
		req->id = htons(request_id);
		req->type = NETCTRL_REQUEST_TYPE_REG;
		req->eq_type = NODE_EQUIPEMENT_TYPE;
		req->data = htonl(netctrl_node_data);
		netctrl_send_message((uint8_t *)req, NETCTRL_REQ_HEADER_SIZE);
		//
		state = NETCTRL_CLIENT_WAIT_CONFIRM;
		return REG_TIMEOUT;
	default:
		PRINTF("  Discarding.\n");
		// discard and retry later
		return REG_RETRY_TIME;
	}
}
/*---------------------------------------------------------------------------*/
static netctrl_time_t netctrl_wait_fort_rsp_stage(int event, netctrl_rsp_header_t * rsp) {
	PRINTF("On Wait stage.\n");
	switch(event) {
		case NETCTRL_CLIENT_EVT_NET:
			if(request_id == ntohs(rsp->id)) {
				if(rsp->result == NETCTRL_RESPONSE_RESULT_REG_OK) {
					PRINTF("  ** Registered!\n");
					// Reg ok. Set periodicity
					periodicity = rsp->periodicity;
					request_id = netctrl_next_id(request_id);
					state = NETCTRL_CLIENT_REGISTERED;
					return periodicity;
				} else if (rsp->result == NETCTRL_RESPONSE_RESULT_REG_FAILED) {
					PRINTF("  !!! Registration Failed!\n");
					// Something goes wrong. Retry later
					state = NETCTRL_CLIENT_UNREGISTERED;
					return REG_RETRY_TIME;
				}
				PRINTF("  !!! Unknown Result!\n");
				// Unknown Result
				state = NETCTRL_CLIENT_UNREGISTERED;
				return REG_TIMEOUT;
			} else {
				PRINTF("  Discarding.\n");
				// Wrong Id. Discard and retry later
				return REG_RETRY_TIME;
			}
			break;
		case NETCTRL_CLIENT_EVT_TIMER:
			PRINTF("  Wait Timeout.\n");
			// Wait timeout. Retry later
			state = NETCTRL_CLIENT_UNREGISTERED;
			return REG_RETRY_TIME;
	}

	PRINTF("  !!! Unknown event!\n");
	// Unknown event
	state = NETCTRL_CLIENT_UNREGISTERED;
	return REG_TIMEOUT;
}
/*---------------------------------------------------------------------------*/
static netctrl_time_t netctrl_registered_stage(int event, netctrl_rsp_header_t * rsp,
		netctrl_req_header_t * req) {
	PRINTF("On Registered stage.\n");
	switch(event) {
	case NETCTRL_CLIENT_EVT_NET:
		if(request_id == ntohs(rsp->id)) {
			if(rsp->result == NETCTRL_RESPONSE_RESULT_RENEW_OK) {
				PRINTF("  ** Renew Ok!\n");
				// Renew ok. Reset periodicity
				periodicity = rsp->periodicity;
				renew_retries = 0;
				request_id = netctrl_next_id(request_id);
				return periodicity;
			} else if (rsp->result == NETCTRL_RESPONSE_RESULT_RENEW_FAILED) {
				PRINTF("  !!! Renew Failed!\n");
				// Something goes wrong. Retry later
				return REG_RETRY_TIME;
			}
		} else {
			PRINTF("  Discarding.\n");
			// Wrong Id. Discard and retry later
			return REG_RETRY_TIME;
		}
		break;
	case NETCTRL_CLIENT_EVT_TIMER:
		if(renew_retries >= MAX_RENEW_RETRIES) {
			PRINTF("  Max retreis reached!\n");
			state = NETCTRL_CLIENT_UNREGISTERED;
			renew_retries = 0;
			return REG_RETRY_TIME;
		} else {
			PRINTF("  Sending renew request.\n");
			renew_retries++;
			req->id = htons(request_id);
			req->type = NETCTRL_REQUEST_TYPE_RENEW;
			req->eq_type = NODE_EQUIPEMENT_TYPE;
			req->data = htonl(netctrl_node_data);
			//
			netctrl_send_message((uint8_t *)req, NETCTRL_REQ_HEADER_SIZE);
			return periodicity;
		}
	}

	PRINTF("  !!! Unknown event!\n");
	// Unknown event
	state = NETCTRL_CLIENT_UNREGISTERED;
	return REG_TIMEOUT;
}
/*---------------------------------------------------------------------------*/
netctrl_time_t netctrl_client_handle_event(int event) {
	// FIXME Currently the implementation don't use the configurations feature, thus,
	//       don't need to worrie abount the response size.
	//       In the future, need to check the MSS configured in the system and use a local buffer
	//       or limit the configuration size to de MSS.
	netctrl_rsp_header_t * hdr = (netctrl_rsp_header_t *)netctrl_data_buffer;

#if DEBUG
	if(event == NETCTRL_CLIENT_EVT_NET) {
		PRINTF("Data received: %d bytes\n", uip_len);
		PRINTF("id: %02x\n", ntohs(hdr->id));
		PRINTF("result: %02x\n", hdr->result);
		PRINTF("periodicity: %02x\n", hdr->periodicity);
		PRINTF("conf_len: %02x\n", hdr->conf_len);
		PRINTF("Data: ");

		int i;
		char * c = (char *)((&hdr->conf_len) + 2);
		for(i = hdr->conf_len; i > 0; i--) {
			PRINTF("%c", c[0]);
			c++;
		}
		PRINTF("\n");
	}
#endif /* DEBUG */

	switch(state) {
	case NETCTRL_CLIENT_UNREGISTERED:
		return netctrl_registration_stage(event, (netctrl_req_header_t *)hdr);
	case NETCTRL_CLIENT_WAIT_CONFIRM:
		return netctrl_wait_fort_rsp_stage(event, hdr);
	case NETCTRL_CLIENT_REGISTERED:
		return netctrl_registered_stage(event, hdr, (netctrl_req_header_t *)hdr);
	}

	PRINTF("  !!! Something nasty occurred! Unknow state!na\n");
	return (netctrl_time_t)UINT_MAX;
}
/**
 * @}
 */
