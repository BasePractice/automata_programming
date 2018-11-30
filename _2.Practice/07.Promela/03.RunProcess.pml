proctype main() {
    printf("Hello world!\n")
}

init {
    pid main_pid;
    
    main_pid = run main();
    printf("Hello %d!. My pid %d\n", main_pid, _pid)
}