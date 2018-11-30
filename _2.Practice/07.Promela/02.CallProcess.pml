proctype main() {
    printf("Hello world!\n")
}

init {
    printf("Hello init!\n")
    run main()
}