mtype = {POWER, ON, OFF, CONTROL, COMMAND, MOVE_X, MOVE_Y, MOVE_Z, MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN, MOVE_STOP, CRITICAL};
// https://pastebin.com/eHSuSQds
mtype state = POWER;
mtype last_state = POWER;
int   step = 1;

typedef Guard {
  int   x_max; //+Xl, +Xr
  int   y_max; //+Yl, +Yr
  int   z_max; //+Zh, 0
};

typedef Current {
  int   x;
  int   y;
  int   z;
};

typedef Move {
  mtype x;
  mtype y;
  mtype z;
};

proctype cnc() {
  Guard   guard;
  Current current;
  Move    move;

  printf("[CNC %d] Enter\n", _pid);
  do
    :: last_state = state;
    :: (state == OFF) ->       
      break;
    :: (state == ON) ->
      printf("[CNC %d] Load properties\n", _pid);

      guard.x_max = 100;
      guard.y_max = 100;
      guard.z_max = 10;

      current.x = 0;
      current.y = 0;
      current.z = 0;

      move.x = MOVE_STOP;
      move.y = MOVE_STOP;
      move.z = MOVE_STOP;
      
      state = CONTROL;
    :: (state == CONTROL) ->
      printf("[CNC %d] Control\n", _pid);
      if
        :: (move.y == MOVE_RIGHT && current.y + step >= guard.y_max) || (move.y == MOVE_LEFT && current.y - step < 0) ->
          state = CRITICAL;
        :: (move.z == MOVE_UP && current.z + step >= guard.z_max) || (move.z == MOVE_DOWN && current.z - step < 0) ->
          state = CRITICAL;
        :: else ->
          skip;
      fi
      state = COMMAND;
    :: (state == COMMAND) ->
      printf("[CNC %d] Command\n", _pid);
      move.x = MOVE_RIGHT;
      state = MOVE_X;
    :: (state == MOVE_X) ->
      printf("[CNC %d] Move X. X: %d. %d(LEFT: %d, RIGHT: %d, STOP: %d)\n", _pid, current.x, move.x, MOVE_LEFT, MOVE_RIGHT, MOVE_STOP);
      state = CONTROL;      
      if
        :: (move.x == MOVE_RIGHT && current.x + step >= guard.x_max) || (move.x == MOVE_LEFT && current.x - step < 0) ->
          state = CRITICAL;
        :: (move.x == MOVE_RIGHT) ->
          current.x = current.x + step;
        :: (move.x == MOVE_LEFT) ->
          current.x = current.x - step;
        :: else ->
          printf("[CNC %d] Move blocked. X: %d\n", _pid, current.x);
          state = OFF;
      fi
    :: (state == CRITICAL) -> 
      printf("[CNC %d] Critical\n", _pid);
      state = OFF;
    :: (state == MOVE_Y) ->
      printf("[CNC %d] Move Y\n", _pid);
      state = CONTROL;
    :: (state == MOVE_Z) ->
      printf("[CNC %d] Move Z\n", _pid);
      state = CONTROL;
  od
  printf("[CNC %d] Exit\n", _pid);
}


init {
  run cnc();
  assert(state == POWER);
  printf("[Init] Power\n");
  state = ON
}
