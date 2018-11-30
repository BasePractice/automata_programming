mtype = {Produce, Consume};
mtype turn = Produce;

active proctype producer() {
    do
    :: (turn == Produce) -> 
       printf("Produced\n");
       turn = Consume
    od
}

active proctype consumer() {
    do
    :: (turn == Consume) -> 
       printf("Consume\n");
       turn = Produce
    do
}
