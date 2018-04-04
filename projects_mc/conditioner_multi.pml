mtype = {START, ON, OFF, CONTROL, COOLING, HEATING, WAITING,  T_INITIALIZE, T_READ, T_WAIT, T_OFF};

mtype state = START;
mtype t_state = START;
mtype last_state = START;
int guard_temperature = 12;
int current_temperature = 14;
int waiting_count = 10;

/** ! not, && - and, || - or, -> - зависимость, <-> - эквивалентность */
/** Проверка что температура ([] - обязательно, <> - случится) будет равна предельной */
ltl temp_equals   { []<> (current_temperature == guard_temperature) }
/** Проверка что если состояние COOLING то текущая температура больше границы */
ltl cooling       { <> (state == COOLING && current_temperature > guard_temperature) }
/** Проверка что если состояние HEATING то текущая температура меньше границы */
ltl heating       { <> (state == HEATING && current_temperature < guard_temperature) }
/** Проверка на то, что состояние OFF обязательно будет идти только после состояния WAITING */
ltl off_complete  { []<> ( (last_state == WAITING) && (state == OFF) ) }

/** По умолчанию запускается temp_equals проверка. Для смены проверки: pan -N off_complete */

int  t_wait_count = 2;

proctype termometer() {
  printf("[Termometer %d] Enter\n", _pid);
  do
    :: (t_state == T_INITIALIZE) ->
      printf("[Termometer %d] Load properties\n", _pid);
      t_wait_count = 2;
      t_state = T_READ;
    :: (t_state == T_OFF) ->
      printf("[Termometer %d] Signal off\n", _pid);
      break;
    :: (t_state == T_READ) ->
      printf("[Termometer %d] Read current temperature: %d\n", _pid, current_temperature);
      t_state = T_WAIT;
    :: (t_state == T_WAIT) ->
      /*printf("[Termometer %d] Wait: %d\n", _pid, t_wait_count);*/
      t_wait_count--;
      if
        :: (t_wait_count <= 0) -> 
          t_wait_count = 2;
          t_state = T_READ;
        :: else ->
          t_state = T_WAIT;
      fi
    :: (state == OFF) ->
      t_state = T_OFF;
    :: (state == COOLING) ->
      current_temperature--;
      t_state = T_READ;
    :: (state == HEATING) ->
      current_temperature++;
      t_state = T_READ;
  od
  printf("[Termometer %d] Exit\n", _pid);
}

proctype conditioner() {
  int local_temperature = 0;
  run termometer();
  printf("[Conditioner %d] Enter\n", _pid);
  do
    :: last_state = state;
    :: (state == OFF) ->
      printf("[Conditioner %d] Store temperature and properties\n", _pid);
      /*Дожидаемся, что терометр завершит работу */
      t_state == T_OFF;
      printf("[Conditioner %d] Dependencies processes, wait\n", _pid);
      break;
    :: (state == ON) ->
      t_state = T_INITIALIZE;
      printf("[Conditioner %d] Load properties\n", _pid);
      printf("[Conditioner %d] Guard: %dC\n", _pid, guard_temperature);
      t_state == T_READ;
      state = CONTROL;
    :: (state == CONTROL) ->
      printf("[Conditioner %d] Controling\n", _pid);
      local_temperature = current_temperature;
      printf("[Conditioner %d] Current temperature: %dC\n", _pid, local_temperature);
      if
        :: (local_temperature > guard_temperature) ->
          state = COOLING;
        :: (local_temperature < guard_temperature) ->
          state = HEATING;
        :: else ->
          state = WAITING; 
      fi      
    :: (state == COOLING) ->
      assert(local_temperature > guard_temperature);
      printf("[Conditioner %d] Cooling\n", _pid);
      state = CONTROL;
    :: (state == HEATING) ->
      assert(local_temperature < guard_temperature);
      printf("[Conditioner %d] Heating\n", _pid);
      state = CONTROL;
    :: (state == WAITING) ->
      assert(local_temperature == guard_temperature);
      printf("[Conditioner %d] Waiting\n", _pid);
      state = CONTROL;
      waiting_count--;
      if
        :: (waiting_count == 5) ->
          printf("[Conditioner %d] Simulating. Downgrade temperature.\n", _pid);
          current_temperature = guard_temperature - 2; /** Для проверки HEATING */
        :: (waiting_count < 0) ->
          state = OFF;
        :: else ->
          skip;
      fi  
  od
  printf("[Conditioner %d] Exit\n", _pid);
}


init {
  run conditioner();
  assert(state == START);
  printf("[Init] Start\n");
  state = ON;
}
