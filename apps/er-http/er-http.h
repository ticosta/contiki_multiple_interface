/*
 * er-http.h
 *
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
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#if NETSTACK_CONF_WITH_IPV6
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#else
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#endif
/*---------------------------------------------------------------------------*/
#define HTTPD_PATHLEN  100
#define HTTPD_INBUF_LEN (HTTPD_PATHLEN + 10)

#define TMP_BUF_SIZE   (UIP_TCP_MSS + 1)
/*---------------------------------------------------------------------------*/
/* POST request handlers */
// TODO: pagar?
#define HTTPD_SIMPLE_POST_HANDLER_OK      1
#define HTTPD_SIMPLE_POST_HANDLER_UNKNOWN 0
#define HTTPD_SIMPLE_POST_HANDLER_ERROR   0xFFFFFFFF
/*---------------------------------------------------------------------------*/
#define REQUEST_TYPE_GET  1
#define REQUEST_TYPE_POST 2
/*---------------------------------------------------------------------------*/
//
#define HTTP_ACCEPT_HEADER_TEXT_PLAIN			1 << 1
#define HTTP_ACCEPT_HEADER_APPLICATION_JSON 	1 << 2
#define HTTP_ACCEPT_HEADER_APPLICATION_XML	 	1 << 3
#define MAX_URI_LEN	                            50
//#define PARSE_POST_BUF_SIZES                    64
#define MAX_PAYLOAD_SIZE                        100

/* Last byte always used to null terminate */
//#define PARSE_POST_MAX_POS        (PARSE_POST_BUF_SIZES - 2)

#define RETURN_CODE_OK   0
#define RETURN_CODE_NF   1 /* Not Found */
#define RETURN_CODE_SU   2 /* Service Unavailable */
#define RETURN_CODE_BR   3 /* Bad Request */
#define RETURN_CODE_LR   4 /* Length Required */
#define RETURN_CODE_TL   5 /* Content Length too Large */

 // TODO: apagar?
typedef struct httpd_simple_post_handler {
  struct httpd_simple_post_handler *next;
  int (*handler)(char *key, int key_len, char *val, int val_len);
} httpd_simple_post_handler_t;

/* Declare a handler */
#define HTTPD_SIMPLE_POST_HANDLER(name, fp) \
  httpd_simple_post_handler_t name##_handler = { NULL, fp }

/*---------------------------------------------------------------------------*/
typedef struct http_response_t {
	char buf[HTTPD_SIMPLE_MAIN_BUF_SIZE];
	int blen;
	const char * content_type;
	uint16_t content_length;
	const char * status;
	const char **additional_hdrs;
	const char * redir_path;
	char immediate_response;
} http_response;
/*---------------------------------------------------------------------------*/
typedef struct coap_request_params_t {
	// ip address of dest node
	  uint8_t *dst_ipaddr;
	  // Used to pass the URI for CoAP client
	  // Here URI means the action parameter in a POST request to the CoAP nodes
	  char *action;
	  // without null terminator
	  uint16_t action_len;
	  //
	  char *params;
	  // without null terminator
	  uint16_t params_len;
} coap_request_params;
/*---------------------------------------------------------------------------*/
/* http request struct */
typedef struct  {
  uint8_t buf[HTTPD_SIMPLE_MAIN_BUF_SIZE];
  char tmp_buf[TMP_BUF_SIZE];
  struct timer timer;
  struct psock sin, sout;

  const char **ptr;
  int content_length;

  size_t complete_uri_len; 				/*!< complete_uri length >*/
  size_t uri_len;
  char *uri; 							/* pointer to 'complete_uri' */
  char complete_uri[MAX_URI_LEN]; 		/* complete_uri- with query */
  const char* uri_query;
  size_t uri_query_len;
  int blen;
  int tmp_buf_len;
  int tmp_buf_copied;

  char inputbuf[HTTPD_INBUF_LEN];

  struct pt outputpt; //usada para gerar o output
  struct pt generate_pt; //usada para fazer headers

  short state;
  short request_type;
  short return_code;
  // response
  http_response response;
  char buffer[MAX_PAYLOAD_SIZE];
  //
  coap_request_params coap_req;
}httpd_state, http_packet_t ;
/*---------------------------------------------------------------------------*/

/**
 * \brief Register a handler for POST requests
 * \param h A pointer to the handler structure
 */
void httpd_simple_register_post_handler(httpd_simple_post_handler_t *h);

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

#endif /* ER_HTTP_H_ */