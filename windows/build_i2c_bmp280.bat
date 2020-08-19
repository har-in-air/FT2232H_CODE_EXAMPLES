
echo on
gcc -c i2c_bmp280.c -o bmp280.o
gcc -o i2c_bmp280.exe bmp280.o -L. -lMPSSE
echo Running the application ...
i2c_bmp280.exe
echo off
