#include "server/WebServer.h"

int main() {
    WebServer server(1316,
                     3,
                     TimerType::Loop,
                     10000,
                     false,
                     3306,
                     "root",
                     "123456",
                     "webserver",
                     12,
                     6,
                     false,
                     0,
                     1024);
    server.Start();
}
