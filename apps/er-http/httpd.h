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
 * @file     httpd.h
 * @brief    Contains the Definitions of the HTTP Server
 * @version  1.0
 * @date     20 May. 2017
 * @author   Tiago Costa & Ricardo Jesus & Claudio Prates
 *
 **/

/** @defgroup HTTP HTTP
 * @{
 */

/** @addtogroup Server
 * @{
 */

#ifndef HTTPD_SIMPLE_H_
#define HTTPD_SIMPLE_H_

#include "process.h"
#include "error_codes.h"

/*---------------------------------------------------------------------------*/
#define ISO_nl									0x0A /*!< New Line */
#define ISO_space								0x20 /*!< Space */
#define ISO_slash								0x2F /*!< / */
#define ISO_amp									0x26 /*!< & */
#define ISO_semi_colon							0x3B /*!< ; */
#define ISO_column								0x3A /*!< : */
#define ISO_equal								0x3D /*!< = */
#define ISO_null_term							0x00 /*!< 0 */
/*---------------------------------------------------------------------------*/
/* It SHOULD be higher the 15 to avoid retries from requesters because
 * uIP close the connection after 15~ seconds after no activity. */
#define ABORT_CONN_TIMEOUT                      20 /*!< Abourt Connection Timeout */
/*---------------------------------------------------------------------------*/
// POST key max bytes -> After parsed. Count with null terminator
#define POST_PARAMS_KEY_MAX_LEN					11 /*!< Key Max Size */
// POST val max bytes -> After parsed. Count with null terminator
#define POST_PARAMS_VAL_MAX_LEN					11 /*!< Val Max Size */
/*---------------------------------------------------------------------------*/
#define CONNS                2 					/*!< Max Connections */
#define CONTENT_LENGTH_MAX   256				/*!< Max Content-Length */
#define STATE_WAITING        0x1				/*!< Waiting */
#define STATE_OUTPUT         0x2				/*!< Ouput */
#define STATE_PROCESSED      0x4				/*!< Processed */
#define STATE_RES_FOUND      0x8				/*!< Found Resource */
#define STATE_IGNORE         0x10 				/*!< Ignore: Useful when waiting for CoAP responses */
#define IPADDR_BUF_LEN       64					/*!< Buffer Size */
/*---------------------------------------------------------------------------*/
/**
 * HTTP Status Headers
 */
#define HTTP_200_OK                "HTTP/1.0 200 OK\r\n"						/*!< Status 200 */
/* NOTE: Has no direct mapping to CoAP status codes */
#define HTTP_302_FO                "HTTP/1.0 302 Found\r\n"						/*!< Status 302 */
#define HTTP_400_BR                "HTTP/1.0 400 Bad Request\r\n"				/*!< Status 400 */
#define HTTP_403_FB                "HTTP/1.0 403 Forbidden\r\n"					/*!< Status 403 */
#define HTTP_404_NF                "HTTP/1.0 404 Not Found\r\n"					/*!< Status 404 */
#define HTTP_405_MNA               "HTTP/1.0 405 Method Not Allowed\r\n"		/*!< Status 405 */	
#define HTTP_411_LR                "HTTP/1.0 411 Length Required\r\n"			/*!< Status 411 */
#define HTTP_413_TL                "HTTP/1.0 413 Request Entity Too Large\r\n"	/*!< Status 413 */
#define HTTP_500_ISR               "HTTP/1.0 500 Internal Server Error\r\n"		/*!< Status 500 */
#define HTTP_503_SU                "HTTP/1.0 503 Service Unavailable\r\n"		/*!< Status 503 */
#define HTTP_NOT_MAPPED_OR_UNKNOWN "HTTP/1.0 998 Not Mapped or Unknown\r\n"		/*!< Status 998 */
#define HTTP_DUMMY_STATUS          "HTTP/1.0 999 Dummy Status\r\n"				/*!< Status 999 */
/*---------------------------------------------------------------------------*/
#define CONN_CLOSE                         "Connection: close\r\n"				/*!< String for the Connection Close header */
#define HTTP_CONTENT_LENGTH_TEMPLATE       "Content-Length:%s\r\n"				/*!< String for the Content-Length header */
/*---------------------------------------------------------------------------*/	
#define HTTP_CONTENT_TYPE_TEXT_PLAIN            "text/plain"					/*!< String for the Content-Type: text/plain header */
#define HTTP_CONTENT_TYPE_TEXT_HTML             "text/html"						/*!< String for the Content-Type: text/html header */
#define HTTP_CONTENT_TYPE_APP_JSON              "application/json"				/*!< String for the Content-Type: application/json header */
#define HTTP_CONTENT_TYPE_TEXT_XML              "text/xml"						/*!< String for the Content-Type: text/xml header */
#define HTTP_CONTENT_TYPE_APP_XML               "application/xml"				/*!< String for the Content-Type: application/xml header */
#define HTTP_DUMMY_CONTENT_TYPE                 "dummy/type"					/*!< String for the Content-Type: dummy/type header */
/*---------------------------------------------------------------------------*/
static const char http_content_type_txt_plain[] =             HTTP_CONTENT_TYPE_TEXT_PLAIN; /*!< String for the Content-Type: text/plain */
static const char http_content_type_txt_html[] =              HTTP_CONTENT_TYPE_TEXT_HTML; /*!< String for the Content-Type: text/html */
static const char http_content_type_app_json[] =              HTTP_CONTENT_TYPE_APP_JSON; /*!< String for the Content-Type: application/json header */
static const char http_content_type_txt_xml[] =               HTTP_CONTENT_TYPE_TEXT_XML; /*!< String for the Content-Type: text/xml header */
static const char http_content_type_app_xml[] =               HTTP_CONTENT_TYPE_APP_XML; /*!< String for the Content-Type: application/xml header */
static const char http_content_type_dummy[] =                 HTTP_DUMMY_CONTENT_TYPE; /*!< String for the Content-Type: dummy/type header */
/*---------------------------------------------------------------------------*/
/* Page template */
static const char http_header_200[] = HTTP_200_OK; 					/*!< String for the Status: HTTP_200_OK */
/* NOTE: Has no direct mapping to CoAP status codes */
static const char http_header_302[] = HTTP_302_FO; 					/*!< String for the Status:HTTP_302_FO */
static const char http_header_400[] = HTTP_400_BR; 					/*!< String for the Status: HTTP_400_BR */
static const char http_header_403[] = HTTP_403_FB; 					/*!< String for the Status: HTTP_403_FB */
static const char http_header_404[] = HTTP_404_NF; 					/*!< String for the Status: HTTP_404_NF */
static const char http_header_405[] = HTTP_405_MNA; 				/*!< String for the Status: HTTP_405_MNA */
static const char http_header_411[] = HTTP_411_LR; 					/*!< String for the Status: HTTP_411_LR */
static const char http_header_413[] = HTTP_413_TL; 					/*!< String for the Status: HTTP_413_TL */
static const char http_header_500[] = HTTP_500_ISR; 				/*!< String for the Status: HTTP_500_ISR */
static const char http_header_503[] = HTTP_503_SU; 					/*!< String for the Status: HTTP_503_SU */
static const char http_header_998[] = HTTP_NOT_MAPPED_OR_UNKNOWN; 	/*!< String for the Status: HTTP_NOT_MAPPED_OR_UNKNOWN */
static const char http_header_999[] = HTTP_DUMMY_STATUS; 			/*!< String for the Status: HTTP_DUMMY_STATUS */
/*---------------------------------------------------------------------------*/
static const char http_header_content_legth[] = HTTP_CONTENT_LENGTH_TEMPLATE; /*!< Template of Content-Length - (Content-Length:%s) */
/*---------------------------------------------------------------------------*/

/* HTTP response codes */
/* Mapping to CoAP - https://tools.ietf.org/id/draft-ietf-core-http-mapping-07.html */
typedef enum {
  /* ------  2xx  ------ */
  OK_200 =                               200,        /*!< OK */

  /* ------  3xx  ------ */
  FOUND_302 =                            302,        /*!< Found - NOTE: Has no direct mapping to CoAP status codes */

  /* ------  4xx  ------ */
  BAD_REQUEST_400 =                      400,        /*!< Bad Request */
  FORBIDDEN_403 =                        403,        /*!< Forbidden */
  NOT_FOUND_404 =                        404,        /*!< Not Found */
  METHOD_NOT_ALLOWED_405 =               405,        /*!< Method Not Allowed */
  LENGTH_REQUIRED_411 =                  411,        /*!< Length Required */
  REQUEST_ENTITY_TL_413 =                413,        /*!< Request Entity Too Large */

  /* ------  5xx  ------ */
  INTERNAL_SERVER_ERROR_500 =            500,        /*!< Internal Server Error */
  SERVICE_UNAVAILABLE_503 =              503,        /*!< Service Unavailable */

  /* ------  Dummy  ------ */
  DUMMY_STATUS_999 =                     999         /*!< Used for unimplemented status codes and to help the debug */
} http_status_t;


// Event sent by CoAP client with node's response
extern process_event_t coap_client_event_new_response;

/**
 * @}
 */

/**
 * @}
 */

#endif /* HTTPD_SIMPLE_H_ */
