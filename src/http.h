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
#include <dirent.h>
#include "colors.h"

#define PORT 8080            // default server port
#define BUFFER_SIZE BUFSIZ   // default server buffersize
#define MAX_BODY_SIZE BUFSIZ // default response body size
#define DEBUG                // DEBUG for debug messages

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

//----------------------- REQUEST METHODS -----------------------------------
#define GET "GET"       // get
#define POST "POST"     // post
#define PUT "PUT"       // put
#define PATCH "PATCH"   // patch
#define DELETE "DELETE" // delete
//---------------------------------------------------------------------------
#define HTTP11 "HTTP/1.1" // http version

#define HTTP_OK "200 OK" // HTTP 200 OK response

#define MAX_HEADERS 100 // MAX_HEADERS

//---------------------- REQUEST LINE STRUCTURE -----------------------------------
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

//--------------------- REQUEST STRUCTURE -----------------------------------
typedef struct Request {
    RequestLine requestline;     // request first line
    Header headers[MAX_HEADERS]; // headers array
    int headerCount;             // header counter
    char *body;                  // request body
} Request;
//---------------------------------------------------------------------------

//--------------------- RESPONSE STRUCTURE ----------------------------------
typedef struct Response {
    int status_code;                // status code
    char *reason_phrase;            // reason phrase
    char headers[MAX_HEADERS][256]; // headers array
    int headerCout;                 // headers counter
    char body[MAX_BODY_SIZE];       // response body
} Response;
//---------------------------------------------------------------------------

//------------------- HANDLER STRUCTURE -------------------------------------
typedef struct Handler {
    char *path;                                  // request path
    void (*handle)(const Request *, Response *); // request function
} Handler;
//---------------------------------------------------------------------------

typedef struct mount_point {
    char *path;
    char *webpath;
    char *type;
} mount_point;

//------------------------- SERVER STRUCTURE --------------------------------
typedef struct Server {
    int server_socket;              // server socket
    int reuse_addr;                 // enable reuse addess
    struct sockaddr_in server_addr; // server address
    Handler *handlers;              // handlers array
    int handlers_size;              // size of handlers array
    mount_point *mount_points;
    int mount_points_size;
} Server;
//---------------------------------------------------------------------------

//----------------------- HANDLE PARAMETER STRUCTURE ------------------------
typedef struct handle_parameter {
    int client_fd; // client socket
    Server *srv;   // server structure
} handle_parameter;
//---------------------------------------------------------------------------

//-------------------- INIT DEFAULT RESPONSSE DATA FUNC ---------------------
void init_response(Response *response) {
    response->status_code = 200;    // Default to OK
    response->reason_phrase = "OK"; // Default reason phrase
    response->headerCout = 0;       // No headers
    response->body[0] = '\0';       // Empty body
}
//---------------------------------------------------------------------------

//-------------------- FREE RESPONSE FUNC -----------------------------------
void free_response(Response *response) {
    free(response->reason_phrase);
    free(response);
}
//---------------------------------------------------------------------------

//-------------------- ADD HEADER TO RESPONSE FUNC --------------------------
void add_header(Response *response, const char *key, const char *value) {
    if (response->headerCout < MAX_HEADERS) {
        snprintf(response->headers[response->headerCout], sizeof(response->headers[response->headerCout]), "%s: %s", key, value);
        response->headerCout++;
    } else {
        fprintf(stderr, "Max headers limit reached\n");
    }
}
//---------------------------------------------------------------------------

//------------------- SET RESPONSE BODY FUNC --------------------------------
void set_body(Response *response, const char *body) {
    strncpy(response->body, body, MAX_BODY_SIZE-1);
    response->body[MAX_BODY_SIZE-1] = '\0';
}
//---------------------------------------------------------------------------

//------------------- SET RESPONSE STATUS CODE ------------------------------
void set_status(Response *response, int status_code, char *reason_phrase) {
    response->status_code = status_code;
    response->reason_phrase = reason_phrase;
}
//---------------------------------------------------------------------------

//------------------ GENERATE RESPONSE STRING FUNC --------------------------
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
//---------------------------------------------------------------------------

//----------- DELETE ALL WHITESPACES FROM STRING FUNC -----------------------
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
//---------------------------------------------------------------------------

//--------------- PARSE FIRST LINE IN REQUEST FUNC --------------------------
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
//---------------------------------------------------------------------------

//------------------- PARSE REQUEST HEADERS FUNC ----------------------------
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
//---------------------------------------------------------------------------

//------------------ PARSE REQUEST FUNC -------------------------------------
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
//---------------------------------------------------------------------------

//----------------- DELETE REQUEST FUNC -------------------------------------
void free_request(Request *request) {
    if (request) {
        if (request->body) free(request->body);
        free(request);
    }
}
//---------------------------------------------------------------------------

//--------------------- PRINT REQUEST DATA FUNC -----------------------------
void print_request(Request *request) {
    printf("Method: %s\nPath: %s\nVersion: %s\n", request->requestline.method, request->requestline.path, request->requestline.version);
    for (int i = 0; i < request->headerCount; i++) {
        printf("Header: %s -> %s\n", request->headers[i].key, request->headers[i].value);
    }
    if (request->body) {
        printf("body: %s\n", request->body);
    }
}
//---------------------------------------------------------------------------

//----------------- FUNCTION TO SET RESPONSE CONTENT ------------------------
void set_response_content(Response *response, const char *content, const char *content_type) {
    add_header(response, "Content-Type", content_type);
    char content_length[20];
    sprintf(content_length, "%ld", strlen(content));
    add_header(response, "Content-Length", content_length);
    set_body(response, content);
}
//---------------------------------------------------------------------------

//-------------- set response file function ---------------------------------
void set_response_file(Response *response, char *path, char *type) {
    FILE *file = fopen(path, "r");

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char filestr[size+1];
    fread(filestr, 1, size, file);
    filestr[size] = '\0';
    set_response_content(response, filestr, type);

    fclose(file);
}
//---------------------------------------------------------------------------

//------------------------- add get handler function ------------------------
void add_get(Server *srv, char *path, void (*handler)(const Request *request, Response *response)) {
    // http->funcs = (void(**)(int, HTTPreq*))realloc(http->funcs,
    //                                                    http->cap * (sizeof (void(*)(int, HTTPreq*))));
    srv->handlers[srv->handlers_size].path = malloc(sizeof(path));
    srv->handlers[srv->handlers_size].path = path;
    srv->handlers[srv->handlers_size].handle = malloc(sizeof(handler));
    srv->handlers[srv->handlers_size].handle = handler;
    srv->handlers_size++;
    // Request *req = NULL;
    // Response *response = NULL;
    // (*handler)(req, response);
}
//---------------------------------------------------------------------------

//--------------------- add mount function ----------------------------------
void add_mount(Server *srv, char *webpath, char *path) {
    DIR *dir;
    struct dirent *dir_iterator;
    dir = opendir(path);
    if (dir) {
        while ((dir_iterator = readdir(dir)) != NULL) {
            if (!strcmp(dir_iterator->d_name, ".") || !strcmp(dir_iterator->d_name, "..")) {
                continue;
            }
            char filepath[1024];
            strcpy(filepath, path);
            strcat(filepath, "/");
            strcat(filepath, dir_iterator->d_name);
            filepath[strlen(filepath)] = '\0';
            srv->mount_points[srv->mount_points_size].path = malloc(sizeof(filepath));
            srv->mount_points[srv->mount_points_size].path = filepath;
            char filewebpath[1024];
            strcpy(filewebpath, webpath);
            strcat(filewebpath, "/");
            strcat(filewebpath, dir_iterator->d_name);
            srv->mount_points[srv->mount_points_size].webpath = malloc(sizeof(filewebpath));
            srv->mount_points[srv->mount_points_size].webpath = filewebpath;
            char filetype[1024];
            if (strstr(dir_iterator->d_name, ".html")) {
                strcpy(filetype, TEXT_HTML);
                filetype[strlen(filetype)] = '\0';
            }
            if (strstr(dir_iterator->d_name, ".js")) {
                strcpy(filetype, TEXT_JS);
                filetype[strlen(filetype)] = '\0';
            }
            if (strstr(dir_iterator->d_name, ".css")) {
                strcpy(filetype, TEXT_CSS);
                filetype[strlen(filetype)] = '\0';
            }
            srv->mount_points[srv->mount_points_size].type = malloc(sizeof(filetype));
            srv->mount_points[srv->mount_points_size].type = filetype;
            srv->mount_points_size++;
            printf("%s\n", dir_iterator->d_name);
        }
        closedir(dir);
    }
}
//---------------------------------------------------------------------------

//------------------- HANDLE CLIENT CONNECTION FUNC -------------------------
void handle_client(void *arg) {
    handle_parameter *parameter = arg;
    int client_fd = parameter->client_fd;
    char *buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));

    // ssize_t bytes_received = 
    recv(client_fd, buffer, BUFFER_SIZE, 0);

    Request *req = parse_request(buffer);
#ifdef DEBUG
    printf("%s%s%s - %s%s%s\n", YELLOW, req->requestline.method, RESET, GREEN, req->requestline.path, RESET);
#endif // DEBUG

    Response *response = malloc(sizeof(Response));
    init_response(response);

    // set_status(&response, 200, "OK");
    // set_response_content(&response, "[{\"id\":0,\"username\":\"tola\",\"email\":\"tolamironcenko@icloud.com\",\"group\":\"root\",\"is_superuser\":true,\"password\":\"2808\"}]", APPLICATION_JSON);

    bool found_path = false;
    for (int i = 0; i < parameter->srv->handlers_size; i++) {
        if (!strcmp(req->requestline.path, parameter->srv->handlers[i].path) && !strcmp(req->requestline.method, GET)) {
            parameter->srv->handlers[i].handle(req, response);
            found_path = true;
        }
    }
    if (!found_path) {
        for (int i = 0; i < parameter->srv->mount_points_size; i++) {
            if (!strcmp(req->requestline.path, parameter->srv->mount_points[i].webpath) && !strcmp(req->requestline.method, GET)) {
                set_response_file(response, parameter->srv->mount_points[i].path, parameter->srv->mount_points[i].type);
                found_path = true;
            }
        }
    }
    if (!found_path) {
        set_status(response, 404, "Not Found");
        set_response_content(response, "{\"ERROR\":\"404 Not Found\"}", APPLICATION_JSON);
    }

    char response_string[4096] = {0};
    generate_response(response, response_string, sizeof(response_string));

    send(client_fd, response_string, sizeof(response_string), 0);
    close(client_fd);
    free(arg);
    free(buffer);
    // free_response(response);
    free_request(req);
}
//---------------------------------------------------------------------------

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
    srv->handlers = (Handler *)malloc(sizeof(Handler));
    srv->handlers_size = 0;
    srv->mount_points = (mount_point *)malloc(sizeof(mount_point));
    srv->mount_points_size = 0;
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

    printf("%d\n", srv->mount_points_size);
    for (int i = 0; i < srv->mount_points_size; i++) {
        printf("%s - %s - %s\n", srv->mount_points[i].path, srv->mount_points[i].webpath, srv->mount_points[i].type);
    }

    printf("%sServer listening on host %s%s%s port %s%d\n%s", UGREEN, UYELLOW, address, UGREEN, UYELLOW, PORT, RESET);
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

        handle_parameter *arg = malloc(sizeof(handle_parameter));
        arg->client_fd = *client_fd;
        arg->srv = srv;
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, (void*)handle_client, (void*)arg);
        pthread_detach(thread_id);
    }

    return close(srv->server_socket);
}
//---------------------------------------------------------------------------

#endif // HTTP_H
