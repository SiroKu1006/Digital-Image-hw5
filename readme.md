## p1
```
gcc p1.c fft.c -fexec-charset=big5 -lm -o p1
.\p1.exe testpattern1024.bmp p1_output.bmp {value}
```
## p2
```
gcc p2.c fft.c -lm -o p2
./p2 Fig0464(a).bmp output.bmp {value} {value} {value} {value}
```
## p3
```
gcc p3.c fft.c -lm -o p3
.\p3.exe "Fig0508(a)(circuit-board-pepper-prob-pt1).bmp" "p3(a)_output.bmp" {value}
.\p3.exe "Fig0508(b)(circuit-board-pepper-prob-pt2).bmp" "p3(b)_output.bmp" {value}
```