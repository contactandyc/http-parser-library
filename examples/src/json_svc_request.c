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
