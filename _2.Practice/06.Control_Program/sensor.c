#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include "sensor.h"

#if defined(_WIN32)
#include <windows.h>
#define ms_sleep(ms)   Sleep((ms))
#else

#include <pthread.h>
#include <unistd.h>

#define ms_sleep(ms)    usleep((ms) * 1000)
#endif

#pragma pack(1)
static int registers[LAST_SENSOR + 1];
static int sensor[LAST_SENSOR] = {0, 1};
#pragma pack(0)

static unsigned int step_timeout = 1000;

#if defined(NETWORK_EMULATE)

#include <eth_server.h>

#endif


struct EmulateRegisters {
#if defined(NETWORK_EMULATE)
#if defined(_WIN32)
    DWORD thread_id = 0;
    HANDLE thread_h;
    CRITICAL_SECTION access;
#else
    pthread_mutex_t access;
    pthread_t server_thread;
#endif
#elif defined(FILE_EMULATE)
    FILE *fd;
#endif
    bool running;
    bool started;
    size_t current_step;
} emulator = {.started = false, .running = true, .current_step = 0};

#if defined(NETWORK_EMULATE)

void *network_server_thread(void *parameter) {
    struct EmulateRegisters *pe = (struct EmulateRegisters *) parameter;

#if defined(_WIN32)
    InitializeCriticalSectionAndSpinCount(&emulator.access, 0x00000400);
#else
    pthread_mutex_init(&emulator.access, 0);
#endif
    int client_socket;
    struct sockaddr_in sock_addr_in;
    int sock_addr_len = sizeof(sock_addr_in);
    int listen_socket = socket_listen_server(NETWOR_EMULATE_PORT);
    while (pe->running) {
        client_socket = accept(listen_socket,
                               (struct sockaddr *) &sock_addr_in,
                               (socklen_t *) &sock_addr_len);
        if (client_socket == -1)
            break;

        {
            int read = (int) recv(client_socket, &registers, sizeof(registers), 0);
            if (read == sizeof(registers)) {
#if defined(_WIN32)
                EnterCriticalSection(&emulator.access);
#else
                pthread_mutex_lock(&emulator.access);
#endif
                memcpy(&sensor, &registers, sizeof(sensor));
                step_timeout = (unsigned int) registers[LAST_SENSOR];
#if defined(_WIN32)
                LeaveCriticalSection(&emulator.access);
#else
                pthread_mutex_unlock(&emulator.access);
#endif
            }
        }

        closesocket(client_socket);
    }
    closesocket(listen_socket);
#if defined(_WIN32)
    DeleteCriticalSection(&emulator.access);
    WSACleanup();
#else
    pthread_mutex_destroy(&emulator.access);
#endif
    return 0;
}
#elif defined(FILE_EMULATE)
static void update_file_registers(struct EmulateRegisters *em) {
    if (em->fd != 0) {

        if (feof(em->fd)) {
            fclose(em->fd);
            em->fd = 0;
            return;
        }

        fscanf(em->fd, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
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
        memcpy(&sensor, &registers, sizeof(sensor));
        step_timeout = (unsigned int) registers[LAST_SENSOR];
    }
}
#endif



static void simulate_update() {
    if (emulator.started == false) {
#if defined(NETWORK_EMULATE)
        memset(&emulator, 0, sizeof(emulator));
#if defined(_WIN32)
        emulator.thread_h = CreateThread(NULL, 0, network_server_thread, (void *) &emulator, 0, &emulator.thread_id);
        if (emulator.thread_h != INVALID_HANDLE_VALUE) {
            CloseHandle(emulator.thread_h);
        }
#else
        if (pthread_create(&emulator.server_thread, 0, network_server_thread, (void *) &emulator) == 0) {
            pthread_detach(emulator.server_thread);
        }
#endif
#elif defined(FILE_EMULATE)
        emulator.fd = fopen("registers.txt", "r");
#endif
        emulator.started = true;
        emulator.running = true;
    }

    if (step_timeout > 0) {
        fprintf(stdout, "[%05lu] Simulate step waiting %d ms\n", emulator.current_step, step_timeout);
        ms_sleep(step_timeout);
    }

#if defined(FILE_EMULATE)
    update_file_registers(&emulator);
#endif
    ++emulator.current_step;
}

static int get_value(enum Sensor id) {
    return sensor[id];
}

static char *get_name(enum Sensor sensor) {
    static char buf[20];
    switch (sensor) {
        case SENSOR_D1:
        case SENSOR_D2:
        case SENSOR_D3:
        case SENSOR_D4:
        case SENSOR_D5:
        case SENSOR_M1:
        case SENSOR_M2:
        case SENSOR_M3:
        case SENSOR_M4:
        case SENSOR_S1:
        case SENSOR_POWER_OFF:
            sprintf(buf, "Sensor %02d", sensor);
            return buf;
    }
}

void
emulator_sensor_init(struct SensorInterface *si) {
    si->get_name = get_name;
    si->get_value = get_value;
    si->simulate_update = simulate_update;
}
