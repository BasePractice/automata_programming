bool b;

active proctype P() {
  do
  :: b = !b;
  od
}

ltl f1 {[]<>b}
ltl f2 {<>[]b}