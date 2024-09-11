#include "http.h"

void example(const Request *request, Response *response) {
    set_response_content(response, "{\"hello\":\"world\"}", APPLICATION_JSON);
}

int main()
{
    int port = 8080;
    Server *srv = malloc(sizeof(Server));
    create_server(srv);

    // add_get(srv, "/", example);
    add_get(srv, "/example", example);

    add_mount(srv, "/", "web");

    serve(srv, "127.0.0.1", &port);
    return 0;
}
