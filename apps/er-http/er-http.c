/**
 * @file     er-http.c
 * @brief    Rest HTTP Implementation
 * @version  1.0
 * @date     01 Jun. 2017
 * @author   Tiago Costa & Ricardo Jesus & Claudio Prates
 *
 **/

/** @defgroup HTTP HTTP
 * @{
 */

/** @addtogroup Rest_Implementation
 * @{
 */

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"

#include "httpd.h"
#include "er-http.h"
#include "er-coap.h"   // TODO: Esta aqui só para as constantes e enums no rest interface

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include "net/ip/uip-debug.h"
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/*---------------------------------------------------------------------------*/
/* Used to temporarily stores content-length as ASCII.
 * Assumes that this value never exceeds 9999 Bytes PLUS null terminator. */
#define CONTENT_LENGTH_BUFF_SIZE                   5
static char content_len_buf[CONTENT_LENGTH_BUFF_SIZE];
/*---------------------------------------------------------------------------*/
static int
http_get_variable(const char *buffer, size_t length, const char *name,
                  const char **output);

/** Initialize the REST implementation. */
void http_init_engine(void) {
	process_start(&httpd_process, NULL);
}


int http_set_additional_headers(void *request, const void **headers) {
	httpd_state *const http_pkt = (httpd_state *)request;

	http_pkt->response.additional_hdrs = (const char **)headers;
	return 1;
}

// 18 -> Content-Length:%s\r\n
#define CLEN_HDR_BUF_SIZE             18 + CONTENT_LENGTH_BUFF_SIZE
static char clength_hdr_buff[CLEN_HDR_BUF_SIZE];
void * build_header_content_length(void *request, int len) {
	itoa(len, content_len_buf, 10);
	snprintf(clength_hdr_buff, CLEN_HDR_BUF_SIZE, http_header_content_legth, content_len_buf);

	return clength_hdr_buff;
}
/* -------------------------- Error status function ----------------------- */
const char **add_hdrs[] = {NULL, NULL};
void set_http_error(void *request, char *err, unsigned int status) {
	((httpd_state *)request)->response.immediate_response = 1;
	//
	int len = snprintf(error_buffer, ERROR_BUFFER_SIZE, error_template, err);
	REST.set_response_payload(request, error_buffer, len);
	REST.set_header_content_type(request, REST.type.APPLICATION_JSON);
	REST.set_response_status(request, status);
	// Additional header Content-Length
	add_hdrs[0] = build_header_content_length(request, len);
	http_set_additional_headers(request, (const void **)add_hdrs);
}
/*---------------------------------------------------------------------------*/




int http_set_redir_path(void *request, const void *path) {
	httpd_state *const http_pkt = (httpd_state *)request;

	http_pkt->response.redir_path = (char *)path;
	return 1;
}


/** Get request URI path. */
int http_get_header_uri_path(void *request, const char **path) {
	httpd_state *const http_pkt = (httpd_state *)request;
	*path = http_pkt->uri;

	return http_pkt->uri_len;
}

/** Get the method of a request. */
rest_resource_flags_t http_get_rest_method(void *request) {
	httpd_state *const http_pkt = (httpd_state *)request;

	// TODO: isto não é verdade
	return (rest_resource_flags_t)(1 <<
	                                 (http_pkt->request_type - 1));
}

/** Set the status code of a response.
 * It maps HTTP and CoAP codes to an HTTP status.
 * The mapping for CoAP only map status codes with just one map code to HTTP.
 *
 * Based on https://tools.ietf.org/html/draft-ietf-core-http-mapping-17#section-7 */
int http_set_status_code(void *response, unsigned int code) {
    httpd_state *const http_pkt = (httpd_state *)response;
    const char * hdr_ptr;

    switch(code) {
	/* ------  2xx  ------ */
    case OK_200:
    case CONTENT_2_05:
    	hdr_ptr = http_header_200;
    	break;

	/* ------  3xx  ------ */
    /* Found - NOTE: Has no direct mapping to CoAP status codes */
    case FOUND_302:
    	hdr_ptr = http_header_302;
    	break;

	/* ------  4xx  ------ */
    case BAD_REQUEST_400:
    case BAD_REQUEST_4_00:
    	hdr_ptr = http_header_400;
    	break;
    case FORBIDDEN_403:
    case FORBIDDEN_4_03:
    	hdr_ptr = http_header_403;
    	break;
    case NOT_FOUND_404:
    case NOT_FOUND_4_04:
    	hdr_ptr = http_header_404;
    	break;
    case METHOD_NOT_ALLOWED_405:
    	hdr_ptr = http_header_405;
    	break;
    case LENGTH_REQUIRED_411:
    	hdr_ptr = http_header_411;
    	break;
    case REQUEST_ENTITY_TL_413:
    case REQUEST_ENTITY_TOO_LARGE_4_13:
    	hdr_ptr = http_header_413;
    	break;

	/* ------  5xx  ------ */
    case INTERNAL_SERVER_ERROR_500:
    case INTERNAL_SERVER_ERROR_5_00:
    	hdr_ptr = http_header_500;
    	break;
    case SERVICE_UNAVAILABLE_503:
    case SERVICE_UNAVAILABLE_5_03:
    	hdr_ptr = http_header_503;
    	break;

	/* ------  Dummy  ------ */
    /* Used for unimplemented status codes and to help the debug */
    case DUMMY_STATUS_999:
    	hdr_ptr = http_header_999;
    	break;

    default:
    	hdr_ptr = http_header_998;
    	break;
    }

    http_pkt->response.status = hdr_ptr;
	return 1;
}

/** Get the content-type of a request. */
int http_get_header_content_type(void *request,
							 unsigned int *content_format) {
	httpd_state *const http_pkt = (httpd_state *)request;

	// TODO: this should be from the request, not from the response
	// Assume http_pkt->response.content_type is already converted
	*content_format = (unsigned int)http_pkt->response.content_type;
	return 1;
}

/** Set the Content-Type of a response. */
int http_set_header_content_type(void *request,
							 unsigned int content_type) {
	httpd_state *const http_pkt = (httpd_state *)request;
	const char * content_t_ptr;

	// TODO: replace dummys
	switch(content_type) {
	case TEXT_PLAIN:
		content_t_ptr = http_content_type_txt_plain;
		break;
	case TEXT_XML:
		content_t_ptr = http_content_type_txt_xml;
		break;
	case TEXT_HTML:
		content_t_ptr = http_content_type_txt_html;
		break;
	case APPLICATION_XML:
		content_t_ptr = http_content_type_app_xml;
		break;
	case APPLICATION_JSON:
		content_t_ptr = http_content_type_app_json;
		break;

	case TEXT_CSV:
	case IMAGE_GIF:
	case IMAGE_JPEG:
	case IMAGE_PNG:
	case IMAGE_TIFF:
	case AUDIO_RAW:
	case VIDEO_RAW:
	case APPLICATION_LINK_FORMAT:
	case APPLICATION_OCTET_STREAM:
	case APPLICATION_RDF_XML:
	case APPLICATION_SOAP_XML:
	case APPLICATION_ATOM_XML:
	case APPLICATION_XMPP_XML:
	case APPLICATION_EXI:
	case APPLICATION_FASTINFOSET:
	case APPLICATION_SOAP_FASTINFOSET:
	case APPLICATION_X_OBIX_BINARY:
		content_t_ptr = http_content_type_dummy;
		break;

	default:
		return 0;
	}


	http_pkt->response.content_type = content_t_ptr;
	return 1;
}

/** Get the Accept types of a request. */
int http_get_header_accept(void *request, unsigned int *accept) {
	//httpd_state *const http_pkt = (httpd_state *)request;


	return 0;
}

/** Get the Length option of a request. */
int http_get_header_size2(void *request, uint32_t *size) {
	// TODO
	return 0;
}

/** Set the Length option of a response. */
int http_set_header_size2(void *response, uint32_t size) {
	// TODO
	return 0;
}

/** Get the Max-Age option of a request. */
int http_get_header_max_age(void *request, uint32_t *age) {
	// TODO
	return 0;
}

/** Set the Max-Age option of a response. */
int http_set_header_max_age(void *response, uint32_t age) {
	//httpd_state *const http_pkt = (httpd_state *)response;
	// TODO
	return 0;
}

/** Set the ETag option of a response. */
int http_set_header_etag(void *response, const uint8_t *etag,
					 size_t length) {
	//httpd_state *const http_pkt = (httpd_state *)response;
	// TODO: etag é especifico do CoAP, e serve para marcar a resource com uma versão.


	// return: len da etag
	return 0;
}

/** Get the If-Match option of a request. */
int http_get_header_if_match(void *request, const uint8_t **etag) {
	//httpd_state *const http_pkt = (httpd_state *)request;

	// TODO
	return 0;
}

/** Get the If-Match option of a request. */
int http_get_header_if_none_match(void *request) {
	//httpd_state *const http_pkt = (httpd_state *)request;

	// TODO
	return 0;
}

/** Get the Host option of a request. */
int http_get_header_uri_host(void *request, const char **host) {
	//httpd_state *const http_pkt = (httpd_state *)request;

	// TODO
	return 0;
}

/** Set the location option of a response. */
int http_set_header_location_path(void *response, const char *location) {
	//httpd_state *const http_pkt = (httpd_state *)response;

	// TODO
	return 0;
}

/** Get the payload option of a request. */
int http_get_payload(void *request, const uint8_t **payload) {
	//httpd_state *const http_pkt = (httpd_state *)request;
	// TODO
	return 0;
}

/** Set the payload option of a response. */
int http_set_payload(void *request, const void *payload,
						  size_t length) {
	httpd_state *const http_pkt = (httpd_state *)request;

	http_pkt->response.blen = MIN(REST_MAX_CHUNK_SIZE, length);
	memcpy(http_pkt->response.buf, payload, http_pkt->response.blen);
	return http_pkt->response.blen;
}

/** Get the query string of a request. */
int http_get_header_uri_query(void *request, const char **value) {
	// TODO
	return 0;
}

/** Get the value of a request query key-value pair. */
int http_get_query_variable(void *request, const char *name,
						const char **value) {
	http_packet_t *req = (http_packet_t *)request;

	return http_get_variable(req->uri_query, req->uri_query_len,
							 name, value);
}

/** Get the value of a request POST key-value pair. */
int http_get_post_variable(void *request, const char *name,
					   const char **value) {
	// TODO
	return 0;
}

/** Send the payload to all subscribers of the resource at url. */
void http_notify_observers(resource_t *resource) {
	// TODO
}

void
http_observe_handler(resource_t *resource, void *request, void *response) {

}

static int
http_get_variable(const char *buffer, size_t length, const char *name,
                  const char **output)
{
  const char *start = NULL;
  const char *end = NULL;
  const char *value_end = NULL;
  size_t name_len = 0;

  /*initialize the output buffer first */
  *output = 0;

  name_len = strlen(name);
  end = buffer + length;

  for(start = buffer; start + name_len < end; ++start) {
    if((start == buffer || start[-1] == '&') && start[name_len] == '='
       && strncmp(name, start, name_len) == 0) {

      /* Point start to variable value */
      start += name_len + 1;

      /* Point end to the end of the value */
      value_end = (const char *)memchr(start, '&', end - start);
      if(value_end == NULL) {
        value_end = end - 1;
      }
      *output = start;

      return value_end - start;
    }
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
/*- REST Engine Interface ---------------------------------------------------*/
/*---------------------------------------------------------------------------*/
struct rest_implementation http_rest_implementation = {
  "HTTP-INT",

  http_init_engine,
  http_set_service_callback,

  http_get_header_uri_path,
  http_get_rest_method,
  http_set_status_code,

  http_get_header_content_type,
  http_set_header_content_type,
  http_get_header_accept,
  http_get_header_size2,
  http_set_header_size2,
  http_get_header_max_age,
  http_set_header_max_age,
  http_set_header_etag,
  http_get_header_if_match,
  http_get_header_if_none_match,
  http_get_header_uri_host,
  http_set_header_location_path,

  http_get_payload,
  http_set_payload,

  http_get_header_uri_query,
  http_get_query_variable,
  http_get_post_variable,

  http_notify_observers,
  http_observe_handler,

  // TODO: replace dummys
  {
    OK_200,
	DUMMY_STATUS_999, //CREATED_2_01,
	DUMMY_STATUS_999, //CHANGED_2_04,
	DUMMY_STATUS_999, //DELETED_2_02,
	DUMMY_STATUS_999, //VALID_2_03,
	BAD_REQUEST_400,
	DUMMY_STATUS_999, //UNAUTHORIZED_4_01,
	DUMMY_STATUS_999, //BAD_OPTION_4_02,
    FORBIDDEN_403,
    NOT_FOUND_404,
	METHOD_NOT_ALLOWED_405,
	DUMMY_STATUS_999, //NOT_ACCEPTABLE_4_06,
    REQUEST_ENTITY_TL_413,
	DUMMY_STATUS_999, //UNSUPPORTED_MEDIA_TYPE_4_15,
    INTERNAL_SERVER_ERROR_500,
	DUMMY_STATUS_999, //NOT_IMPLEMENTED_5_01,
	DUMMY_STATUS_999, //BAD_GATEWAY_5_02,
	SERVICE_UNAVAILABLE_503,
	DUMMY_STATUS_999, //GATEWAY_TIMEOUT_5_04,
	DUMMY_STATUS_999, //PROXYING_NOT_SUPPORTED_5_05
  },

  {
    TEXT_PLAIN,
    TEXT_XML,
	TEXT_CSV,
    TEXT_HTML,
	IMAGE_GIF,
	IMAGE_JPEG,
	IMAGE_PNG,
	IMAGE_TIFF,
	AUDIO_RAW,
	VIDEO_RAW,
	APPLICATION_LINK_FORMAT,
    APPLICATION_XML,
	APPLICATION_OCTET_STREAM,
	APPLICATION_RDF_XML,
	APPLICATION_SOAP_XML,
	APPLICATION_ATOM_XML,
	APPLICATION_XMPP_XML,
	APPLICATION_EXI,
	APPLICATION_FASTINFOSET,
	APPLICATION_SOAP_FASTINFOSET,
    APPLICATION_JSON,
	APPLICATION_X_OBIX_BINARY
  }
};
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

/**
 * @}
 */
