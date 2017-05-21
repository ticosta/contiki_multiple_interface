/*
 * er-http.c
 *
 *  Created on: Mar 23, 2017
 *      Author: user
 */

#include "contiki.h"

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

static int
http_get_variable(const char *buffer, size_t length, const char *name,
                  const char **output);

/** Initialize the REST implementation. */
void http_init_engine(void) {
	process_start(&httpd_process, NULL);
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

	return (rest_resource_flags_t)(1 <<
	                                 (http_pkt->request_type - 1));
}

/** Set the status code of a response. */
int http_set_status_code(void *response, unsigned int code) {
    httpd_state *const http_pkt = (httpd_state *)response;

    http_pkt->response.status = code;

	return http_pkt->response.status;
}

/** Get the content-type of a request. */
int http_get_header_content_type(void *request,
							 unsigned int *content_format) {
	httpd_state *const http_pkt = (httpd_state *)request;

	*content_format = (unsigned int)http_pkt->response.content_type;
	return 1;
}

/** Set the Content-Type of a response. */
int http_set_header_content_type(void *response,
							 unsigned int content_type) {
	httpd_state *const http_pkt = (httpd_state *)response;

	memcpy(http_pkt->response.content_type, "text/plain", strlen("text/plain"));

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
int http_set_payload(void *response, const void *payload,
						  size_t length) {
	http_response *rsp = (http_response *)response;

	rsp->blen = MIN(REST_MAX_CHUNK_SIZE, length);
	memcpy(rsp->buf, payload, rsp->blen);
	return rsp->blen;
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
        value_end = end;
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

  {
    CONTENT_2_05,
    CREATED_2_01,
    CHANGED_2_04,
    DELETED_2_02,
    VALID_2_03,
    BAD_REQUEST_4_00,
    UNAUTHORIZED_4_01,
    BAD_OPTION_4_02,
    FORBIDDEN_4_03,
    NOT_FOUND_4_04,
    METHOD_NOT_ALLOWED_4_05,
    NOT_ACCEPTABLE_4_06,
    REQUEST_ENTITY_TOO_LARGE_4_13,
    UNSUPPORTED_MEDIA_TYPE_4_15,
    INTERNAL_SERVER_ERROR_5_00,
    NOT_IMPLEMENTED_5_01,
    BAD_GATEWAY_5_02,
    SERVICE_UNAVAILABLE_5_03,
    GATEWAY_TIMEOUT_5_04,
    PROXYING_NOT_SUPPORTED_5_05
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
