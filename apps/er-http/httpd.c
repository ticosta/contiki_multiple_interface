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
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include "debug.h"

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
#define RETURN_CODE_OK   0
#define RETURN_CODE_NF   1 /* Not Found */
#define RETURN_CODE_SU   2 /* Service Unavailable */
#define RETURN_CODE_BR   3 /* Bad Request */
#define RETURN_CODE_LR   4 /* Length Required */
#define RETURN_CODE_TL   5 /* Content Length too Large */
/*---------------------------------------------------------------------------*/
/* POST request machine states */
#define PARSE_POST_STATE_INIT            0
#define PARSE_POST_STATE_MORE            1
#define PARSE_POST_STATE_READING_KEY     2
#define PARSE_POST_STATE_READING_VAL     3
#define PARSE_POST_STATE_ERROR  0xFFFFFFFF
/*---------------------------------------------------------------------------*/
#define PARSE_POST_BUF_SIZES            64

/* Last byte always used to null terminate */
#define PARSE_POST_MAX_POS        (PARSE_POST_BUF_SIZES - 2)

static char key[PARSE_POST_BUF_SIZES];
static char val_escaped[PARSE_POST_BUF_SIZES];
static char val[PARSE_POST_BUF_SIZES];
static int key_len;
static int val_len;
static long state;
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
static httpd_state *lock;
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
#define REQUEST_TYPE_GET  1
#define REQUEST_TYPE_POST 2
/*---------------------------------------------------------------------------*/
#define BOARD_STRING "-WISMOTE-"

/*---------------------------------------------------------------------------*/
/* Page template */
static const char http_header_200[] = HTTP_200_OK;
static const char http_header_302[] = HTTP_302_FO;
static const char http_header_400[] = HTTP_400_BR;
static const char http_header_404[] = HTTP_404_NF;
static const char http_header_411[] = HTTP_411_LR;
static const char http_header_413[] = HTTP_413_TL;
static const char http_header_503[] = HTTP_503_SU;

static const char http_get[] = "GET ";
static const char http_post[] = "POST ";
static const char http_put[] = "PUT ";

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
static const httpd_simple_post_handler_t *handler;
/*---------------------------------------------------------------------------*/


MEMB(conns, httpd_state, CONNS);
/*---------------------------------------------------------------------------*/
#define HEX_TO_INT(x)  (isdigit(x) ? x - '0' : x - 'W')

static service_callback_t service_cbk = NULL;

void http_set_service_callback(service_callback_t callback) {
	service_cbk = callback;
}
static size_t
url_unescape(const char *src, size_t srclen, char *dst, size_t dstlen)
{
  size_t i, j;
  int a, b;

  for(i = j = 0; i < srclen && j < dstlen - 1; i++, j++) {
    if(src[i] == '%' && isxdigit(*(unsigned char *)(src + i + 1))
       && isxdigit(*(unsigned char *)(src + i + 2))) {
      a = tolower(*(unsigned char *)(src + i + 1));
      b = tolower(*(unsigned char *)(src + i + 2));
      dst[j] = ((HEX_TO_INT(a) << 4) | HEX_TO_INT(b)) & 0xff;
      i += 2;
    } else if(src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }

  dst[j] = '\0';

  return i == srclen;
}
/*---------------------------------------------------------------------------*/

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

  //PRINTF("********************* Enqueue Chunk blen: %d\n", s->blen);
  //PRINTF("********************* Enqueue Chunk tmp_buf_len: %d\n", s->tmp_buf_len);

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


/*---------------------------------------------------------------------------*/
static void
lock_obtain(httpd_state *s)
{
  if(lock == NULL) {
    lock = s;
  }
}
/*---------------------------------------------------------------------------*/
static void
lock_release(httpd_state *s)
{
  if(lock == s) {
    lock = NULL;
  }
}
/*---------------------------------------------------------------------------*/
static void
parse_post_request_chunk(char *buf, int buf_len, int last_chunk)
{

  int i;
  int finish;

  for(i = 0; i < buf_len; i++) {
    switch(state) {
    case PARSE_POST_STATE_INIT:
      state = PARSE_POST_STATE_MORE;
    /* continue */
    case PARSE_POST_STATE_MORE:
      memset(key, 0, PARSE_POST_BUF_SIZES);
      memset(val, 0, PARSE_POST_BUF_SIZES);
      memset(val_escaped, 0, PARSE_POST_BUF_SIZES);
      key_len = 0;
      val_len = 0;
      state = PARSE_POST_STATE_READING_KEY;
    /* continue */
    case PARSE_POST_STATE_READING_KEY:
      if(buf[i] == ISO_equal) {
        state = PARSE_POST_STATE_READING_VAL;
      } else if(buf[i] == ISO_amp) {
        /* Don't accept an amp while reading a key */
        state = PARSE_POST_STATE_ERROR;
      } else {
        /* Make sure we don't overshoot key's boundary */
        if(key_len <= PARSE_POST_MAX_POS) {
          key[key_len] = buf[i];
          key_len++;
        } else {
          /* Not enough space for the key. Abort */
          state = PARSE_POST_STATE_ERROR;
        }
      }
      break;
    case PARSE_POST_STATE_READING_VAL:
      finish = 0;
      if(buf[i] == ISO_amp) {
        finish = 1;
      } else if(buf[i] == ISO_equal) {
        /* Don't accept an '=' while reading a val */
        state = PARSE_POST_STATE_ERROR;
      } else {
        /* Make sure we don't overshoot key's boundary */
        if(val_len <= PARSE_POST_MAX_POS) {
          val[val_len] = buf[i];
          val_len++;
          /* Last character of the last chunk */
          if((i == buf_len - 1) && (last_chunk == 1)) {
            finish = 1;
          }
        } else {
          /* Not enough space for the value. Abort */
          state = PARSE_POST_STATE_ERROR;
        }
      }

      if(finish == 1) {
        /*
         * Done reading a key=value pair, either because we encountered an amp
         * or because we reached the end of the message body.
         *
         * First, unescape the value.
         *
         * Then invoke handlers. We will bail out with PARSE_POST_STATE_ERROR,
         * unless the key-val gets correctly processed
         */
        url_unescape(val, val_len, val_escaped, PARSE_POST_BUF_SIZES);
        val_len = strlen(val_escaped);

        for(handler = list_head(post_handlers); handler != NULL;
            handler = list_item_next((void *)handler)) {
          if(handler->handler != NULL) {
            finish = handler->handler(key, key_len, val_escaped, val_len);
          }
          if(finish == HTTPD_SIMPLE_POST_HANDLER_ERROR) {
            state = PARSE_POST_STATE_ERROR;
            break;
          } else if(finish == HTTPD_SIMPLE_POST_HANDLER_OK) {
            /* Restart the state machine to expect the next pair */
            state = PARSE_POST_STATE_MORE;

            break;
          }
          /* Else, continue */
        }
      }
      break;
    case PARSE_POST_STATE_ERROR:
      /* If we entered the error state earlier, do nothing */
      return;
    default:
      break;
    }
  }
  //(char *buf, int buf_len, int last_chunk)
  PRINTF("Fim do Post! => chave=%s <-> valor=%s || length = %d || state = %ld\n", key, val, buf_len, state);
}
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
  if(s->request_type == REQUEST_TYPE_POST) {
	  PRINTF("***** METHOD POST *******\n");
	  //TODO ver como e que lidadmos com esta resposta
	  PRINTF("Return Code: %d\n", s->return_code);
    if(s->return_code == RETURN_CODE_OK) {
    	/* TODO: Neste caso temos que dar um novo sitio para redirecionar */
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s,
					  http_header_302,
					  (char *)s->response.content_type, //TODO: passar a char o content_type
					  NULL,
					  NULL
			  )
	  	  );
    } else if(s->return_code == RETURN_CODE_LR) {
      PT_WAIT_THREAD(
    		  &s->outputpt,
			  send_headers(
					  s, http_header_411,
					  (char *)s->response.content_type,
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
					  (char *)s->response.content_type,
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
					  (char *)s->response.content_type,
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
					  (char *)s->response.content_type,
					  NULL,
					  http_header_con_close
				  )
			  );
      PT_WAIT_THREAD(&s->outputpt, send_string(s, "Bad Request\n"));
    }
  } else if(s->request_type == REQUEST_TYPE_GET) {
	  PRINTF("***** METHOD GET *******\n");
	  PRINTF("Content-Type: %d\n", s->response.content_type);
	  if(resourse_found) {
    	PRINTF("***** Resource Found!\n");
        PT_WAIT_THREAD(
        		&s->outputpt,
				send_headers(
						s,
						http_header_200,
						(char *)s->response.content_type,
						NULL,
						http_header_con_close
					)
				);
        PT_WAIT_THREAD(&s->outputpt,
                       enqueue_chunk(
                    		   s,
							   1,
							   s->response.buf
						   )
					   );

	  }else {
		PT_WAIT_THREAD(
				&s->outputpt,
				send_headers(
						s,
						http_header_404,
						(char *)s->response.content_type,
						NULL,
						http_header_con_close
					)
				);
		/*PT_WAIT_THREAD(&s->outputpt,
					 send_string(s, NOT_FOUND));*/
		uip_close();
		PT_EXIT(&s->outputpt);
	  }
  }
  PSOCK_CLOSE(&s->sout);
  PT_END(&s->outputpt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_input(httpd_state *s))
{
  PSOCK_BEGIN(&s->sin);

  PSOCK_READTO(&s->sin, ISO_space);
  if(strncasecmp(s->inputbuf, http_get, 4) == 0) {
    s->request_type = REQUEST_TYPE_GET;
    PSOCK_READTO(&s->sin, ISO_space);

    if(s->inputbuf[0] != ISO_slash) {
      PSOCK_CLOSE_EXIT(&s->sin);
    }

    // copy uri
    s->uri_len = PSOCK_DATALEN(&s->sin) - 1; // we remove the space at the end
    memcpy(&s->uri, &s->inputbuf, s->uri_len);

  } else if(strncasecmp(s->inputbuf, http_post, 5) == 0) {
	  PRINTF("***** handle_input: POST *******\n");
    s->request_type = REQUEST_TYPE_POST;
    PSOCK_READTO(&s->sin, ISO_space);

    if(s->inputbuf[0] != ISO_slash) {
      PSOCK_CLOSE_EXIT(&s->sin);
    }

    s->inputbuf[PSOCK_DATALEN(&s->sin) - 1] = 0;


    /* POST: Read out the rest of the line and ignore it */
    PSOCK_READTO(&s->sin, ISO_nl);

    /*
     * Start parsing headers. We look for Content-Length and ignore everything
     * else until we hit the start of the message body.
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

    if(s->return_code == RETURN_CODE_OK) {
      /* Acceptable Content Length. Try to obtain a lock */
      lock_obtain(s);

      if(lock == s) {
        state = PARSE_POST_STATE_INIT;
      } else {
    	  PRINTF("Entrei aqui codigo service una\n");
        s->return_code = RETURN_CODE_SU;
      }
    }

    /* Parse the message body, unless we have detected an error. */
    while(s->content_length > 0 && lock == s &&
          s->return_code == RETURN_CODE_OK) {
      PSOCK_READBUF_LEN(&s->sin, s->content_length);

      s->content_length -= PSOCK_DATALEN(&s->sin);

      /* Parse the message body */
      parse_post_request_chunk(s->inputbuf, PSOCK_DATALEN(&s->sin),
                               (s->content_length == 0));
      s->return_code = RETURN_CODE_OK;


      if(state == PARSE_POST_STATE_ERROR) {
        /* Could not parse: Bad Request and stop parsing */
        s->return_code = RETURN_CODE_BR;
      }
    }

    /*
     * Done. If our return code is OK but the state machine is not in
     * STATE_MORE, it means that the message body ended half-way reading a key
     * or value. Set 'Bad Request'
     */
    // TODO
    if(s->return_code == RETURN_CODE_OK && state != PARSE_POST_STATE_MORE) {
    	PRINTF("Bad Request: Line 573\n");
      s->return_code = RETURN_CODE_BR;
    }

    lock_release(s);
  } else if(strncasecmp(s->inputbuf, http_put, 4) == 0){

	  PRINTF("passei por aqui\n");

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
	/*typedef int (*service_callback_t)(void *request, void *response,
	                                  uint8_t *buffer, uint16_t preferred_size,
	                                  int32_t *offset);*/
	int res_found = service_cbk(s, &s->response, (uint8_t *)&s->response.buf, 0, 0);

	/*typedef int (*service_callback_t)(void *request, void *response,
	                                  uint8_t *buffer, uint16_t preferred_size,
	                                  int32_t *offset);*/

//	PRINTF("\n||||||||||||||||||||||||||||||||||||||||||||||||||||||| Resource Found: %d\n", res_found);
//	PRINTF("||||||||||||||||||||||||||||||||||||||||||||||||||||||| LEN: %d\n", s->response.blen);
//	int i;
//	for(i = 0; i < s->response.blen; i++) {
//		PRINTF("%c", s->response.buf[i]);
//	}
//	PRINTF("\n|||||||||||||||||||||||||||||||||||||||||||||||||||||||\n\n");
	PRINTF("(handle_connection) State: %d\n", s->return_code);
	handle_output(s, res_found);
  }
}

/*---------------------------------------------------------------------------*/
static void
appcall(void *state)
{
  httpd_state *s = (httpd_state *)state;

  int status_code = 1;

  if(uip_closed() || uip_aborted() || uip_timedout()) {
	if(s != NULL) {
		s->blen = 0;
		s->tmp_buf_len = 0;
		s->response.blen = 0;
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

  //list_add(pages_list, &http_index_page);
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
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    appcall(data);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
