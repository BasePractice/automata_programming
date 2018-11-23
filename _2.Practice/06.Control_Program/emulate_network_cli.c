#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined(WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>

#define closesocket close
#endif

#include "sensor.h"

#pragma pack(1)
static int registers[LAST_SENSOR + 1];
#pragma pack(0)

static inline int get_network_error() {
#if defined(WIN32)
    return (int)WSAGetLastError();
#else
    return errno;
#endif
}

void send_registers(int s) {
    int ret;

    ret = (int) send(s, &registers, sizeof(registers), 0);
    if (ret != -1) {
        fprintf(stdout, "Sent %d bytes, need %lu bytes\n", ret, sizeof(registers));
    } else {
        fprintf(stderr, "Can't send registers %d\n", get_network_error());
    }
}

int main(int argc, char **argv) {
    int s;
    char *hostname = "127.0.0.1";
    char *filename = "registers.txt";
    FILE *fd = 0;


    if (argc >= 2)
        filename = argv[1];
    if (argc >= 3)
        hostname = argv[2];

    fd = fopen(filename, "r");
    if (fd == 0) {
        fprintf(stderr, "Registers file %s not found\n", filename);
        return EXIT_FAILURE;
    }
    fscanf(fd, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
           &registers[SENSOR_POWER_OFF],
           &registers[SENSOR_POINT_PRESENT],
           &registers[SENSOR_D1],
           &registers[SENSOR_D2],
           &registers[SENSOR_D3],
           &registers[SENSOR_D4],
           &registers[SENSOR_D5],
           &registers[SENSOR_M1],
           &registers[SENSOR_M2],
           &registers[SENSOR_M3],
           &registers[SENSOR_M4],
           &registers[SENSOR_S1],
           &registers[LAST_SENSOR]);
    fclose(fd);
#if defined(WIN32)
    WSADATA wsa_data;
    int ret;

    ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (ret != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", ret);
    }
#endif
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s > 0) {
        int ret;
        struct addrinfo *result = NULL, hints = {};

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        ret = getaddrinfo(hostname, NETWOR_EMULATE_PORT, &hints, &result);
        if (ret == 0) {
            ret = connect(s, result->ai_addr, (int) result->ai_addrlen);
            if (ret != -1) {
                send_registers(s);
            } else {
                fprintf(stderr, "Can't connect to hostname %s. Error: %d.\n", hostname, get_network_error());
            }
            freeaddrinfo(result);
        } else {
            fprintf(stderr, "Can't resolve hostname %s. Error: %d.\n", hostname, get_network_error());
        }
        closesocket(s);
    } else {
        fprintf(stderr, "Can't create socket. Error: %d.\n", get_network_error());
    }
#if defined(WIN32)
    WSACleanup();
#endif
    return EXIT_SUCCESS;
}

