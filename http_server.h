#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

// Based on code from:
// https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/tcp_server/ (official tcp server impl example)
// https://github.com/sysprogs/PicoHTTPServer/ (a full web app implementation including usage of access points, this is a much more stripped down version)

#include <string.h>
#include <stdio.h>
#include <lwip/err.h>
#include <lwip/pbuf.h>
#include <lwip/tcp.h>
#include <lwip/netif.h>
#include <lwip/ip4.h>

enum class ServerState
{
    None,
    DeskError,
    PreAlarm,
    Alarm,
    Login,
    Logout
};

class HTTPServer
{
private:
    int current_position = 0;
    char current_melody = '\0';
    char current_username[11] = {};
    ServerState state = ServerState::None;

    const char *set_error_state(const char *query)
    {
        state = ServerState::DeskError;
        return "{\"result\":\"success\"}";
    }

    const char *set_error_end_state(const char *query)
    {
        state = ServerState::None;
        return "{\"result\":\"success\"}";
    }

    const char *set_pre_alarm_state(const char *query)
    {
        state = ServerState::PreAlarm;
        return "{\"result\":\"success\"}";
    }

    const char *set_alarm_state(const char *query)
    {
        int position = -1;
        char melody = '\0';

        if (query && strlen(query) > 0)
        {
            // Find the substrings "position=" and "melody=" within the query string.
            // Use const_cast since strstr doesn't work with const char *, only char *
            char *positionStr = strstr(const_cast<char *>(query), "position=");
            char *melodyStr = strstr(const_cast<char *>(query), "melody=");

            if (positionStr)
            {
                positionStr += 9; // Skip "position="
                position = atoi(positionStr);
            }

            if (melodyStr)
            {
                melodyStr += 7; // Skip "melody="
                melody = *melodyStr;
            }
        }

        if (position > 0 && melody != '\0')
        {
            current_position = position;
            current_melody = melody;
            state = ServerState::Alarm;
            return "{\"result\":\"success\"}";
        }
        else
            return "{\"result\":\"error\",\"error\":\"invalid or missing parameters (position, melody) for alarm\"}";
    }

    const char *set_login_state(const char *query)
    {
        char *usernameStr = strstr(const_cast<char *>(query), "username=");

        if (usernameStr)
        {
            usernameStr += 9; // Skip "username="
            strncpy(current_username, usernameStr, sizeof(current_username) - 1);
            current_username[sizeof(current_username) - 1] = '\0';
            state = ServerState::Login;
            return "{\"result\":\"success\"}";
        }
        else
            return "{\"result\":\"error\",\"error\":\"username not provided\"}";
    }

    const char *set_logout_state(const char *query)
    {
        state = ServerState::Logout;
        return "{\"result\":\"success\"}";
    }

    void parse_http_request(const char *request, char *path, size_t pathSize, char *query, size_t querySize)
    {
        const char *pathStart = strchr(request, ' ') + 1;
        const char *queryStart = strchr(pathStart, '?'); // this is the "?" inside the URL, meaning there is a query in addition to the route
        const char *pathEnd = strchr(pathStart, ' ');

        size_t pathLength = (queryStart ? queryStart : pathEnd) - pathStart;
        strncpy(path, pathStart, pathLength < pathSize ? pathLength : pathSize - 1);

        if (queryStart)
            strncpy(query, queryStart + 1, pathEnd - queryStart - 1 < querySize ? pathEnd - queryStart - 1 : querySize - 1);
        else
            query[0] = '\0';
    }

    const char *match_route_and_handle(const char *path, const char *query)
    {
        struct Route
        {
            const char *path;
            const char *(HTTPServer::*method)(const char *query); // function pointer to a method in HTTPServer
        };
        
        // Note: This obviously isn't very secure since anyone with a browser or curl could ping these endpoints if they're in the same network.
        // An easy way to add security would be to use HTTPS, and require adding a secret key in the route (could even be the same keys as for the desk API),
        // the key could be defined in CMakeCache so it's not exposed to the remote git repository.
        // But this method makes testing much easier, delivery much faster, and we're not sending sensitive info to the Pico anyway...
        static const Route routes[] = {
            {"/api/error", &HTTPServer::set_error_state},
            {"/api/errend", &HTTPServer::set_error_end_state},
            {"/api/prealarm", &HTTPServer::set_pre_alarm_state},
            {"/api/alarm", &HTTPServer::set_alarm_state},
            {"/api/login", &HTTPServer::set_login_state},
            {"/api/logout", &HTTPServer::set_logout_state},
        };

        for (const auto &route : routes)
        {
            if (strncmp(path, route.path, strlen(route.path)) == 0)
            {
                if (route.method)
                    return (this->*route.method)(query);
            }
        }

        return "{\"result\":\"error\",\"error\":\"404 Not Found: Path does not exist\"}";
    }

    void handle_request(struct tcp_pcb *tpcb, const char *request)
    {
        constexpr size_t PATH_BUFFER_SIZE = 64, QUERY_BUFFER_SIZE = 64;
        char path[PATH_BUFFER_SIZE] = {}, query[QUERY_BUFFER_SIZE] = {};
        parse_http_request(request, path, PATH_BUFFER_SIZE, query, QUERY_BUFFER_SIZE);

        const char *responseBody = match_route_and_handle(path, query);
        char response[256];
        int responseLength = snprintf(response,
                                      sizeof(response),
                                      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s",
                                      strlen(responseBody),
                                      responseBody);

        tcp_write(tpcb, response, responseLength, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }

    static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
    {
        if (!p)
            return tcp_close(tpcb), ERR_OK;

        constexpr size_t REQUEST_BUFFER_SIZE = 128;
        char request[REQUEST_BUFFER_SIZE] = {};
        pbuf_copy_partial(p, request, sizeof(request) - 1, 0);
        pbuf_free(p);

        static_cast<HTTPServer *>(arg)->handle_request(tpcb, request);
        return ERR_OK;
    }

    static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
    {
        tcp_recv(newpcb, http_recv);
        tcp_arg(newpcb, arg);
        return ERR_OK;
    }

public:
    HTTPServer()
    {
        printf("HTTP server initialized\n");
    }

    void start()
    {
        struct tcp_pcb *pcb = tcp_new();
        if (!pcb)
            return;

        tcp_bind(pcb, IP_ADDR_ANY, 80);
        pcb = tcp_listen(pcb);
        tcp_accept(pcb, http_accept);
        tcp_arg(pcb, this);
    }

    ServerState get_state() const
    {
        return state;
    }

    int get_position()
    {
        return current_position;
    }

    char get_melody()
    {
        return current_melody;
    }

    char *get_username()
    {
        return current_username;
    }

    void clear_state()
    {
        state = ServerState::None;
    }
};

#endif // HTTP_SERVER_H
