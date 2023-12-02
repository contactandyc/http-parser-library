/*
Copyright 2019 Andy Curtis

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _hpl_h
#define _hpl_h

#include <stdbool.h>
#include <uv.h>

#include "a-client-server-library/impl/llhttp.h"

#include "a-memory-library/aml_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hpl_header_s {
  uv_buf_t key;
  uv_buf_t value;
  struct hpl_header_s *next;
} hpl_header_t;

typedef struct {
  aml_pool_t *pool;

  void *data;

  hpl_header_t *headers;
  // only for servers
  uv_buf_t url;
  uv_buf_t body;

  size_t content_length;
  unsigned short http_major;
  unsigned short http_minor;

  // only for servers
  unsigned char method;

  // only for clients
  int status_code;

  int keep_alive;
  bool chunked;
} hpl_t;

typedef void (*on_http_cb)(hpl_t *lh);

/* init the http parser for parsing a service request.  The pool_size
   should be zero for default size or a user specified size. */
hpl_t *hpl_service_init(on_http_cb on_url, size_t pool_size);

/* init the http parser for parsing a client response.  The pool_size
   should be zero for default size or a user specified size. */
hpl_t *hpl_client_init(on_http_cb on_body, size_t pool_size);

/* set functions to handle chunk encoding.  on_chunk is typically the only
   function required.  Specify NULL if no callback is specified. */
void hpl_chunk(hpl_t *h, on_http_cb on_chunk,
                   on_http_cb on_chunk_encoding, on_http_cb on_chunk_complete);

/* make the http parser reusable */
void hpl_clear(hpl_t *h);

/* write data to the http parser */
bool hpl_data(hpl_t *h, const void *d, size_t len);

/* destroy the http parser */
void hpl_destroy(hpl_t *h);

#ifdef __cplusplus
}
#endif

#endif
