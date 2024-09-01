#include "http.h"

void example(const Request *request, Response *response) {
    // printf("example function\n");
    set_response_content(response, "{\"hello\":\"world\"}", APPLICATION_JSON);
}

int main()
{
    int port = 8080;
    Server *srv = malloc(sizeof(Server));
    create_server(srv);

    add_get(srv, "/", example);
    add_get(srv, "/example", example);

    serve(srv, "0.0.0.0", &port);
    return 0;
}
