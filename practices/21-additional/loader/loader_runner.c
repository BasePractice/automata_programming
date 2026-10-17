/**
 * @file
 * Реализация связки «автомат — установка»: см. loader_runner.h.
 *
 * Порты автомата — не переменные, а вызовы: порождённый код обращается к ним
 * через обратные вызовы `read_bit`/`write_bit` и `read_numeric`/`write_numeric`.
 * Здесь эти вызовы отвечают показаниями модели цеха и текущей командой плана.
 */

#include <stdlib.h>
#include <string.h>

#include "loader_runner.h"

/* bit — номер разряда порта: у однобитных портов погрузчика он всегда 0, а
   у порта вида [bit; 8] выбирал бы разряд. Takt передаёт его с версии языка
   0.19; до неё обработчик разряда не получал. */
static bool runner_read_bit(Loader_In_BitPort port, uint8_t bit, void *userdata) {
    struct LoaderRunner *runner = (struct LoaderRunner *)userdata;

    (void)bit;
    switch (port) {
    case LOADER_PORT_CMD_VALID:
        return runner->cmd_valid != 0;
    case LOADER_PORT_SENSE_LINE:
        return runner->sensors.line != 0;
    case LOADER_PORT_SENSE_MOTION:
        return runner->sensors.motion != 0;
    case LOADER_PORT_SENSE_LOAD:
        return runner->sensors.load != 0;
    case LOADER_PORT_RESET:
        return runner->reset != 0;
    }
    return false;
}

static void runner_write_bit(Loader_Out_BitPort port, uint8_t bit, bool value, void *userdata) {
    struct LoaderRunner *runner = (struct LoaderRunner *)userdata;
    int flag = value ? 1 : 0;

    (void)bit;
    switch (port) {
    case LOADER_PORT_CMD_ACK:
        runner->cmd_ack = flag;
        break;
    case LOADER_PORT_CMD_DONE:
        runner->cmd_done = flag;
        break;
    case LOADER_PORT_DRIVE_GAS:
        runner->commands.gas = flag;
        break;
    case LOADER_PORT_TURN_LEFT:
        runner->commands.turn_left = flag;
        break;
    case LOADER_PORT_TURN_RIGHT:
        runner->commands.turn_right = flag;
        break;
    case LOADER_PORT_FORK_UP:
        runner->commands.fork_up = flag;
        break;
    case LOADER_PORT_FORK_DOWN:
        runner->commands.fork_down = flag;
        break;
    case LOADER_PORT_FAULT:
        runner->fault = flag;
        break;
    }
}

/* index — элемент порта-массива; у числовых портов погрузчика массивов нет. */
static int64_t runner_read_numeric(Loader_In_NumericPort port, uint8_t index, void *userdata) {
    struct LoaderRunner *runner = (struct LoaderRunner *)userdata;

    (void)index;
    switch (port) {
    case LOADER_PORT_CMD_CODE:
        return runner->cmd_code;
    case LOADER_PORT_CMD_POINT:
        return runner->cmd_point;
    case LOADER_PORT_CMD_EXTRA:
        return runner->cmd_extra;
    case LOADER_PORT_CMD_TIMEOUT:
        return runner->cmd_timeout_ms;
    case LOADER_PORT_SENSE_POINT:
        return runner->sensors.point;
    case LOADER_PORT_SENSE_ANGLE:
        return runner->sensors.angle;
    case LOADER_PORT_SENSE_ODOMETER:
        return runner->sensors.odometer;
    case LOADER_PORT_SENSE_RANGE:
        return runner->sensors.range;
    case LOADER_PORT_SENSE_STACK:
        return runner->sensors.stack;
    case LOADER_PORT_SENSE_PALLET:
        return runner->sensors.pallet;
    }
    return 0;
}

/*
 * Источник времени для выдержек модели. Время даёт установка, а не счётчик
 * тактов: такт — шаг логики, и его длительность к физике отношения не имеет.
 */
static uint64_t runner_now_ms(void *userdata) {
    struct LoaderRunner *runner = (struct LoaderRunner *)userdata;

    return (uint64_t)runner->plant.clock_ms;
}

bool loader_runner_init(struct LoaderRunner *runner, const struct FactoryMap *map, int from, int to,
                        int direction, const struct PlanOptions *options) {
    if (runner == NULL || map == NULL)
        return false;
    memset(runner, 0, sizeof(*runner));
    runner->map = map;
    runner->target = to;
    if (!route_find(map, from, to, direction, 3, &runner->route))
        return false;
    if (!plan_build(&runner->route, options, &runner->plan))
        return false;
    if (!loader_plant_init(&runner->plant, map, from, direction))
        return false;
    loader_plant_sensors(&runner->plant, &runner->sensors);

    /* Обработчики ставятся до init: инициализация выполняет enter начального
       состояния, а тот уже пишет в выходные порты. */
    runner->model.userdata = runner;
    runner->model.read_bit = runner_read_bit;
    runner->model.write_bit = runner_write_bit;
    /* Числовых выходных портов у модели нет, поэтому write_numeric в
       порождённой структуре не появился: `taktc` заводит только те обработчики,
       которые модель действительно использует. */
    runner->model.read_numeric = runner_read_numeric;
    runner->model.now_ms = runner_now_ms;
    Loader_init(&runner->model);
    return true;
}

bool loader_runner_init_mission(struct LoaderRunner *runner, const struct FactoryMap *map,
                                const struct Mission *mission) {
    struct LoaderTiming timing;

    if (runner == NULL || map == NULL || mission == NULL)
        return false;
    memset(runner, 0, sizeof(*runner));
    runner->map = map;
    runner->target = mission->place_point != 0 ? mission->place_point : mission->pick_point;
    loader_timing_default(&timing);

    /* Маршрут до места, где стоит паллета, нужен ещё и графике: она
       подсвечивает метки маршрута. Полный план собирает mission_plan. */
    if (!route_find(map, mission->start_point, mission->pick_point, mission->start_direction, 3,
                    &runner->route))
        return false;
    if (!mission_plan(map, mission, &timing, &runner->plan))
        return false;
    if (!loader_plant_init(&runner->plant, map, mission->start_point, mission->start_direction))
        return false;

    loader_plant_place_pallet(&runner->plant, mission->pick_point, mission->pick_stack,
                              mission->pick_pallet);
    if (mission->place_point != 0)
        loader_plant_add_stack(&runner->plant, mission->place_point, mission->place_stack);
    if (mission->jam_after_cells >= 0)
        loader_plant_jam_after(&runner->plant, mission->jam_after_cells);
    if (mission->block_point != 0)
        loader_plant_block(&runner->plant, mission->block_point);

    loader_plant_sensors(&runner->plant, &runner->sensors);
    runner->model.userdata = runner;
    runner->model.read_bit = runner_read_bit;
    runner->model.write_bit = runner_write_bit;
    runner->model.read_numeric = runner_read_numeric;
    runner->model.now_ms = runner_now_ms;
    Loader_init(&runner->model);
    return true;
}

void loader_runner_set_scenario(struct LoaderRunner *runner, const struct Scenario *scenario) {
    if (runner == NULL)
        return;
    runner->scenario = scenario;
    runner->scenario_step = 0;
}

bool loader_runner_scenario_done(const struct LoaderRunner *runner) {
    return runner != NULL && runner->scenario != NULL
           && runner->scenario_step >= runner->scenario->count;
}

/* Такт в режиме сценария: значения портов берутся из файла как есть. */
static void tick_from_scenario(struct LoaderRunner *runner) {
    const struct ScenarioStep *step;

    if (loader_runner_scenario_done(runner))
        return;
    step = &runner->scenario->step[runner->scenario_step++];
    runner->cmd_valid = step->value[LOADER_IN_CMD_VALID];
    runner->cmd_code = step->value[LOADER_IN_CMD_CODE];
    runner->cmd_point = step->value[LOADER_IN_CMD_POINT];
    runner->cmd_extra = step->value[LOADER_IN_CMD_EXTRA];
    runner->cmd_timeout_ms = step->value[LOADER_IN_CMD_TIMEOUT];
    runner->reset = step->value[LOADER_IN_RESET];
    runner->sensors.line = step->value[LOADER_IN_SENSE_LINE];
    runner->sensors.point = step->value[LOADER_IN_SENSE_POINT];
    runner->sensors.angle = step->value[LOADER_IN_SENSE_ANGLE];
    runner->sensors.odometer = step->value[LOADER_IN_SENSE_ODOMETER];
    runner->sensors.range = step->value[LOADER_IN_SENSE_RANGE];
    runner->sensors.motion = step->value[LOADER_IN_SENSE_MOTION];
    runner->sensors.stack = step->value[LOADER_IN_SENSE_STACK];
    runner->sensors.pallet = step->value[LOADER_IN_SENSE_PALLET];
    runner->sensors.load = step->value[LOADER_IN_SENSE_LOAD];

    /* Время всё равно нужно: по нему работает сторожевой таймер команды.
       В режиме сценария такт стоит столько же, сколько на стенде. */
    runner->plant.clock_ms += LOADER_TICK_MS;
    Loader_tick(&runner->model);
}

void loader_runner_tick(struct LoaderRunner *runner) {
    if (runner == NULL)
        return;
    ++runner->ticks;

    if (runner->scenario != NULL) {
        tick_from_scenario(runner);
        return;
    }

    /*
     * Команда предъявляется и держится, пока автомат её не исполнит: так
     * исполнителю не нужен буфер, а планировщику — угадывать, успел ли автомат
     * её прочитать. Обратная половина рукопожатия столь же обязательна:
     * следующая команда подаётся только после того, как автомат снял cmd_done.
     * Без этой проверки новая команда поднимается в том же такте, в котором
     * снята прежняя, автомат не видит cmd_valid = 0 и навсегда остаётся в
     * состоянии Reporting — а драйвер, глядя на застрявший cmd_done, бодро
     * досчитывает план до конца, хотя погрузчик стоит на месте.
     */
    if (!runner->cmd_valid && !runner->cmd_done && runner->step < runner->plan.count) {
        runner->cmd_code = runner->plan.step[runner->step].code;
        runner->cmd_point = runner->plan.step[runner->step].point;
        runner->cmd_extra = runner->plan.step[runner->step].extra;
        runner->cmd_timeout_ms = runner->plan.step[runner->step].timeout_ms;
        runner->cmd_valid = 1;
    }

    Loader_tick(&runner->model);
    loader_plant_tick(&runner->plant, &runner->commands);
    loader_plant_sensors(&runner->plant, &runner->sensors);
    runner->reset = 0;

    if (runner->cmd_done && runner->cmd_valid) {
        runner->cmd_valid = 0;
        runner->cmd_code = 0;
        runner->cmd_point = 0;
        runner->cmd_extra = 0;
        runner->cmd_timeout_ms = 0;
        ++runner->step;
    }
}

bool loader_runner_done(const struct LoaderRunner *runner) {
    return runner != NULL && runner->step >= runner->plan.count && !runner->cmd_valid;
}

void loader_runner_reset_fault(struct LoaderRunner *runner) {
    if (runner != NULL)
        runner->reset = 1;
}

const char *loader_runner_command_name(const struct LoaderRunner *runner) {
    if (runner == NULL || runner->step >= runner->plan.count)
        return "план исполнен";
    return plan_command_name(runner->plan.step[runner->step].code);
}
