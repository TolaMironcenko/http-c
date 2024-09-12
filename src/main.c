#include "http.h"

void example(const Request *request, Response *response) {
    set_response_content(response, "{\"hello\":\"world\"}", APPLICATION_JSON);
}

void get_token(const Request *request, Response *response) {
    const char *header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    const char *payload = "{\"sub\":\"1234567890\",\"name\":\"John Doe\",\"admin\":true}";
    char *token = generate_jwt(header, payload);
    // char rescontent[strlen(token)*2];
    // snprintf(rescontent, strlen(token)*2, "{\"token\":\"%s\"}", token);
    set_response_content(response, token, TEXT_PLAIN);
    // free(header);
    // free(payload);
    // free(token);
}

int main()
{
    int port = 8080;
    Server *srv = malloc(sizeof(Server));
    create_server(srv);

    // add_get(srv, "/", example);
    add_get(srv, "/example", example);
    add_get(srv, "/api/token", get_token);

    add_mount(srv, "/", "web");

    serve(srv, "127.0.0.1", &port);
    return 0;
}
