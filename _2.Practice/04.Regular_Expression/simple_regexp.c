#include <string.h>
#include "simple_regexp.h"

enum OneZeroPlus_State {
    OZP_ONE, OZP_ZERO, OZP_EMPTY
};

enum XYZ_State {
    XYZ_EMPTY, XYZ_Z, XYZ_X, XYZ_Y
};

struct Iterator {
    char *text;
    size_t size;
    size_t it;
};

static void iterator_init(struct Iterator *it, const char *text) {
    (*it).it = 0;
    (*it).text = (char *) text;
    (*it).size = strlen(text);
}

static int iterator_next(struct Iterator *it) {
    int ch = -1;
    if (it != 0) {
        if (it->it < it->size) {
            ch = it->text[it->it++];
        }
    }
    return ch;
}

static bool one_zero_plus(const char *text) {
    struct Iterator it;
    int ch;
    enum OneZeroPlus_State state = OZP_EMPTY;

    iterator_init(&it, text);
    do {
        ch = iterator_next(&it);
        switch (state) {
            case OZP_EMPTY: {
                if (ch == '1') {
                    state = OZP_ONE;
                    break;
                }
                return false;
            }
            case OZP_ONE: {
                if (ch == '0') {
                    state = OZP_ZERO;
                    break;
                }
                return false;
            }
            case OZP_ZERO: {
                if (ch == '0') {
                    break;
                } else if (ch == -1) {
                    break;
                }
                return false;
            }

        }
    } while (ch != -1);
    return true;
}

static bool xyz(const char *text) {
    struct Iterator it;
    int ch;
    enum XYZ_State state = XYZ_EMPTY;

    iterator_init(&it, text);
    do {
        ch = iterator_next(&it);
        switch (state) {
            case XYZ_EMPTY: {
                if (ch == 'z') {
                    state = XYZ_Z;
                    break;
                } else if (ch == 'x') {
                    state = XYZ_X;
                    break;
                }
                return false;
            }
            case XYZ_X: {
                if (ch == 'y') {
                    state = XYZ_Y;
                    break;
                } else if (ch == -1) {
                    break;
                }
                return false;
            }
            case XYZ_Y: {
                if (ch == 'y') {
                    break;
                } else if (ch == -1) {
                    break;
                }
                return false;
            }
            case XYZ_Z: {
                if (ch == -1)
                    break;
                return false;
            }
        }
    } while (ch != -1);
    return true;
}

bool match(enum Type type, const char *text) {
    switch (type) {
        case ONE_ZERO_PLUS:
            return one_zero_plus(text);
        case XYZ:
            return xyz(text);
        default:
            return false;
    }
}
