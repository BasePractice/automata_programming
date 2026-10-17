/**
 * @file
 * Лекция 12. Драйвер для автомата, порождённого компилятором Takt.
 *
 * Модель `model/watchdog.takt` описывает сторожевой таймер; taktc порождает
 * из неё watchdog.h/watchdog.c. Порты модели не глобальные переменные —
 * порождённый код обращается к ним через callback'и `read_bit`/`write_bit`,
 * которые и реализует этот файл. Так автоматная логика отделена от драйвера
 * платформы: на стенде те же вызовы читают вывод микроконтроллера.
 *
 * Сам драйвер написан на ISO C90 и проходит общую проверку курса;
 * порождённый код — C99 (uint8_t, bool, строчные комментарии), поэтому собирается
 * отдельной целью со своими флагами (см. CMakeLists.txt).
 */

#include <stdio.h>
#include <stdlib.h>
#include "watchdog.h"

/* Сценарий: 1 — сигнал «система жива» пришёл, 0 — не пришёл.
   Первые четыре такта система отвечает, потом замолкает на семь тактов
   (LIMIT = 5, поэтому авария обязана возникнуть), затем оживает. */
static const int SCENARIO[] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1};
#define SCENARIO_LEN ((int)(sizeof(SCENARIO) / sizeof(SCENARIO[0])))

struct Bench {
    int tick;
    int alarm;
    int alarm_raised_at;
};

/* bit — номер разряда порта. Порты сторожевого таймера однобитные, и он
   всегда 0; у порта вида [bit; 8] он выбирал бы разряд. */
static bool bench_read_bit(Watchdog_In_BitPort port, uint8_t bit, void *userdata) {
    struct Bench *bench = (struct Bench *)userdata;

    (void)bit;
    if (port == WATCHDOG_WATCHDOG_PORT_KICK) {
        if (bench->tick >= 0 && bench->tick < SCENARIO_LEN)
            return SCENARIO[bench->tick] != 0;
    }
    return false;
}

static void bench_write_bit(Watchdog_Out_BitPort port, uint8_t bit, bool value, void *userdata) {
    struct Bench *bench = (struct Bench *)userdata;

    (void)bit;
    if (port == WATCHDOG_WATCHDOG_PORT_ALARM) {
        bench->alarm = value ? 1 : 0;
        if (bench->alarm && bench->alarm_raised_at < 0)
            bench->alarm_raised_at = bench->tick + 1;
    }
}

int main(void) {
    Watchdog model;
    struct Bench bench;
    int i;

    bench.tick = 0;
    bench.alarm = 0;
    bench.alarm_raised_at = -1;

    /* Порядок существен: Watchdog_init выполняет enter начального состояния,
       а тот пишет в выходной порт — то есть вызывает write_bit. Установка
       обработчиков после init даёт разыменование нулевого указателя. */
    model.userdata = &bench;
    model.read_bit = bench_read_bit;
    model.write_bit = bench_write_bit;
    Watchdog_init(&model);

    for (i = 0; i < SCENARIO_LEN; ++i) {
        bench.tick = i;
        Watchdog_tick(&model);
        printf("такт %2d: kick=%d idle=%u alarm=%d\n", i + 1, SCENARIO[i],
               (unsigned)model.root.idle, bench.alarm);
    }

    if (bench.alarm_raised_at < 0) {
        fprintf(stderr, "ОШИБКА: авария не сработала, хотя сигнал пропадал\n");
        return EXIT_FAILURE;
    }
    printf("Авария поднята на такте %d\n", bench.alarm_raised_at);
    return EXIT_SUCCESS;
}
