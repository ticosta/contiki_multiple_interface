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
 * \file
 *         Header file for the HTTPD of the HTTP Server.
 * \author
 *
 */
/*---------------------------------------------------------------------------*/
#ifndef HTTPD_SIMPLE_H_
#define HTTPD_SIMPLE_H_

#include "process.h"
#include "error_codes.h"

/*---------------------------------------------------------------------------*/
#define ISO_nl									0x0A
#define ISO_space								0x20
#define ISO_slash								0x2F
#define ISO_amp									0x26
#define ISO_semi_colon							0x3B
#define ISO_column								0x3A
#define ISO_equal								0x3D
#define ISO_null_term							0x00
/*---------------------------------------------------------------------------*/
/* It SHOULD be higher the 15 to avoid retries from requesters because
 * uIP close the connection after 15~ seconds after no activity. */
#define ABORT_CONN_TIMEOUT                      20
/*---------------------------------------------------------------------------*/
// POST key max bytes -> After parsed. Count with null terminator
#define POST_PARAMS_KEY_MAX_LEN					11
// POST val max bytes -> After parsed. Count with null terminator
#define POST_PARAMS_VAL_MAX_LEN					11
/*---------------------------------------------------------------------------*/
#define CONNS                2
#define CONTENT_LENGTH_MAX   256
#define STATE_WAITING        0x1
#define STATE_OUTPUT         0x2
#define STATE_PROCESSED      0x4
#define STATE_RES_FOUND      0x8
#define STATE_IGNORE         0x10 /* Useful when waiting for CoAP responses */
#define IPADDR_BUF_LEN       64
/*---------------------------------------------------------------------------*/
/**
 * HTTP Status Headers
 */
#define HTTP_200_OK                "HTTP/1.0 200 OK\r\n"
/* NOTE: Has no direct mapping to CoAP status codes */
#define HTTP_302_FO                "HTTP/1.0 302 Found\r\n"
#define HTTP_400_BR                "HTTP/1.0 400 Bad Request\r\n"
#define HTTP_403_FB                "HTTP/1.0 403 Forbidden\r\n"
#define HTTP_404_NF                "HTTP/1.0 404 Not Found\r\n"
#define HTTP_405_MNA               "HTTP/1.0 405 Method Not Allowed\r\n"
#define HTTP_411_LR                "HTTP/1.0 411 Length Required\r\n"
#define HTTP_413_TL                "HTTP/1.0 413 Request Entity Too Large\r\n"
#define HTTP_500_ISR               "HTTP/1.0 500 Internal Server Error\r\n"
#define HTTP_503_SU                "HTTP/1.0 503 Service Unavailable\r\n"
#define HTTP_NOT_MAPPED_OR_UNKNOWN "HTTP/1.0 998 Not Mapped or Unknown\r\n"
#define HTTP_DUMMY_STATUS          "HTTP/1.0 999 Dummy Status\r\n"
/*---------------------------------------------------------------------------*/
#define CONN_CLOSE                         "Connection: close\r\n"
#define HTTP_CONTENT_LENGTH_TEMPLATE       "Content-Length:%s\r\n"
/*---------------------------------------------------------------------------*/
#define HTTP_CONTENT_TYPE_TEXT_PLAIN            "text/plain"
#define HTTP_CONTENT_TYPE_TEXT_HTML             "text/html"
#define HTTP_CONTENT_TYPE_APP_JSON              "application/json"
#define HTTP_CONTENT_TYPE_TEXT_XML              "text/xml"
#define HTTP_CONTENT_TYPE_APP_XML               "application/xml"
#define HTTP_DUMMY_CONTENT_TYPE                 "dummy/type"
/*---------------------------------------------------------------------------*/
static const char http_content_type_txt_plain[] =             HTTP_CONTENT_TYPE_TEXT_PLAIN;
static const char http_content_type_txt_html[] =              HTTP_CONTENT_TYPE_TEXT_HTML;
static const char http_content_type_app_json[] =              HTTP_CONTENT_TYPE_APP_JSON;
static const char http_content_type_txt_xml[] =               HTTP_CONTENT_TYPE_TEXT_XML;
static const char http_content_type_app_xml[] =               HTTP_CONTENT_TYPE_APP_XML;
static const char http_content_type_dummy[] =                 HTTP_DUMMY_CONTENT_TYPE;
/*---------------------------------------------------------------------------*/
/* Page template */
static const char http_header_200[] = HTTP_200_OK;
/* NOTE: Has no direct mapping to CoAP status codes */
static const char http_header_302[] = HTTP_302_FO;
static const char http_header_400[] = HTTP_400_BR;
static const char http_header_403[] = HTTP_403_FB;
static const char http_header_404[] = HTTP_404_NF;
static const char http_header_405[] = HTTP_405_MNA;
static const char http_header_411[] = HTTP_411_LR;
static const char http_header_413[] = HTTP_413_TL;
static const char http_header_500[] = HTTP_500_ISR;
static const char http_header_503[] = HTTP_503_SU;
static const char http_header_998[] = HTTP_NOT_MAPPED_OR_UNKNOWN;
static const char http_header_999[] = HTTP_DUMMY_STATUS;
/*---------------------------------------------------------------------------*/
static const char http_header_content_legth[] = HTTP_CONTENT_LENGTH_TEMPLATE;
/*---------------------------------------------------------------------------*/

/* HTTP response codes */
/* Mapping to CoAP - https://tools.ietf.org/id/draft-ietf-core-http-mapping-07.html */
typedef enum {
  /* ------  2xx  ------ */
  OK_200 =                               200,        /* OK */

  /* ------  3xx  ------ */
  FOUND_302 =                            302,        /* Found - NOTE: Has no direct mapping to CoAP status codes */

  /* ------  4xx  ------ */
  BAD_REQUEST_400 =                      400,        /* Bad Request */
  FORBIDDEN_403 =                        403,        /* Forbidden */
  NOT_FOUND_404 =                        404,        /* Not Found */
  METHOD_NOT_ALLOWED_405 =               405,        /* MEthod Not Allowed */
  LENGTH_REQUIRED_411 =                  411,        /* Length Required */
  REQUEST_ENTITY_TL_413 =                413,        /* Request Entity Too Large */

  /* ------  5xx  ------ */
  INTERNAL_SERVER_ERROR_500 =            500,        /* Internal Server Error */
  SERVICE_UNAVAILABLE_503 =              503,        /* Service Unavailable */

  /* ------  Dummy  ------ */
  DUMMY_STATUS_999 =                     999         /* Used for unimplemented status codes and to help the debug */
} http_status_t;


// Event sent by CoAP client with node's response
extern process_event_t coap_client_event_new_response;

#endif /* HTTPD_SIMPLE_H_ */
