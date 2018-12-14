#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum Direct {
    MoveLeft, MoveRight, MoveUp, MoveDown
};

void move_step(enum Direct direct) {
    fprintf(stdout, "Шаг в %d\n", direct);
}

bool read_sensor_d1() {
    return false;
}


int main(int argc, char **argv) {
    enum State {
        MoveCenter,
        NextState,
        PowerOff
    } state;
    int step_it = 0;
    int center_count = 10;
    for (;state != PowerOff;) {
        bool d1 = read_sensor_d1();
        switch (state) {
            case MoveCenter: {
                move_step(MoveLeft);
                ++step_it;
                if (d1 == true) {
                    state = NextState;
                    step_it = 0;
                } else if (step_it >= 3) {
                    state = NextState;
                    step_it = 0;
                }
                break;
            }
        }
    }

    return EXIT_SUCCESS;
}
