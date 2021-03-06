/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*---------------------------------------------------------------------------*/

/**
 * @file     httpd.c
 * @brief    Contains the logic of handling the connections a processing the http
 * @version  1.0
 * @date     01 Jun. 2017
 * @author   Tiago Costa & Ricardo Jesus & Claudio Prates
 *
 **/

/** @defgroup HTTP HTTP
 * @{
 */

/** @addtogroup Server
 * @{
 */

#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6-route.h"
#include "lib/list.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include "debug.h"
#include "coap_client.h"
#include "er-coap-constants.h"

#include "httpd.h"
#include "er-http.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define SEND_STRING(s, str) PSOCK_SEND(s, (uint8_t *)str, strlen(str)) /*!< Sends a String through the socket */

PROCESS(httpd_process, "Web Server"); /*!< Process of HTTP Server */
/*---------------------------------------------------------------------------*/
static const char http_get[] = "GET "; /*!< String of the HTTP Method GET */
static const char http_post[] = "POST "; /*!< String of the HTTP Method POST */
static const char http_put[] = "PUT "; /*!< String of the HTTP Method PUT */
static const char http_delete[] = "DELETE "; /*!< String of the HTTP Method DELETE */
/*---------------------------------------------------------------------------*/
static const char *NOT_FOUND = "<html><body bgcolor=\"white\">"
                               "<center>"
                               "<h1>404 - File Not Found</h1>"
                               "</center>"
                               "</body>"
                               "</html>"; /*!< NOT FOUND Template */

const char *default_content_type = http_content_type_txt_html; /*!< Default Content-Type */

process_event_t coap_client_event_new_response; /*!< Event New Response */

static const char *http_header_srv_str[] = {
  "Server: Contiki, ",
  BOARD_STRING "\r\n",
  NULL
}; /*!< HTTP Server Header String */

static const char *http_header_con_close[] = {
  CONN_CLOSE,
  NULL
}; /*!< Con Close Template */

MEMB(conns, httpd_state, CONNS); /*!< Allocation of http_state to the variable conns */

static service_callback_t service_cbk = NULL; /*!< Rest Engine Callback */

/**
 * @brief Rest Implementation of setting the callback
 *
 * Example query 'p1=param1;p2=param2' converts to 'p1=param1&p2=param2'
 *
 * @param callback : A callback to be called once a resource is found
 * @return nothing
 */
void http_set_service_callback(service_callback_t callback) {
	service_cbk = callback;
}

/**
 * @brief ProtoThread: Handling a enqueue_chunk function
 *
 * Sends data in chunks
 *
 * @param httpd_state : query to convert
 * @param uint8_t : size of the entire query
 * @param const char : size of the entire query
 * @return nothing
 */
static
PT_THREAD(enqueue_chunk(httpd_state *s, uint8_t immediate,
                        const char *format, ...))
{
  va_list ap;

  PSOCK_BEGIN(&s->sout);

  va_start(ap, format);

  s->tmp_buf_len = vsnprintf(s->tmp_buf, TMP_BUF_SIZE, format, ap);

  va_end(ap);

  if(s->blen + s->tmp_buf_len < HTTPD_SIMPLE_MAIN_BUF_SIZE) {
    /* Enough space for the entire chunk. Copy over */
    memcpy(&s->buf[s->blen], s->tmp_buf, s->tmp_buf_len);
    s->blen += s->tmp_buf_len;
  } else {
    memcpy(&s->buf[s->blen], s->tmp_buf, HTTPD_SIMPLE_MAIN_BUF_SIZE - s->blen);
    s->tmp_buf_copied = HTTPD_SIMPLE_MAIN_BUF_SIZE - s->blen;
    s->blen = HTTPD_SIMPLE_MAIN_BUF_SIZE;
    PSOCK_SEND(&s->sout, (uint8_t *)s->buf, s->blen);
    s->blen = 0;
    if(s->tmp_buf_copied < s->tmp_buf_len) {
      memcpy(s->buf, &s->tmp_buf[s->tmp_buf_copied],
             s->tmp_buf_len - s->tmp_buf_copied);
      s->blen += s->tmp_buf_len - s->tmp_buf_copied;
    }
  }

  if(immediate != 0 && s->blen > 0) {
    PSOCK_SEND(&s->sout, (uint8_t *)s->buf, s->blen);
    s->blen = 0;
  }

  PSOCK_END(&s->sout);
}

/**
 * @brief Send a string through the SOCKET
 *
 * @param s : HTTP State
 * @param str : String to Send
 * @return nothing
 */
static
PT_THREAD(send_string(httpd_state *s, const char *str))
{
  PSOCK_BEGIN(&s->sout);

  SEND_STRING(&s->sout, str);

  PSOCK_END(&s->sout);
}

/**
 * @brief Send Headers through the SOCKET
 *
 * @param s : HTTP State
 * @param statushdr : Header Status Code (404, 200, etc)
 * @param content_type : Content-Type to send in the Header
 * @param redir : New URL to Redirect (Normally associated with POST "Location: {URL}")
 * @param additional : Aditional Headers 
 * @return nothing
 */
static
PT_THREAD(send_headers(httpd_state *s, const char *statushdr,
                       const char *content_type, const char *redir,
                       const char **additional))
{
  PT_BEGIN(&s->generate_pt);

  PT_WAIT_THREAD(&s->generate_pt, enqueue_chunk(s, 0, statushdr));

  for(s->ptr = http_header_srv_str; *(s->ptr) != NULL; s->ptr++) {
    PT_WAIT_THREAD(&s->generate_pt, enqueue_chunk(s, 0, *(s->ptr)));
  }

  if(redir) {
    PT_WAIT_THREAD(&s->generate_pt,
                   enqueue_chunk(s, 0, "Location: %s\r\n", redir));
  }

  if(additional) {
    for(s->ptr = additional; *(s->ptr) != NULL; s->ptr++) {
      PT_WAIT_THREAD(&s->generate_pt, enqueue_chunk(s, 0, *(s->ptr)));
    }
  }

  PT_WAIT_THREAD(&s->generate_pt,
                 enqueue_chunk(s, 0, "Content-type: %s; ", content_type));

  PT_WAIT_THREAD(&s->generate_pt, enqueue_chunk(s, 1, "charset=UTF-8\r\n\r\n"));

  PT_END(&s->generate_pt);
}

/**
 * @brief Handles the Output response
 *
 * @param s : HTTP State
 * @return nothing
 */
static
PT_THREAD(handle_output(httpd_state *s))
{
  PT_BEGIN(&s->outputpt);

  PT_INIT(&s->generate_pt);

  /* Send a Not Found status */
  if(!(s->state & STATE_RES_FOUND) && (s->return_code == RETURN_CODE_OK)) {
      PT_WAIT_THREAD(&s->outputpt, send_headers(s, http_header_404,
                                                http_content_type_txt_html,
                                                NULL,
                                                http_header_con_close));

      PT_WAIT_THREAD(&s->outputpt,
                     send_string(s, NOT_FOUND));
      uip_close();
      PT_EXIT(&s->outputpt);
  }

  if(s->request_type == REQUEST_TYPE_POST) {
	  PRINTF("***** METHOD POST *******\n");
	  PRINTF("Return Code: %d\n", s->return_code);
    if(s->return_code == RETURN_CODE_OK) {
      PRINTF("RETURN_CODE_OK\n");
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  s->response.status,
					  (s->response.content_type == 0)? default_content_type : s->response.content_type,
					  s->response.redir_path,
					  s->response.additional_hdrs
			  )
	  	  );
    } else if(s->return_code == RETURN_CODE_LR) {
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  http_header_411,
					  http_content_type_txt_html,
					  NULL,
					  http_header_con_close
				  )
			  );
      PT_WAIT_THREAD(&s->outputpt, send_string(s, "Content-Length Required\n"));
    } else if(s->return_code == RETURN_CODE_TL) {
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  http_header_413,
					  http_content_type_txt_html,
					  NULL,
					  http_header_con_close
				  )
			  );
      PT_WAIT_THREAD(&s->outputpt, send_string(s, "Content-Length too Large\n"));
    } else if(s->return_code == RETURN_CODE_SU) {
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  http_header_503,
					  http_content_type_txt_html,
					  NULL,
					  http_header_con_close
				  )
			  );
      PT_WAIT_THREAD(&s->outputpt, send_string(s, "Service Unavailable\n"));
    } else {
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  http_header_400,
					  http_content_type_txt_html,
					  NULL,
					  http_header_con_close
				  )
			  );
      PT_WAIT_THREAD(&s->outputpt, send_string(s, "Bad Request\n"));
    }
  } else if( (s->request_type == REQUEST_TYPE_GET) ||
		  (s->request_type == REQUEST_TYPE_PUT)) {
#ifdef DEBUG
	  if(s->request_type == REQUEST_TYPE_GET) {
		  PRINTF("***** METHOD GET *******\n");
	  } else {
		  PRINTF("***** METHOD PUT *******\n");
	  }
#endif
	  PRINTF("Content-Type: %s\n", s->response.content_type);
	  PRINTF("Status Str: %s\n", s->response.status);

	  /* Send Headers */
	  PT_WAIT_THREAD(
			  &s->outputpt,
			  send_headers(
					  s,
					  s->response.status,
					  (s->response.content_type == 0)? default_content_type : s->response.content_type,
					  NULL,
					  s->response.additional_hdrs
			  ));

	/* Send response body */
    if(s->response.large_rsp == RESPONSE_TYPE_NORMAL) {
	  // Guarantees the null terminator
      int idx = MIN(s->response.blen, HTTPD_SIMPLE_MAIN_BUF_SIZE - 1);
      s->response.buf[idx] = ISO_null_term;
	  PT_WAIT_THREAD(&s->outputpt, send_string(s, s->response.buf));
//  	PT_WAIT_THREAD(
//	  		&s->outputpt,
//		  	enqueue_chunk(s, 1, s->response.buf)
//			);
    } else if (s->response.large_rsp == RESPONSE_TYPE_LARGE) {
      if(s->response.large_rsp_hnd != NULL) {
    	  while(s->response.large_rsp_hnd(&s->response.buf, HTTPD_SIMPLE_MAIN_BUF_SIZE) > 0) {
    		  PT_WAIT_THREAD(&s->outputpt, send_string(s, s->response.buf));
    	  }
      } else {
        PRINTF("\n\n!!! Large response with no handler!\n\n");
      }
    }
  }

  PSOCK_CLOSE(&s->sout);
  PT_END(&s->outputpt);
}


static char * ptr_toWrite;

/**
 * @brief Handles the Input Request
 *
 * Reads Data from the socket and interprets the result
 *
 * @param s : HTTP State
 * @return nothing
 */
static
PT_THREAD(handle_input(httpd_state *s))
{
  PRINTF("** Processing Input.\n");

  PSOCK_BEGIN(&s->sin);

  PSOCK_READTO(&s->sin, ISO_space);

  /* ---------------------------- Handle GET and PUT ---------------------------- */
  if( (strncasecmp(s->inputbuf, http_get, 4) == 0) ||
		  (strncasecmp(s->inputbuf, http_put, 4) == 0)) {

	if(strncasecmp(s->inputbuf, http_get, 4) == 0) {
	  PRINTF("***** handle_input: GET *******\n");
	  s->request_type = REQUEST_TYPE_GET;
	} else {
	  PRINTF("***** handle_input: PUT *******\n");
	  s->request_type = REQUEST_TYPE_PUT;
	}

	PSOCK_READTO(&s->sin, ISO_space);
	// copy uri
	s->complete_uri_len = PSOCK_DATALEN(&s->sin) - 1; // we remove the space at the end
	memcpy(&s->complete_uri, &s->inputbuf, s->complete_uri_len);
	s->complete_uri[s->complete_uri_len] = '\0';

	s->uri = s->complete_uri;
	s->uri_query = strstr(s->complete_uri, "?");

	if(s->uri_query != NULL){
		/* move forward one byte uri_query to remove '?' */
		s->uri_query++;
	    s->uri_len = s->uri_query - s->complete_uri - 1; /* -1 to remove '?' from complete_uri */
	    s->uri_query_len = s->complete_uri_len - s->uri_len;
	}
	else{
	    s->uri_query_len = 0;
	    s->uri_len = s->complete_uri_len;
	}

	if(s->inputbuf[0] != ISO_slash) {
		PSOCK_CLOSE_EXIT(&s->sin);
	}

  /* ---------------------------- Handle POST ---------------------------- */
  } else if(strncasecmp(s->inputbuf, http_post, 5) == 0) {

	PRINTF("***** handle_input: POST *******\n");
	s->request_type = REQUEST_TYPE_POST;
	PSOCK_READTO(&s->sin, ISO_space);

	if(s->inputbuf[0] != ISO_slash) {
		PSOCK_CLOSE_EXIT(&s->sin);
	}


	s->complete_uri_len = PSOCK_DATALEN(&s->sin) - 1; // we remove the space at the end
	memcpy(&s->complete_uri, &s->inputbuf, s->complete_uri_len);
	s->complete_uri[s->complete_uri_len] = '\0';

	s->uri = s->complete_uri;
	s->uri_query = strstr(s->complete_uri, "?");

	if(s->uri_query != NULL){
		/* move forward one byte uri_query to remove '?' */
		s->uri_query++;
	    s->uri_len = s->uri_query - s->complete_uri - 1; /* -1 to remove '?' from complete_uri */
	    s->uri_query_len = s->complete_uri_len - s->uri_len;
	}
	else{
	    s->uri_query_len = 0;
	    s->uri_len = s->complete_uri_len;
	}

    //s->inputbuf[PSOCK_DATALEN(&s->sin) - 1] = ISO_null_term;


    /* POST: Read out the rest of the line and ignore it */
    PSOCK_READTO(&s->sin, ISO_nl);

    /*
     * Start parsing headers. We look for Content-Length and ignore everything
     * else until we hit the start of the message body.status_code
     *
     * We will return 411 if the client doesn't send Content-Length and 413
     * if Content-Length is too high
     */
    s->content_length = 0;
    s->return_code = RETURN_CODE_LR;

    do {
      s->inputbuf[PSOCK_DATALEN(&s->sin)] = 0;
      /* We anticipate a content length */
      if((PSOCK_DATALEN(&s->sin) > 14) &&
         strncasecmp(s->inputbuf, "Content-Length:", 15) == 0) {
        char *val_start = &s->inputbuf[15];
        s->content_length = atoi(val_start);

        /* So far so good */
        s->return_code = RETURN_CODE_OK;
      }
      PSOCK_READTO(&s->sin, ISO_nl);
    } while(PSOCK_DATALEN(&s->sin) != 2);

    /*
     * Done reading headers.
     * Reject content length greater than CONTENT_LENGTH_MAX bytes
     */
    if(s->content_length > CONTENT_LENGTH_MAX) {
      s->content_length = 0;
      s->return_code = RETURN_CODE_TL;
    }

    /* Parse the message body, unless we have detected an error. */

    s->response.content_length = s->content_length;
    ptr_toWrite = s->buffer;
    PRINTF("content_length = %d || return_code = %d\n",
           s->content_length,
           s->return_code);

    while(s->content_length > 0 /*&& lock == s */&&
          s->return_code == RETURN_CODE_OK) {
      PSOCK_READBUF_LEN(&s->sin, s->content_length);

      s->content_length -= PSOCK_DATALEN(&s->sin);

      int len = PSOCK_DATALEN(&s->sin);
      memcpy(ptr_toWrite, &s->inputbuf, len);
      ptr_toWrite += len;

    }

    // TODO: retirar isto e ver se dá barraca...
    if(s->content_length > 0)
    	*(++ptr_toWrite) = ISO_null_term;

    //
    s->content_length = s->response.content_length;
    s->response.content_length = 0;
  /* ---------------------------- Handle PUT ---------------------------- */
  } else {
    PSOCK_CLOSE_EXIT(&s->sin);
  }

  s->state = STATE_OUTPUT;

  while(1) {
    PSOCK_READTO(&s->sin, ISO_nl);
  }

  PSOCK_END(&s->sin);
}

/**
 * @brief Handles the incoming connections
 *
 * Receives connections and pass it to Handle Input for processing
 * Call's a service callback (rest implementation) after handling the input
 *
 * @param s : HTTP State
 * @return nothing
 */
static void
handle_connection(httpd_state *s)
{
  int res_found = 0;

  if(s->state & STATE_IGNORE) {
	  return;
  }

  PRINTF("** Handle Connection\n");
  if(!(s->state & STATE_OUTPUT)) {
	  handle_input(s);
  }

  if(s->state & STATE_OUTPUT) {
	/* By default, all endpoints have an immediate response */
	s->response.immediate_response = 1;

	/* have already call service callback for this request? */
	if(!(s->state & STATE_PROCESSED)) {

		/* call service callback only if received data is correct */
		if(s->return_code == RETURN_CODE_OK) {
			PRINTF("** Calling Service Callback\n");
			//
			res_found = service_cbk(s, &s->response, (uint8_t *)&s->response.buf, 0, 0);
			// Update the request state
			if(res_found) {
				s->state |= STATE_RES_FOUND;
			}
		} else {
			PRINTF("** Received data NOK!\n");
			/* Force Resource Found to make handle_output process the error */
			s->state |= STATE_RES_FOUND;
		}

		s->state |= STATE_PROCESSED;

		PRINTF("\n** State: %d, res_found = %d, Immediate: %d\n", s->return_code, res_found, s->response.immediate_response);
	} else {
		PRINTF("** Request already processed or CoAP response: %d\n", s->state);
	}

	//
	if(s->response.immediate_response){
	    handle_output(s);
	}
  }
}

/**
 * @brief Converts the HTTP to CoAP
 *
 * Copies the payload, maps the status code and the content-type
 *
 * @param coap_request : query to convert
 * @param s : The HTTP State
 * @return nothing
 */
static void
parse_coap(coap_client_request_t *coap_request, httpd_state *s){

	// TODO: Discutir como funciona o payload no POST. A resposta a um POST nunca trás payload? Se verdade, então isto só deve ser feito quando não é um POST...
	// POST suporta payload na resposta, apesar de enviar um 302, pode enviar payload, para que o redirect seja mais rapido exemplo ("sends the html of the page of 302 and the redirect url, so it caches")
	// Se for verdade, no output_handler tem de se enviar o payload.
	memcpy(s->response.buf, coap_request->buffer, coap_request->blen);
	s->response.blen = coap_request->blen;

	// get the original request method
    s->request_type = coap_request->method;
    // Content-Type
    REST.set_header_content_type(s, coap_request->content_type);
    // status code
    REST.set_response_status(s, coap_request->resp_status);
}

/**
 * @brief Resets a HTTP State Object
 *
 * @param s : The HTTP State
 * @return nothing
 */
static void reset_http_state_obj(httpd_state *s) {
    s->blen = 0;
    s->tmp_buf_len = 0;
    s->response.blen = 0;
	s->response.additional_hdrs = 0;
	s->response.content_type = 0;
	s->response.redir_path = 0;
	s->response.large_rsp = RESPONSE_TYPE_NORMAL;
	s->response.large_rsp_hnd = NULL;
    memset(s->buffer, 0, sizeof(s->buffer));
}

/**
 * @brief Prapares a new CoAP Request
 *
 *
 * @param state : The CoAP Request
 * @return nothing
 */
static size_t
prepare_response(void *state){
    coap_client_request_t *coap_request = (coap_client_request_t *)state;

    if(coap_request->resp_status == 0) {
    	PRINTF("!!! No response from CoAP.\n");
    	return 1;
    }

    /* TODO: Esta verificação pode não ser suficiente no caso em que esta zona de memória seja utilizada
     * para outra conexão que neste momento possa estar activa.
     * Este problema está relacionado com o TODO mais a baixo. */
    if(((struct uip_conn *)coap_request->http_conn)->tcpstateflags != UIP_ESTABLISHED) {
    	PRINTF("!!! Connection closed.\n");
    	return 1;
    }

	uip_tcp_appstate_t *st = (uip_tcp_appstate_t *)&((struct uip_conn *)coap_request->http_conn)->appstate;
	httpd_state *s = (httpd_state *)st->state;

    /* Call Parse */
    parse_coap(coap_request, s);
    // set the new state ready do send out
    s->state = STATE_OUTPUT | STATE_PROCESSED | STATE_RES_FOUND;

    // set our connection
    // TODO: ver melhor aquela situação em que este apontador pode apontar para uma conexão
    // que pertence a outro pedido devido à "original" ter sido fechada pelo cliente ou dado timeout
	tcp_markconn(coap_request->http_conn, s);

	/* init structures */
	PSOCK_INIT(&s->sin, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);
	PSOCK_INIT(&s->sout, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);

	PT_INIT(&s->outputpt);

	timer_set(&s->timer, CLOCK_SECOND * ABORT_CONN_TIMEOUT);

	return 0;
}

/**
 * @brief Main Function
 *
 * Polling through the uip to check incoming connection, and if there is
 * starts the process of handling
 *
 * @param state : The HTTP State
 * @return nothing
 */
static void
appcall(void *state)
{
  httpd_state *s = (httpd_state *)state;

  if(uip_closed() || uip_aborted() || uip_timedout()) {
	if(s != NULL) {
		reset_http_state_obj(s);
		memb_free(&conns, s);
	}
  } else if(uip_connected()) {
	  s = (httpd_state *)memb_alloc(&conns);
	  if(s == NULL) {
		  uip_abort();
		  return;
	  }

	  reset_http_state_obj(s);
	  tcp_markconn(uip_conn, s);
	  PSOCK_INIT(&s->sin, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);
	  PSOCK_INIT(&s->sout, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);

	  PT_INIT(&s->outputpt);

	  s->state = STATE_WAITING;

	  timer_set(&s->timer, CLOCK_SECOND * ABORT_CONN_TIMEOUT);
	  handle_connection(s);

  } else if(s != NULL) {
	if(uip_poll()) {
	  if(timer_expired(&s->timer)) {
		uip_abort();
		memb_free(&conns, s);
	  }
	} else {
	    timer_restart(&s->timer);
	}

	handle_connection(s);
  } else {
        uip_abort();
    }
}

/**
 * @brief Init Function
 *
 * Starts to listen on port 80 and inits a variable conns
 *
 * @return nothing
 */
static void
init(void)
{
  tcp_listen(UIP_HTONS(80));
  memb_init(&conns);
}

/**
 * @brief Process of http_process
 *
 * Handles the events and redirects to the appropriate location
 *
 * @param httpd_process : The process
 * @param ev : The event that were fired
 * @param data : The data associated with the event
 * @return nothing
 */
PROCESS_THREAD(httpd_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("* Web Server starting...\n");

  // Get an Event Id
  coap_client_event_new_response = process_alloc_event();

  init();

  PROCESS_PAUSE();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event || ev == coap_client_event_new_response);
    PRINTF("**TODO HTTPD PROCESS WHILE!\n");

    // select the REST HTTP interface
    rest_select_if(HTTP_IF);

    if(ev == tcpip_event){
    	PRINTF("**TODO HTTPD tcp event!\n");
        appcall(data);
    }else if(ev == coap_client_event_new_response){
    	PRINTF("**TODO coap_client_event_new_response!\n");

    	// Prepare the response and poll this connection to do the right flow
    	if(!prepare_response(data)) {
    		tcpip_poll_tcp(((coap_client_request_t *)data)->http_conn);
    	}
    }

  }

  PROCESS_END();
}

/**
 * @}
 */

/**
 * @}
 */
