#include "http.h"

void example(const Request *request, Response *response) {
    set_response_content(response, "{\"hello\":\"world\"}", APPLICATION_JSON);
}

void indexhtml(const Request *request, Response *response) {
    set_response_file(response, "bin/index.html", TEXT_HTML);
}

void indexjs(const Request *request, Response *response) {
    set_response_file(response, "bin/index.js", TEXT_JS);
}

void indexcss(const Request *request, Response *response) {
    set_response_file(response, "bin/index.css", TEXT_CSS);
}

int main()
{
    int port = 8080;
    Server *srv = malloc(sizeof(Server));
    create_server(srv);

    add_get(srv, "/", example);
    add_get(srv, "/example", example);
    add_get(srv, "/index.html", indexhtml);
    add_get(srv, "/index.js", indexjs);
    add_get(srv, "/index.css", indexcss);

    add_mount(srv, "/web", "bin");

    serve(srv, "127.0.0.1", &port);
    return 0;
}
