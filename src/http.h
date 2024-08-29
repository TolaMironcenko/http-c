#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#define PORT 8080            // default server port
#define BUFFER_SIZE BUFSIZ   // default server buffersize
#define MAX_BODY_SIZE BUFSIZ // default response body size

//--------- MIME TYPES ------------------------------------------------------
//----------- TEXT ----------------------------------------------------------
#define TEXT_PLAIN "text/plain" // plain text
#define TEXT_HTML "text/html"   // html
#define TEXT_CSS "text/css"     // css
#define TEXT_JS "text/js"       // javascript
#define TEXT_RTF "text/rtf"     // .rtf
#define TEXT_XML "text/xml"     // xml
#define TEXT_CSV "text/csv"     // csv
//---------------------------------------------------------------------------
//----------- APPLICATION ---------------------------------------------------
#define APPLICATION_JSON "application/json"         // json
#define APPLICATION_INDEX "application/index"       // index
#define APPLICATION_JSONSEQ "application/json-seq"  // jsonseq
#define APPLICATION_JSONPATH "application/jsonpath" // jsonpath
//---------------------------------------------------------------------------
//----------- IMAGE ---------------------------------------------------------
#define IMAGE_WEBP "image/webp"       // webp
#define IMAGE_SVG_XML "image/svg+xml" // svg
#define IMAGE_PNG "image/png"         // png
#define IMAGE_JPEG "image/jpeg"       // jpeg
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//----------------------- REQUEST TYPES -------------------------------------
#define GET "GET"       // get
#define POST "POST"     // post
#define PUT "PUT"       // put
#define PATCH "PATCH"   // patch
#define DELETE "DELETE" // delete
//---------------------------------------------------------------------------
#define HTTP11 "HTTP/1.1" // http version

#define HTTP_OK "200 OK"

//------------------------- SERVER STRUCTURE --------------------------------
typedef struct Server {
    int server_socket;              // server socket
    int reuse_addr;                 // enable reuse addess
    struct sockaddr_in server_addr; // server address
} Server;
//---------------------------------------------------------------------------

//---------------------- REQUEST STRUCTURE -----------------------------------
typedef struct RequestLine {
    char method[8];        // request method
    char path[256];        // request path
    char version[16];      // http version
} RequestLine;
//---------------------------------------------------------------------------

//-------------------- HEADER STRUCTURE -------------------------------------
typedef struct Header {
    char key[256];    // header name
    char value[1024]; // header value
} Header;
//---------------------------------------------------------------------------

#define MAX_HEADERS 100 // MAX_HEADERS

//--------------------- REQUEST STRUCTURE -----------------------------------
typedef struct Request {
    RequestLine requestline;     // request first line
    Header headers[MAX_HEADERS]; // headers array
    int headerCount;             // header counter
    char *body;                  // request body
} Request;
//---------------------------------------------------------------------------

//-------------------- RESPONSELINE STRUCTURE -------------------------------
// typedef struct ResponseLine { 
//     int status_code;
//     char version[16];
//     char *status;
// } ResponseLine;
//---------------------------------------------------------------------------

//--------------------- RESPONSE STRUCTURE ----------------------------------
typedef struct Response {
    int status_code;
    char *reason_phrase;
    char headers[MAX_HEADERS][256];
    int headerCout;
    char body[MAX_BODY_SIZE];
} Response;
//---------------------------------------------------------------------------

void init_response(Response *response) {
    response->status_code = 200;    // Default to OK
    response->reason_phrase = "OK"; // Default reason phrase
    response->headerCout = 0;       // No headers
    response->body[0] = '\0';       // Empty body
}

void add_header(Response *response, const char *key, const char *value) {
    if (response->headerCout < MAX_HEADERS) {
        snprintf(response->headers[response->headerCout], sizeof(response->headers[response->headerCout]), "%s: %s", key, value);
        response->headerCout++;
    } else {
        fprintf(stderr, "Max headers limit reached\n");
    }
}

void set_body(Response *response, const char *body) {
    strncpy(response->body, body, MAX_BODY_SIZE-1);
    response->body[MAX_BODY_SIZE-1] = '\0';
}

void set_status(Response *response, int status_code, const char *reason_phrase) {
    response->status_code = status_code;
    response->reason_phrase = reason_phrase;
}

void generate_response(Response *response, char *output, size_t size) {
    // Sratr with the status line
    snprintf(output, size, "HTTP/1.1 %d %s\r\n", response->status_code, response->reason_phrase);

    // Add headers
    for (int i = 0; i < response->headerCout; i++) {
        strncat(output, response->headers[i], size - strlen(output) - 1);
        strncat(output, "\r\n", size - strlen(output) - 1);
    }

    // Add a black line to separate headers from the body
    strncat(output, "\r\n", size - strlen(output) - 1);
    
    // Add body if it exists
    if (strlen(response->body) > 0) {
        strncat(output, response->body, size - strlen(output) - 1);
    }
}

char *trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while(isspace((unsigned char) *str)) str++;

    // All spaces?
    if (*str == 0) return str;

    end = str+strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) end--;

    // Write new null terminator
    *(end+1) = '\0';

    return str;
}

int parse_request_line(char *buffer, RequestLine *requestline) {
    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");

    if (!method || !path || !version) return -1;

    strncpy(requestline->method, method, sizeof(requestline->method)-1);
    strncpy(requestline->path, path, sizeof(requestline->path)-1);
    strncpy(requestline->version, version, sizeof(requestline->version));

    return 0;
}

int parse_headers(char *header_lines, Header *headers, int *headerCount) {
    char *line = strtok(header_lines, "\r\n");
    *headerCount = 0;

    while (line != NULL) {
        char *colon = strchr(line, ':');
        if (colon == NULL) return -1;
        
        *colon = '\0';
        char *key = trim_whitespace(line);
        char *value = trim_whitespace(colon + 1);

        strncpy(headers[*headerCount].key, key, sizeof(headers[*headerCount].key) - 1);
        strncpy(headers[*headerCount].value, value, sizeof(headers[*headerCount].value) - 1);
        (*headerCount)++;
        
        line = strtok(NULL, "\r\n");
    }

    return 0;
}

Request *parse_request(char *raw_request) {
    Request *request = malloc(sizeof(Request));
    if (request == NULL) return NULL;
    memset(request, 0, sizeof(Request));

    // Separate request line, headers, and body
    char *header_separator = "\r\n\r\n";
    char *header_end = strstr(raw_request, header_separator);
    if (!header_end) {
        free(request);
        return NULL;
    }

    char *headers_body = header_end + strlen(header_separator);
    *header_end = '\0'; // Terminate header section

    // Separate request line and headers
    char *line_end = strstr(raw_request, "\r\n");
    if (!line_end) {
        free(request);
        return NULL;
    }

    *line_end = '\0'; // Terminate request line
    char *header_lines = line_end + 2;

    // Parse the request line
    if (parse_request_line(raw_request, &request->requestline) < 0) {
        free(request);
        return NULL;
    }

    // Parse the headers
    if (parse_headers(header_lines, request->headers, &request->headerCount) < 0) {
        free(request);
        return NULL;
    }

    // Copy body if present
    if (headers_body && *headers_body) {
        request->body = strdup(headers_body);
    }

    return request;
}

void free_request(Request *request) {
    if (request) {
        if (request->body) free(request->body);
        free(request);
    }
}

void print_request(Request *request) {
    printf("Method: %s\nPath: %s\nVersion: %s\n", request->requestline.method, request->requestline.path, request->requestline.version);
    for (int i = 0; i < request->headerCount; i++) {
        printf("Header: %s -> %s\n", request->headers[i].key, request->headers[i].value);
    }
    if (request->body) {
        printf("body: %s\n", request->body);
    }
}

// char *get_req_method(char *buffer) {
//     return strtok(buffer, " ");
// }

// char *get_req_path(char *buffer) {
//     strtok(buffer, " ");
//     return strtok(NULL, "");
// }

void set_response_content(Response *response, const char *content, const char *content_type) {
    add_header(response, "Content-Type", content_type);
    char content_length[10];
    sprintf(content_length, "%ld", strlen(content));
    add_header(response, "Content-Length", content_length);
    set_body(response, content);
}

void handle_client(void *arg) {
    int client_fd = *((int*)arg);
    char *buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));

    // ssize_t bytes_received = 
    recv(client_fd, buffer, BUFFER_SIZE, 0);
    // printf("butes_received: %ld\ndata:\n%s\nreqtype: %s\nreqpath: %s\n", bytes_received, buffer, get_req_type(buffer), get_req_path(buffer));

    Request *req = parse_request(buffer);
    // print_request(req);
    free_request(req);

    Response response;
    init_response(&response);

    set_status(&response, 200, "OK");
    // add_header(&response, "Content-Type", APPLICATION_JSON);
    // add_header(&response, "Content-Length", "117");
    // set_body(&response, "[{\"id\":0,\"username\":\"tola\",\"email\":\"tolamironcenko@icloud.com\",\"group\":\"root\",\"is_superuser\":true,\"password\":\"2808\"}]");
    // printf("%ld", strlen("[{\"id\":0,\"username\":\"tola\",\"email\":\"tolamironcenko@icloud.com\",\"group\":\"root\",\"is_superuser\":true,\"password\":\"2808\"}]"));
    set_response_content(&response, "[{\"id\":0,\"username\":\"tola\",\"email\":\"tolamironcenko@icloud.com\",\"group\":\"root\",\"is_superuser\":true,\"password\":\"2808\"}]", APPLICATION_JSON);

    char response_string[4096] = {0};
    generate_response(&response, response_string, sizeof(response_string));

    send(client_fd, response_string, sizeof(response_string), 0);
    close(client_fd);
    free(arg);
    // free(buffer);
}

//---------------------- CREATE SERVER SOCKET FUNC --------------------------
int create_server(Server *srv) {
    // create server socket
    if ((srv->server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Create socket failed");
        exit(EXIT_FAILURE);
    }
    srv->reuse_addr = 1;
    if (setsockopt(srv->server_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&srv->reuse_addr, sizeof(srv->reuse_addr)) < 0) {
        perror("error SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }
    return 0;
}
//---------------------------------------------------------------------------

//--------------------- START SERVER FUNC -----------------------------------
int serve(Server *srv,  char *address, int *port) {
    // config socket
    srv->server_addr.sin_family = AF_INET;
    srv->server_addr.sin_addr.s_addr = inet_addr(address);
    srv->server_addr.sin_port = htons(PORT);

    // bind socket to port
    if (bind(srv->server_socket, (struct sockaddr *)&srv->server_addr, sizeof(srv->server_addr)) < 0)
    {
        perror("Bind socket to port error");
        exit(EXIT_FAILURE);
    }

    // listen for connections
    if (listen(srv->server_socket, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    while (true)
    {
        // client info
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));

        // accept client connection
        if ((*client_fd = accept(srv->server_socket, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
            perror("Accept client connection failed");
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, (void*)handle_client, (void*)client_fd);
        pthread_detach(thread_id);
    }

    return close(srv->server_socket);
}
//---------------------------------------------------------------------------

#endif // HTTP_H
