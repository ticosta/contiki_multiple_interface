/**
 * @file     er-http.h
 * @brief    Contains the Definitions of the HTTP Rest Implemention
 * @version  1.0
 * @date     20 May. 2017
 * @author   Tiago Costa & Ricardo Jesus & Claudio Prates
 *
 **/

/** @defgroup HTTP HTTP
 * @{
 */

/** @addtogroup Rest_Implementation
 * @{
 */

#ifndef ER_HTTP_H_
#define ER_HTTP_H_
/*---------------------------------------------------------------------------*/
#include "contiki-net.h"
#include "sys/process.h"
#include "rest-engine.h"
/*---------------------------------------------------------------------------*/
/* Ideally a multiple of TCP_MSS */
#ifdef HTTPD_SIMPLE_CONF_MAIN_BUF_SIZE
#define HTTPD_SIMPLE_MAIN_BUF_SIZE HTTPD_SIMPLE_CONF_MAIN_BUF_SIZE
#else
#define HTTPD_SIMPLE_MAIN_BUF_SIZE UIP_TCP_MSS
#endif
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN]) /*!< Pointer to UIP Buffer */
#if NETSTACK_CONF_WITH_IPV6
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len]) /*!< Pointer to UIP UDP Buffer */
#else
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN]) /*!< Pointer to UIP UDP Buffer */
#endif
/*---------------------------------------------------------------------------*/
#define BUFFER_LEN  100 /*!< Size of the HTTP Buffer */
#define HTTPD_INBUF_LEN (BUFFER_LEN + 10) /*!< Max of HTTP Buffer */

#define TMP_BUF_SIZE   (UIP_TCP_MSS + 1) /*!< Max of Chunk Transfer buffer */
/*---------------------------------------------------------------------------*/
#define REQUEST_TYPE_GET  1 /*!< HTTP Method GET */
#define REQUEST_TYPE_POST 2 /*!< HTTP Method POST */
#define REQUEST_TYPE_PUT 3 /*!< HTTP Method PUT */
#define REQUEST_TYPE_DELETE 4 /*!< HTTP Method DELETE */
/*---------------------------------------------------------------------------*/
//
#define HTTP_ACCEPT_HEADER_TEXT_PLAIN			1 << 1 /*!< Bit for Header Accept - TEXT-PLAIN */
#define HTTP_ACCEPT_HEADER_APPLICATION_JSON 	1 << 2 /*!< Bit for Header Accept - APPLICATION_JSON */
#define HTTP_ACCEPT_HEADER_APPLICATION_XML	 	1 << 3 /*!< Bit for Header Accept - APPLICATION_XML */
#define MAX_URI_LEN	                            50 /*!< The MAX URI  */

#define MAX_PAYLOAD_SIZE                        100 /*!< Payload Size */

#define RETURN_CODE_OK   0 /*!< Status OK */
#define RETURN_CODE_NF   1 /*!< Not Found */
#define RETURN_CODE_SU   2 /*!< Service Unavailable */
#define RETURN_CODE_BR   3 /*!< Bad Request */
#define RETURN_CODE_LR   4 /*!< Length Required */
#define RETURN_CODE_TL   5 /*!< Content Length too Large */

/*---------------------------------------------------------------------------*/
typedef struct http_response_t {
	char buf[HTTPD_SIMPLE_MAIN_BUF_SIZE]; /*!< Buffer */
	int blen; /*!< Size of Buffer */
	const char * content_type; /*!< HTTP content type of response */
	uint16_t content_length; /*!< Content length of response */
	const char * status; /*!< Status of response */
	const char **additional_hdrs; /*!< Optional headers of response */
	const char * redir_path; /*!< Location for redirect */
	char immediate_response; /*!< Bit for if response should be immediate */
} http_response;
/*---------------------------------------------------------------------------*/
typedef struct coap_request_params_t {
	  uint8_t *dst_ipaddr; /*!< ip address of dest node */
	  char *action; /*!< Used to pass the URI for CoAP client, Here URI means the action parameter in a POST request to the CoAP nodes */
	  uint16_t action_len; /*!< Size of Action without null terminator */
	  char *params; /*!< The Params of the Actions */
	  uint16_t params_len; /*!< Size of params without null terminator */
} coap_request_params;
/*---------------------------------------------------------------------------*/
/* http request struct */
typedef struct  {
  uint8_t buf[HTTPD_SIMPLE_MAIN_BUF_SIZE]; /*!< Buffer associated with chunk */
  char tmp_buf[TMP_BUF_SIZE]; /*!< Temp Buff used to transfer chunks */
  struct timer timer; /*!< Timer for the event */
  struct psock sin, sout; /*!< Sockets in and out */

  const char **ptr; /*!< Auxiliary pointer for several operations */
  int content_length; /*!< Content-Length */

  size_t complete_uri_len; /*!< Complete_uri length >*/
  size_t uri_len; /*!< Size of Complete_uri */
  char *uri; /*!< Pointer to 'uri' */
  char complete_uri[MAX_URI_LEN]; /*!< complete_uri with query */
  const char* uri_query; /*!< Pointer to uri-query */
  size_t uri_query_len; /*!< Size of uri_query */
  int blen; /*!< Size of buffer */
  int tmp_buf_len; /*!< Size of tmp_buf */
  int tmp_buf_copied; /*!< State of the chunk, how much as already copied */

  char inputbuf[HTTPD_INBUF_LEN]; /*!< Main Buffer associated with the Socket */

  struct pt outputpt; /*!< Protothread, utilizada para gerar o output */
  struct pt generate_pt; /*!< Protothread, utilizada para gerar os headers */

  short state; /*!< Internal State, to control error */
  short request_type; /*!< HTTP Method */
  short return_code; /*!< Internal Status Code */

  http_response response; /*!< The Response */
  char buffer[MAX_PAYLOAD_SIZE]; /*!< Payload Buffer */
  //
  coap_request_params coap_req; /*!< CoAP Request Params */
}httpd_state, http_packet_t ;
/*---------------------------------------------------------------------------*/

/** Register the RESTful service callback at implementation. */
void http_set_service_callback(service_callback_t callback);

/** Set additional headers that will be interpreted by httpd. */
int http_set_additional_headers(void *request, const void **headers);

/** Used by end-points to set redir path. Interpreted by httpd. */
int http_set_redir_path(void *request, const void *path);

/** build the additional Content-Length header. */
void * build_header_content_length(void *request, int len);

/** Prepare an HTTP response with the specified error and status. */
void set_http_error(void *request, char *err, unsigned int status);

/*---------------------------------------------------------------------------*/
/*
 * An event generated by the HTTPD when a new configuration request has been
 * received
 */
extern process_event_t httpd_simple_event_new_config;
/*---------------------------------------------------------------------------*/
PROCESS_NAME(httpd_process);
/*---------------------------------------------------------------------------*/

/**
 * @}
 */

/**
 * @}
 */

#endif /* ER_HTTP_H_ */
