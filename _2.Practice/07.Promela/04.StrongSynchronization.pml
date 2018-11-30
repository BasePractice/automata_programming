bool toggle = true;
short count;

active proctype A() provided (toggle == true) {
    L:
    count++;
    printf("A: %d\n", count);
    toggle = false;
    goto L;
}

active proctype B() provided (toggle == false) {
    L:
    count--;
    printf("B: %d\n", count);
    toggle = true;
    goto L;
}

