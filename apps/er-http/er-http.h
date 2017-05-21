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

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    1280
/*---------------------------------------------------------------------------*/
#define HTTPD_PATHLEN  100
#define HTTPD_INBUF_LEN (HTTPD_PATHLEN + 10)

#define TMP_BUF_SIZE   (UIP_TCP_MSS + 1)
/*---------------------------------------------------------------------------*/
/* POST request handlers */
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
#define PARSE_POST_BUF_SIZES                    64
#define MAX_PAYLOAD_SIZE                        100

/* Last byte always used to null terminate */
#define PARSE_POST_MAX_POS        (PARSE_POST_BUF_SIZES - 2)

#define RETURN_CODE_OK   0
#define RETURN_CODE_NF   1 /* Not Found */
#define RETURN_CODE_SU   2 /* Service Unavailable */
#define RETURN_CODE_BR   3 /* Bad Request */
#define RETURN_CODE_LR   4 /* Length Required */
#define RETURN_CODE_TL   5 /* Content Length too Large */

/**
 * \brief Datatype for a handler which can process incoming POST requests
 * \param key The configuration key to be updated
 * \param key_len The length of the key argument
 * \param val The new configuration value for key
 * \param val_len The length of the value argument
 *
 * \return 1: HTTPD_SIMPLE_POST_HANDLER_OK if the function can handle the
 * request, HTTPD_SIMPLE_POST_HANDLER_UNKNOWN if it does not know how to handle
 * it. HTTPD_SIMPLE_POST_HANDLER_ERROR if it does know how to handle it but
 * the request was malformed.
 */
#define HTTP_200_OK "HTTP/1.0 200 OK\r\n"
#define HTTP_302_FO "HTTP/1.0 302 Found\r\n"
#define HTTP_400_BR "HTTP/1.0 400 Bad Request\r\n"
#define HTTP_404_NF "HTTP/1.0 404 Not Found\r\n"
#define HTTP_411_LR "HTTP/1.0 411 Length Required\r\n"
#define HTTP_413_TL "HTTP/1.0 413 Request Entity Too Large\r\n"
#define HTTP_503_SU "HTTP/1.0 503 Service Unavailable\r\n"
#define CONN_CLOSE  "Connection: close\r\n"
/*---------------------------------------------------------------------------*/
#define CONTENT_OPEN  "<pre>"
#define CONTENT_CLOSE "</pre>"
/*---------------------------------------------------------------------------*/
/* Page template */
 static const char http_header_200[] = HTTP_200_OK;
 static const char http_header_302[] = HTTP_302_FO;
 static const char http_header_400[] = HTTP_400_BR;
 static const char http_header_404[] = HTTP_404_NF;
 static const char http_header_411[] = HTTP_411_LR;
 static const char http_header_413[] = HTTP_413_TL;
 static const char http_header_503[] = HTTP_503_SU;

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
	char content_type[50];
	uint16_t content_length;
	unsigned int status;
	char * status_str;
	char imediate_response;
} http_response;

/*---------------------------------------------------------------------------*/
/* http request struct */
typedef struct  {
  uint8_t buf[HTTPD_SIMPLE_MAIN_BUF_SIZE];
  char tmp_buf[TMP_BUF_SIZE];
  struct timer timer;
  struct psock sin, sout;

  const char **ptr;
  int content_length;

  size_t complete_uri_len; 						/*!< uri-length >*/
  size_t uri_len;
  char *uri; 							/* uri-name */
  char complete_uri[MAX_URI_LEN]; 		/* uri-name */
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
  // ip address of dest node
  char *dst_ipaddr;
  char *action;
  uint16_t action_len;
  struct uip_conn * currentConnection;
}httpd_state, http_packet_t ;
/*---------------------------------------------------------------------------*/

/**
 * \brief Register a handler for POST requests
 * \param h A pointer to the handler structure
 */
void httpd_simple_register_post_handler(httpd_simple_post_handler_t *h);

/** Register the RESTful service callback at implementation. */
void http_set_service_callback(service_callback_t callback);

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
