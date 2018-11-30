//spin -a 07.ResourceExclude.pml
//gcc -DSAFETY -o pan pan.c
//spin -t -p 07.ResourceExclude.pml
bool busy;
byte mutex;

proctype P(bit i) {
    (!busy) -> busy = true;
    mutex++;
    printf("%d -> %d\n", i, mutex);
    mutex--;
    busy = false
}

active proctype invariant() {
    assert( mutex <= 1 )
}

init {
    atomic {
        run P(0); run P(1)
    }
}
