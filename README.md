# Overview of HTTP Parser Library

The HTTP Parser Library (HPL) is a high-performance and versatile library written in C, designed for parsing HTTP requests and responses. It seamlessly integrates with `libuv` for efficient I/O operations and utilizes the `llhttp` library for robust HTTP parsing. Tailored for both client and server-side applications, HPL provides an intuitive API for handling HTTP headers, bodies, and various HTTP attributes. Combined with A Memory Library's memory management capabilities, HPL stands as a reliable solution for handling complex HTTP parsing tasks with minimal memory overhead.

## Dependencies

* [libuv](https://github.com/libuv/libuv) for handling I/O operations.
* [A memory library](https://github.com/contactandyc/a-memory-library) for efficient memory management.

## Installation

### Clone the library and change to the directory

```bash
git clone https://github.com/contactandyc/http-parser-library.git
cd http-parser-library
```

### Build and install the library

```bash
mkdir -p build
cd build
cmake ..
make
make install
```

## An Example


`examples/src/json_svc_request.c`:
```c
#include <stdio.h>
#include <string.h>
#include "http-parser-library/hpl.h"
#include "a-memory-library/aml_pool.h"

// Callback function to handle the parsed HTTP data
void on_body_callback(hpl_t *hpl) {
    // Print the URL
    printf("URL: %.*s\n", (int)hpl->url.len, hpl->url.base);

    // Print each header
    for (hpl_header_t *header = hpl->headers; header != NULL; header = header->next) {
        printf("Header: %.*s: %.*s\n", (int)header->key.len, header->key.base,
                                       (int)header->value.len, header->value.base);
    }

    // Print the body
    printf("Body: %.*s\n", (int)hpl->body.len, hpl->body.base);

    // Print other details
    printf("Content Length: %zu\n", hpl->content_length);
    printf("HTTP Version: %u.%u\n", hpl->http_major, hpl->http_minor);
}

int main(int argc, char *argv[]) {
    // Initialize a memory pool
    aml_pool_t *pool = aml_pool_init(1024);

    // Initialize HTTP parser for service request
    hpl_t *http_parser = hpl_service_init(on_body_callback, 16384);

    // Sample JSON POST request data.  The http_request is altered by
    // the hpl_data call so it must not be constant!
    char *http_request = aml_pool_strdup(pool, "POST /submit HTTP/1.1\r\n"
                               "Host: example.com\r\n"
                               "Content-Type: application/json\r\n"
                               "Content-Length: 28\r\n\r\n"
                               "{\"name\":\"John Doe\",\"age\":30}");

    // Write data to the HTTP parser
    if (!hpl_data(http_parser, http_request, strlen(http_request))) {
        fprintf(stderr, "Error parsing HTTP request.\n");
    }

    // Clean up
    hpl_destroy(http_parser);
    aml_pool_destroy(pool);

    return 0;
}
```

After installing http-parser-library
```build
$ cd examples
$ mkdir -p build
$ cd build
$ cmake ..
$ make
$ ./json_svc_request
URL: /submit
Header: Host: example.com
Header: Content-Type: application/json
Header: Content-Length: 28
Body: {"name":"John Doe","age":30}
Content Length: 28
HTTP Version: 1.1
```

## Core Functions

- **hpl_service_init**: Initializes the HTTP parser for parsing service requests.
- **hpl_client_init**: Initializes the HTTP parser for parsing client responses.
- **hpl_chunk**: Sets functions to handle chunk encoding in HTTP data.
- **hpl_clear**: Resets the HTTP parser for reuse.
- **hpl_data**: Writes data to the HTTP parser for parsing.
- **hpl_destroy**: Destroys the HTTP parser and frees resources.

## HTTP Parsing

- **Handling Headers**: Parses and stores HTTP headers.
- **URL Parsing**: Extracts and decodes the URL in service requests.
- **Body Parsing**: Manages HTTP body data for both requests and responses.

## Advanced Features

- **Content-Length Handling**: Manages the `Content-Length` attribute.
- **HTTP Version**: Captures the HTTP version from requests and responses.
- **Chunked Encoding**: Supports chunked transfer encoding.
- **Keep-Alive**: Manages keep-alive connections.
- **Status Code**: Retrieves the status code in client responses.

This README provides a comprehensive guide to using the HTTP Parser Library, including its dependencies, installation process, a basic usage example, and an overview of its core and advanced features.