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
/**
 * \addtogroup cc26xx-web-demo
 * @{
 *
 * \file
 *     A simple web server which displays network and sensor information
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6-route.h"
//#include "batmon-sensor.h"
//#include "lib/sensors.h"
#include "lib/list.h"
//#include "cc26xx-web-demo.h"
//#include "mqtt-client.h"
//#include "net-uart.h"

#include <stdint.h>
#include <string.h>
//#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include "debug.h"
#include "coap_client.h"
#include "er-coap-constants.h"

#include "er-http.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define HTTPD_SIMPLE_POST_HANDLER_OK      1
#define HTTPD_SIMPLE_POST_HANDLER_UNKNOWN 0
#define HTTPD_SIMPLE_POST_HANDLER_ERROR   0xFFFFFFFF

/*---------------------------------------------------------------------------*/
#define SEND_STRING(s, str) PSOCK_SEND(s, (uint8_t *)str, strlen(str))
/*---------------------------------------------------------------------------*/
#define CONNS                2
#define CONTENT_LENGTH_MAX 256
#define STATE_WAITING        0
#define STATE_OUTPUT         1
#define IPADDR_BUF_LEN      64
/*---------------------------------------------------------------------------*/
/* POST request machine states */
#define PARSE_POST_STATE_INIT            0
#define PARSE_POST_STATE_MORE            1
#define PARSE_POST_STATE_READING_KEY     2
#define PARSE_POST_STATE_READING_VAL     3
#define PARSE_POST_STATE_ERROR  0xFFFFFFFF
/*---------------------------------------------------------------------------*/
/*
 * We can only handle a single POST request at a time. Since a second POST
 * request cannot interrupt us while obtaining a lock, we don't really need
 * this lock to be atomic.
 *
 * An HTTP connection will first request a lock before it starts processing
 * a POST request. We maintain a global lock which is either NULL or points
 * to the http conn which currently has the lock
 */
/*---------------------------------------------------------------------------*/
PROCESS(httpd_process, "Web Server");
/*---------------------------------------------------------------------------*/
#define ISO_nl        0x0A
#define ISO_space     0x20
#define ISO_slash     0x2F
#define ISO_amp       0x26
#define ISO_column    0x3A
#define ISO_equal     0x3D
/*---------------------------------------------------------------------------*/


static const char http_get[] = "GET ";
static const char http_post[] = "POST ";
static const char http_put[] = "PUT ";
/*---------------------------------------------------------------------------*/
static const char *NOT_FOUND = "<html><body bgcolor=\"white\">"
                               "<center>"
                               "<h1>404 - File Not Found</h1>"
                               "</center>"
                               "</body>"
                               "</html>";

static const char *http_header_srv_str[] = {
  "Server: Contiki, ",
  BOARD_STRING "\r\n",
  NULL
};

static const char *http_header_con_close[] = {
  CONN_CLOSE,
  NULL
};
LIST(post_handlers);
void
httpd_simple_register_post_handler(httpd_simple_post_handler_t *h)
{
	PRINTF("Passei por aqui\n");
  list_add(post_handlers, h);
}

/*---------------------------------------------------------------------------*/
//static const httpd_simple_post_handler_t *handler;
/*---------------------------------------------------------------------------*/


MEMB(conns, httpd_state, CONNS);
/*---------------------------------------------------------------------------*/


static service_callback_t service_cbk = NULL;

void http_set_service_callback(service_callback_t callback) {
	service_cbk = callback;
}

/* Envia Bocados */
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


/*---------------------------------------------------------------------------
static void
lock_obtain(httpd_state *s)
{
  if(lock == NULL) {
    lock = s;
  }
}*/
/*---------------------------------------------------------------------------
static void
lock_release(httpd_state *s)
{
  if(lock == s) {
    lock = NULL;
  }
}*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_string(httpd_state *s, const char *str))
{
  PSOCK_BEGIN(&s->sout);

  SEND_STRING(&s->sout, str);

  PSOCK_END(&s->sout);
}

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
/*---------------------------------------------------------------------------*/

static
PT_THREAD(handle_output(httpd_state *s, int resourse_found))
{
  PT_BEGIN(&s->outputpt);

  PT_INIT(&s->generate_pt); /* TODO ver isso */
  /*PT_THREAD(send_headers(httpd_state *s, const char *statushdr,
                         const char *content_type, const char *redir,
                         const char **additional))*/

  /* Send a Not Found status */
  if(!resourse_found) {
      PT_WAIT_THREAD(&s->outputpt, send_headers(s, http_header_404,
                                                http_content_type_html,
                                                NULL,
                                                http_header_con_close));

      PT_WAIT_THREAD(&s->outputpt,
                     send_string(s, NOT_FOUND));
      uip_close();
      PT_EXIT(&s->outputpt);
  }

  if(s->request_type == REQUEST_TYPE_POST) {
	  PRINTF("***** METHOD POST *******\n");
	  //TODO ver como e que lidadmos com esta resposta
	  PRINTF("Return Code: %d\n", s->return_code);
    if(s->return_code == RETURN_CODE_OK) {
    	/* TODO: Neste caso temos que dar um novo sitio para redirecionar */
    	PRINTF("RETURN_CODE_OK\n");
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  http_header_302,
					  s->response.content_type,
					  NULL,
					  http_header_con_close
			  )
	  	  );
    } else if(s->return_code == RETURN_CODE_LR) {
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  http_header_411,
					  s->response.content_type,
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
					  s->response.content_type,
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
					  s->response.content_type,
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
					  s->response.content_type,
					  NULL,
					  http_header_con_close
				  )
			  );
      PT_WAIT_THREAD(&s->outputpt, send_string(s, "Bad Request\n"));
    }
  } else if(s->request_type == REQUEST_TYPE_GET) {
	  PRINTF("***** METHOD GET *******\n");
	  PRINTF("Content-Type: %s\n", s->response.content_type);
	  PRINTF("Status Str: %s\n", s->response.status);

	  /* Send Headers */
	  PT_WAIT_THREAD(
			  &s->outputpt,
			  send_headers(
					  s,
					  s->response.status,
					  s->response.content_type,
					  NULL,
					  http_header_con_close
			  ));

	/* Send response body */
	PT_WAIT_THREAD(&s->outputpt, send_string(s, s->response.buf));
//	PT_WAIT_THREAD(
//			&s->outputpt,
//			enqueue_chunk(s, 1, s->response.buf)
//			);
  }

  PSOCK_CLOSE(&s->sout);
  PT_END(&s->outputpt);
}

/*---------------------------------------------------------------------------*/
static char * ptr_toWrite;
static
PT_THREAD(handle_input(httpd_state *s))
{
  PSOCK_BEGIN(&s->sin);

  PSOCK_READTO(&s->sin, ISO_space);

  if(strncasecmp(s->inputbuf, http_get, 4) == 0) {

	PRINTF("***** handle_input: GET *******\n");
	s->request_type = REQUEST_TYPE_GET;
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


  } else if(strncasecmp(s->inputbuf, http_post, 5) == 0) {

	PRINTF("***** handle_input: POST *******\n");
	s->request_type = REQUEST_TYPE_POST;
	PSOCK_READTO(&s->sin, ISO_space);

	if(s->inputbuf[0] != ISO_slash) {
		PSOCK_CLOSE_EXIT(&s->sin);
	}

	s->uri_len = PSOCK_DATALEN(&s->sin) - 1; // we remove the space at the end
	memcpy(&s->uri, &s->inputbuf, s->uri_len);

    s->inputbuf[PSOCK_DATALEN(&s->sin) - 1] = 0;


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
        PRINTF("INPUTBUF: %s\n", s->inputbuf);
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

    //TODO: ver isto
    /*if(s->return_code == RETURN_CODE_OK) {
       Acceptable Content Length. Try to obtain a lock
      lock_obtain(s);

      if(lock == s) {
        state = PARSE_POST_STATE_INIT;
      } else {
    	  PRINTF("Entrei aqui codigo service una\n");
        s->return_code = RETURN_CODE_SU;
      }
    }*/

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

    if(s->content_length > 0)
    	*(++ptr_toWrite) = '\0';

    //
    s->content_length = s->response.content_length;
    s->response.content_length = 0;

    //TODO: ver isto
    //lock_release(s);
  } else if(strncasecmp(s->inputbuf, http_put, 4) == 0){

	  PRINTF("***** handle_input: PUT *******\n\n");

  }else {
    PSOCK_CLOSE_EXIT(&s->sin);
  }

  s->state = STATE_OUTPUT;

  while(1) {
    PSOCK_READTO(&s->sin, ISO_nl);
  }

  PSOCK_END(&s->sin);
}
/*---------------------------------------------------------------------------*/
static void
handle_connection(httpd_state *s)
{
  handle_input(s);
  if(s->state == STATE_OUTPUT) {
	rest_select_if(HTTP_IF);
	/* By default, all endpoints have an immediate response */
	s->response.imediate_response = 1;

	PRINTF("\n** Calling Service Callback\n");
	int res_found = service_cbk(s, &s->response, (uint8_t *)&s->response.buf, 0, 0);

	PRINTF("\n** State: %d, res_found = %d, Immediate: %d\n", s->return_code, res_found, s->response.imediate_response);

	if(s->response.imediate_response){
	    handle_output(s, res_found);
	}
  }
}
/*---------------------------------------------------------------------------*/
static void
parse_coap(coap_client_request_t *coap_request, httpd_state *s){

    if(coap_request->method == COAP_GET){
        memcpy(s->response.buf, coap_request->buffer, coap_request->blen);
        s->response.blen = coap_request->blen;
    }
    //s->response.status = coap_request->status_code;
    //TODO: nao esquecer
//    if(coap_request->status_code == CONTENT_2_05){
//        s->response.status = 200;
//    }
    s->request_type = coap_request->method;

    //TODO: pode nao ser 200
    s->response.status = http_header_200;
    s->response.content_type = http_content_type_plain;
    //s->return_code = coap_requ TODO:

    s->currentConnection = coap_request->connection;


}

/*---------------------------------------------------------------------------*/
static void
proxy_call(void *state){
    coap_client_request_t *coap_request = (coap_client_request_t *)state;

    httpd_state *s = (httpd_state *)memb_alloc(&conns);

    s->blen = 0;
    s->tmp_buf_len = 0;
    s->response.blen = 0;
    memset(s->buffer, 0, sizeof(s->buffer));

    /* Call Parse */
    parse_coap(coap_request, s);

    if(s == NULL) {
        uip_abort();
        return;
    }
    tcp_markconn(s->currentConnection, s);

    //PSOCK_INIT(&s->sin, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);
    PSOCK_INIT(&s->sout, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);

    PT_INIT(&s->outputpt);

    s->state = STATE_WAITING;

    timer_set(&s->timer, CLOCK_SECOND * 10);
    handle_output(s, 1);

}
/*---------------------------------------------------------------------------*/
static void
appcall(void *state)
{
  httpd_state *s = (httpd_state *)state;

  if(uip_closed() || uip_aborted() || uip_timedout()) {
	if(s != NULL) {
		s->blen = 0;
		s->tmp_buf_len = 0;
		s->response.blen = 0;
		memset(s->buffer, 0, sizeof(s->buffer));
		memb_free(&conns, s);
	}
  } else if(uip_connected()) {
	  s = (httpd_state *)memb_alloc(&conns);
	  if(s == NULL) {
		  uip_abort();
		  return;
	  }
	  tcp_markconn(uip_conn, s);
	  PSOCK_INIT(&s->sin, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);
	  PSOCK_INIT(&s->sout, (uint8_t *)s->inputbuf, sizeof(s->inputbuf) - 1);

	  PT_INIT(&s->outputpt);

	  s->state = STATE_WAITING;

	  s->currentConnection = uip_conn;

	  timer_set(&s->timer, CLOCK_SECOND * 10);
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
/*---------------------------------------------------------------------------*/

static void
init(void)
{
  tcp_listen(UIP_HTONS(80));
  memb_init(&conns);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(httpd_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("Web Server\n");

  // TODO: ?
  //httpd_simple_event_new_config = process_alloc_event();

  init();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event || ev == PROCESS_EVENT_CUSTOM);

    if(ev == tcpip_event){
        appcall(data);
    }else if(ev == PROCESS_EVENT_CUSTOM){
        proxy_call(data);
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
