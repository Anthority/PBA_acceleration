1. required arrival time与哪些量有关？data的slew发生变化，是否会导致required time发生变化？
   slew变化不会导致rat发生变化，只会导致at发生变化，从而影响到slack。rat只与外部constraint、到达endpoint上的clock tree的slew和at有关
2. 在pba下，所有的路径的slack都有可能变好。因为在gba下，所有路径段都采用了最差slew，所以即使是slack最差的一条路径，它的路径段也可能被其他路径上的悲观slew干扰，造成gba下的at变坏。
3. 一条时序弧是不是偏离边和它身上的slew是否最悲观没有任何关系，即使是最短路径树上的边也有可能不是最悲观的slew
4. 只有net arc能够fanout，只有cell arc能够fan in
