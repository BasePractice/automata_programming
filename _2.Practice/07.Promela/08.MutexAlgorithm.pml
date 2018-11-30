//spin -a 08.MutexAlgorithm.pml
//gcc -DSAFETY -o pan pan.c
//spin -t -p 08.MutexAlgorithm.pml
byte turn[2];
byte mutex;

active proctype invariant() {
    assert( mutex <= 1 )
}

active [2] proctype P() {
    bit i = _pid;
    L:
    //turn[i] = 1;
    turn[i] = turn[i + 1];
    (turn[1 - i] == 0 || (turn[i] < turn[1 - i])) ->
      mutex++;
    mutex--;
    turn[i] = 0;
    goto L;
}

