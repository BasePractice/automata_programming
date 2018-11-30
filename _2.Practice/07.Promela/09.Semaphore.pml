mtype = {P, V}
chan sem = [0] of {mtype}

active proctype semaphore() {
    L:
    sem!P -> 
       sem?V;
    goto L;
}

active [5] proctype user() {
    L:
    sem?P ->
        printf("User %d use sem\n", _pid);
        sem!V;
    goto L;
}

