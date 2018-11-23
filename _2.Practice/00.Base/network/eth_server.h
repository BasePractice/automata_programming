#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_ETH_SERVER_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_ETH_SERVER_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include <errno.h>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>

#define closesocket(s)  close((s))

#endif

int socket_listen_server(const char *port);

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_ETH_SERVER_H
