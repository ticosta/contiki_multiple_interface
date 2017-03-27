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

/*---------------------------------------------------------------------------*/
#define ISO_nl									0x0A
#define ISO_space								0x20
#define ISO_slash								0x2F
#define ISO_amp									0x26
#define ISO_column								0x3A
#define ISO_equal								0x3D
#define ISO_null_term							0x00
/*---------------------------------------------------------------------------*/
// POST key max bytes -> After parsed
#define POST_PARAMS_KEY_MAX_LEN					10
// POST val max bytes -> After parsed
#define POST_PARAMS_VAL_MAX_LEN					10
/*---------------------------------------------------------------------------*/
/**
 * HTTP Status Headers
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
static const char http_content_type_html[] = "text/html";
static const char http_content_type_plain[] = "text/plain";
/*---------------------------------------------------------------------------*/
/* Page template */
 static const char http_header_200[] = HTTP_200_OK;
 static const char http_header_302[] = HTTP_302_FO;
 static const char http_header_400[] = HTTP_400_BR;
 static const char http_header_404[] = HTTP_404_NF;
 static const char http_header_411[] = HTTP_411_LR;
 static const char http_header_413[] = HTTP_413_TL;
 static const char http_header_503[] = HTTP_503_SU;
 /*---------------------------------------------------------------------------*/



#endif /* HTTPD_SIMPLE_H_ */
