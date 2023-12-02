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

#include "http-parser-library/hpl.h"
#include "a-memory-library/aml_alloc.h"

typedef struct {
  hpl_t http;

  on_http_cb on_chunk_encoding;
  on_http_cb on_chunk;
  on_http_cb on_chunk_complete;
  on_http_cb on_url;

  uv_buf_t status;
  uv_buf_t header_key;
  uv_buf_t header_value;
  int last_was_value;
  char *body_ptr;
  size_t body_len;
  bool body_terminated;
  hpl_header_t *tail;

  aml_pool_marker_t marker;

  llhttp_t parser;
  llhttp_settings_t settings;
} acs_llhttp_ext_t;

#define DEBUG_OUTPUT(at, length) ;

#define DEBUG_OUTPUTX(at, length)       \
    printf("%s\n", __FUNCTION__);       \
    printf("at(%p,%lu): ", at, length); \
    fwrite(at, length, 1, stdout);      \
    printf("\n")

/* llhttp methods */
static inline void save_current_header(acs_llhttp_ext_t *lh) {
  hpl_header_t *h = (hpl_header_t *)aml_pool_alloc(
      lh->http.pool, sizeof(hpl_header_t));
  h->key = lh->header_key;
  h->value = lh->header_value;
  h->next = NULL;
  if (!lh->tail)
    lh->http.headers = lh->tail = h;
  else {
    lh->tail->next = h;
    lh->tail = h;
  }
}

static inline void copy_buf(aml_pool_t *pool, uv_buf_t *b, const char *at,
                            size_t length) {
  b->base = (char *)aml_pool_ualloc(pool, length + 1);
  memcpy(b->base, at, length);
  b->base[length] = 0;
  b->len = length;
}

static inline void set_buf(aml_pool_t *pool, uv_buf_t *b, const char *at,
                           size_t length) {
  if (b->len > 0) {
    char *d = (char *)aml_pool_ualloc(pool, b->len + length + 1);
    memcpy(d, b->base, b->len);
    char *p = d + b->len;
    memcpy(p, at, length);
    p[length] = 0;
    b->base = d;
    b->len += length;
  } else {
    b->base = (char *)at;
    b->len = length;
  }
}

static int on_status(llhttp_t *parser, const char *at, size_t length) {
  DEBUG_OUTPUT(at, length);
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  set_buf(lh->http.pool, &lh->status, at, length);
  return 0;
}

static int on_header_key(llhttp_t *parser, const char *at, size_t length) {
  DEBUG_OUTPUT(at, length);
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  if (lh->last_was_value && lh->header_key.len > 0) {
    save_current_header(lh);
    lh->header_key.len = 0;
    lh->header_value.len = 0;
  }
  set_buf(lh->http.pool, &lh->header_key, at, length);
  lh->last_was_value = 0;
  lh->header_value.len = 0;
  return 0;
}

static int on_header_value(llhttp_t *parser, const char *at, size_t length) {
  DEBUG_OUTPUT(at, length);
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  set_buf(lh->http.pool, &lh->header_value, at, length);
  lh->last_was_value = 1;
  return 0;
}

static int on_url(llhttp_t *parser, const char *at, size_t length) {
  DEBUG_OUTPUT(at, length);
  /* for some reason, this doesn't work quite right without copying URL */
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  if (!lh->http.url.len)
    copy_buf(lh->http.pool, &lh->http.url, at, length);
  else
    set_buf(lh->http.pool, &lh->http.url, at, length);

  return 0;
}

static int on_body(llhttp_t *parser, const char *at, size_t length) {
  DEBUG_OUTPUT(at, length);
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  uv_buf_t *b = &lh->http.body;
  if (!b->len) {
    if (lh->http.content_length > length) {
      b->base =
          (char *)aml_pool_alloc(lh->http.pool, lh->http.content_length + 1);
      b->base[lh->http.content_length] = 0;
      b->len = lh->http.content_length;
      memcpy(b->base, at, length);
      lh->body_ptr = b->base + length;
      lh->body_len = lh->http.content_length - length;
      lh->body_terminated = true;
    } else {
      b->base = (char *)at;
      b->len = lh->http.content_length;
      lh->body_terminated = false;
      lh->body_len = 0;
    }
  } else {
    char *ptr = lh->body_ptr;
    if (length < lh->body_len) {
      memcpy(ptr, at, length);
      ptr += length;
      lh->body_len -= length;
      lh->body_ptr = ptr;
    } else {
      if (lh->body_len)
        memcpy(ptr, at, lh->body_len);
      lh->body_len = 0;
    }
  }
  return 0;
}

static int on_headers_complete(llhttp_t *parser) {
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  DEBUG_OUTPUT(lh->http.url.base, lh->http.url.len);

  if (lh->header_key.len > 0 && lh->header_value.len > 0)
    save_current_header(lh);

  hpl_header_t *head = lh->http.headers;
  while (head) {
    head->key.base[head->key.len] = 0;
    head->value.base[head->value.len] = 0;
    // printf("%s: %s\n", head->key.base, head->value.base);
    head = head->next;
  }

  lh->header_key.len = 0;
  lh->header_value.len = 0;
  lh->http.body.len = lh->body_len = 0;
  lh->http.content_length = parser->content_length;
  lh->http.http_major = parser->http_major;
  lh->http.http_minor = parser->http_minor;
  lh->http.method = parser->method;
  lh->http.status_code = parser->status_code;
  lh->http.keep_alive = llhttp_should_keep_alive(parser);

  lh->http.chunked = ((lh->parser.flags & F_CHUNKED) == 1) ? true : false;
  if (lh->http.chunked) {
    lh->on_chunk_encoding((hpl_t *)lh);
    aml_pool_save(lh->http.pool, &lh->marker);
  }
  return 0;
}

static int on_message_begin(llhttp_t *parser) { return 0; }

static int on_chunk_header(llhttp_t *parser) {
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  DEBUG_OUTPUT(lh->http.url.base, lh->http.url.len);
  lh->http.chunked = true;
  lh->http.content_length = parser->content_length;
  lh->body_len = 0;
  lh->body_ptr = NULL;
  lh->body_terminated = false;
  lh->http.body.len = 0;
  lh->http.body.base = NULL;
  aml_pool_restore(lh->http.pool, &lh->marker);

  if (parser->content_length == 0) {
    lh->on_chunk_complete((hpl_t *)lh);
    return 0;
  }
  return 0;
}

static int on_message_complete(llhttp_t *h) {
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)h->data;
  DEBUG_OUTPUT(lh->http.url.base, lh->http.url.len);
  if (lh->http.body.len) {
    char *ch = NULL;
    char c;
    if (!lh->body_terminated) {
      ch = lh->http.body.base + lh->http.body.len;
      c = *ch;
      *ch = 0;
    }
    if (!lh->http.chunked)
      lh->on_url((hpl_t *)lh);
    else if (lh->http.chunked)
      lh->on_chunk_complete((hpl_t *)lh);
    if (ch)
      *ch = c;
  } else {
    if (!lh->http.chunked)
      lh->on_url((hpl_t *)lh);
    else if (lh->http.chunked)
      lh->on_chunk_complete((hpl_t *)lh);
  }
  return 0;
}

static int on_chunk_complete(llhttp_t *parser) {
  acs_llhttp_ext_t *lh = (acs_llhttp_ext_t *)parser->data;
  DEBUG_OUTPUT(lh->http.body.base, lh->http.body.len);
  if (lh->http.body.len) {
    char *ch = NULL;
    char c;
    if (!lh->body_terminated) {
      ch = lh->http.body.base + lh->http.body.len;
      c = *ch;
      *ch = 0;
    }
    if (lh->http.chunked)
      lh->on_chunk((hpl_t *)lh);
    if (ch)
      *ch = c;
  }
  lh->http.body.len = 0;
  return 0;
}

static void on_http(hpl_t *h) {}

static hpl_t *hpl_init(size_t pool_size, bool client,
                               on_http_cb _on_url) {
  acs_llhttp_ext_t *h = (acs_llhttp_ext_t *)aml_malloc(sizeof(acs_llhttp_ext_t));
  memset(&(h->http), 0, sizeof(h->http));
  h->http.pool = aml_pool_init(pool_size == 0 ? 4096 : pool_size);
  h->on_url = _on_url ? _on_url : on_http;
  h->on_chunk_encoding = on_http;
  h->on_chunk = on_http;
  h->on_chunk_complete = on_http;
  h->status.len = 0;
  h->status.base = NULL;
  h->header_key.len = 0;
  h->header_key.base = NULL;
  h->header_value.len = 0;
  h->header_value.base = NULL;
  h->last_was_value = 0;
  h->body_len = 0;
  h->body_ptr = NULL;
  h->body_terminated = false;
  h->tail = NULL;
  memset(&h->parser, 0, sizeof(h->parser));
  memset(&h->settings, 0, sizeof(h->settings));

  h->settings.on_status = on_status;
  h->settings.on_header_field = on_header_key;
  h->settings.on_header_value = on_header_value;
  h->settings.on_headers_complete = on_headers_complete;
  h->settings.on_body = on_body;
  h->settings.on_message_begin = on_message_begin;
  h->settings.on_message_complete = on_message_complete;
  h->settings.on_chunk_header = on_chunk_header;
  h->settings.on_chunk_complete = on_chunk_complete;
  h->settings.on_url = on_url;

  llhttp_init(&h->parser, client ? HTTP_RESPONSE : HTTP_REQUEST, &h->settings);
  h->parser.data = h;

  return (hpl_t *)h;
}

hpl_t *hpl_service_init(on_http_cb on_url, size_t pool_size) {
  return hpl_init(pool_size, false, on_url);
}

hpl_t *hpl_client_init(on_http_cb on_body, size_t pool_size) {
  return hpl_init(pool_size, true, on_body);
}

void hpl_clear(hpl_t *h) {
  aml_pool_t *pool = h->pool;
  void *data = h->data;
  memset(h, 0, sizeof(*h));
  h->pool = pool;
  h->data = data;
  aml_pool_clear(pool);
  acs_llhttp_ext_t *hp = (acs_llhttp_ext_t *)h;
  hp->status.len = 0;
  hp->status.base = NULL;
  hp->header_key.len = 0;
  hp->header_key.base = NULL;
  hp->header_value.len = 0;
  hp->header_value.base = NULL;
  hp->last_was_value = 0;
  hp->body_len = 0;
  hp->body_ptr = NULL;
  hp->body_terminated = false;
  hp->tail = NULL;
}

bool hpl_data(hpl_t *h, const void *d, size_t len) {
  acs_llhttp_ext_t *hp = (acs_llhttp_ext_t *)h;
  return llhttp_execute(&hp->parser, (const char *)d, len) ? false : true;
}

void hpl_chunk(hpl_t *h, on_http_cb on_chunk,
                   on_http_cb on_chunk_encoding, on_http_cb on_chunk_complete) {
  acs_llhttp_ext_t *hp = (acs_llhttp_ext_t *)h;
  hp->on_chunk_encoding = on_chunk_encoding ? on_chunk_encoding : on_http;
  hp->on_chunk = on_chunk ? on_chunk : on_http;
  hp->on_chunk_complete = on_chunk_complete ? on_chunk_complete : on_http;
}

void hpl_destroy(hpl_t *h) {
  aml_pool_destroy(h->pool);
  aml_free(h);
}

#include "llhttp/llhttp.c"