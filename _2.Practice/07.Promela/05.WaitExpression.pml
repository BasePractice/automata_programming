int x;

proctype B() {
    x = x + 1
}

proctype A() {
    int y = 1;

    skip;
    x = 2;
    (x > 2 && y == 1);
    printf("This is end\n");
}

init {
    run A()
}
