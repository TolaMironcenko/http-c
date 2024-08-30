#include "http.h"

// #define PORT 8080

// void *handle_client(void *arg) {
//     printf("handle_client is work\n");
//     int client_fd = *((int*)arg);
//     // char *buffer = (char*)malloc(BUFSIZ * sizeof(char));
//     char buffer[12] = "hello world";
//     send(client_fd, buffer, 12, 0);
//     close(client_fd);
//     free(arg);
//     return NULL;
//     // free(buffer);
// }

void example(const Request *request, Response *response) {
    printf("example function\n");
    // set_response_content(response, "{\"hello\":\"world\"}", APPLICATION_JSON);
}

int main()
{
    int port = 8080;
    Server *srv = malloc(sizeof(Server));
    create_server(srv);

    add_get(srv, "/", example);

    serve(srv, "0.0.0.0", &port);
    return 0;
}
