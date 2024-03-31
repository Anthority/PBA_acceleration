[gates:5285]

1. required arrival time与哪些量有关？data的slew发生变化，是否会导致required time发生变化？
   slew变化不会导致rat发生变化，只会导致at发生变化，从而影响到slack。rat只与外部constraint、到达endpoint上的clock tree的slew和at有关
2. 在pba下，所有的路径的slack都有可能变好。因为在gba下，所有路径段都采用了最差slew，所以即使是slack最差的一条路径，它的路径段也可能被其他路径上的悲观slew干扰，造成gba下的at变坏。
3. 一条时序弧是不是偏离边和它身上的slew是否最悲观没有任何关系，即使是最短路径树上的边也有可能不是最悲观的slew
4. 只有net arc能够fanout，只有cell arc能够fan in

big benchmark

ac97_ctrl.exe [gates:14341]

aes_core.exe [gates:22938]

des_perf.exe [gates:105371]

vga_lcd.exe [gates:139529]

sdc "tv80.sdc" doesn't exist [gates:5285]

sdc "usb_phy_ispd.sdc" doesn't exist [gates:923]

sdc "wb_dma.sdc" doesn't exist [gates:4195]

benchmark

"c1355" [gates:180]

"c17" [gates:6]

"c17_slack" [gates:6]

"c1908" [gates:222]

"c2670" [gates:344]

"c3540" [gates:691]

"c3_path" [gates:3]

"c3_slack" [gates:3]

"c432" [gates:134]

"c499" [gates:176]

"c5315" [gates:918]

"c6288" [gates:1667]

"c7552" [gates:1147]

"c7552_slack" [gates:1147]

"c880" [gates:221]

"s1196" [gates:641]

"s1494" [gates:804]

"s27" [gates:28]

"s27_path" [gates:28]

"s344" [gates:182]

"s349" [gates:194]

"s386" [gates:177]

"s400" [gates:221]

"s510" [gates:291]

"s526" [gates:304]

"simple" [gates:5]
