mtype = {POWER, ON, OFF, WAIT, CONTROL, X_PLUS, X_MINUS, Y_PLUS, Y_MINUS, Z_PLUS, Z_MINUS, CRITICAL};
chan command = [0] of { mtype };
mtype state = POWER;
mtype last_state = POWER;

ltl when_off      { []<>(state == OFF)    }
ltl when_on       { <>(state == ON)       }
ltl when_x_minus  { <>(state == X_MINUS)  }
ltl when_x_plus   { <>(state == X_PLUS)   }


active proctype generator() {
  command ! ON;
  command ! X_PLUS;
  command ! X_MINUS;
  command ! X_PLUS;
  command ! X_PLUS;
  command ! WAIT;
  command ! X_PLUS;
  command ! X_PLUS;
  command ! X_PLUS;
  command ! X_PLUS;
  command ! X_PLUS;
  command ! OFF;
}

typedef Coord {
  int  x;
  int  y;
  int  z;
};

typedef Max {
  int  x;
  int  y;
  int  z;
};


active proctype cnc() {
  Coord coord;
  Max max;
  do
    :: state == OFF ->
      break;
    :: state == ON ->
      coord.x = 0;
      coord.y = 0;
      coord.z = 0;

      max.x = 5;
      max.y = 5;
      max.z = 5;
      state = CONTROL;
    :: state == X_PLUS ->
      last_state = state;
      state = CONTROL;
      if
        :: coord.x + 1 >= max.x ->
          coord.x = max.x;
          state = CRITICAL;
        :: else ->
          /*Move*/
          coord.x = coord.x + 1;
      fi
    :: state == X_MINUS ->
      last_state = state;
      state = CONTROL;
      if
        :: coord.x - 1 < 0 ->
          coord.x = 0;
          state = CRITICAL;
        :: else ->
          /*Move*/
          coord.x = coord.x - 1;
      fi
    :: state == WAIT ->
      state = CONTROL;
    :: state == CONTROL ->
      command ? state;
    :: state == CRITICAL ->
      printf("Critical when %e\n", last_state);
      state = OFF;
  od                  	
}

init {
  state = ON;
}
