mtype = {START, ON, OFF, CONTROL, COOLING, HEATING, WAITING};

mtype state = START;
mtype last_state = START;
int guard_temperature = 12;
int current_temperature = 0;
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

proctype conditioner() {
  printf("[Conditioner %d] Enter\n", _pid);
  do
    :: (state == OFF) ->       
      printf("[Conditioner %d] Store temperature and properties\n", _pid);
      break;
    :: (state == ON) ->
      printf("[Conditioner %d] Load  temperature and properties\n", _pid);
      guard_temperature = 12;
      current_temperature = 14; /** Для проверки COOLING */
      printf("[Conditioner %d] Loaded: %dC, Current: %dC\n", _pid, guard_temperature, current_temperature);
      last_state = state;
      state = CONTROL;
    :: (state == CONTROL) ->
      printf("[Conditioner %d] Controling\n", _pid);
      /** Получаем тепуратуру помещения */
      printf("[Conditioner %d] Current temperature: %dC\n", _pid, current_temperature);
      last_state = state;
      if
        :: (current_temperature > guard_temperature) ->
          state = COOLING;
        :: (current_temperature < guard_temperature) ->
          state = HEATING;
        :: else ->
          state = WAITING; 
      fi      
    :: (state == COOLING) ->
      assert(current_temperature > guard_temperature);
      printf("[Conditioner %d] Cooling\n", _pid);
      current_temperature--;
      last_state = state;
      state = CONTROL;
    :: (state == HEATING) ->
      assert(current_temperature < guard_temperature);
      printf("[Conditioner %d] Heating\n", _pid);
      current_temperature++;
      last_state = state;
      state = CONTROL;
    :: (state == WAITING) ->
      assert(current_temperature == guard_temperature);
      printf("[Conditioner %d] Waiting\n", _pid);
      last_state = state;
      state = CONTROL;
      if
        :: (waiting_count == 5) ->
          printf("[Conditioner %d] Simulating. Downgrade temperature.\n", _pid);
          current_temperature = guard_temperature - 2; /** Для проверки HEATING */
          waiting_count--;
        :: (waiting_count < 0) ->
          state = OFF;
        :: else ->
          waiting_count--;
      fi  
  od
  printf("[Conditioner %d] Exit\n", _pid);
}


init {
  run conditioner();
  assert(state == START);
  printf("[Init] Start\n");
  state = ON
}
