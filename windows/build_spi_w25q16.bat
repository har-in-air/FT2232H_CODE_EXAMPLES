echo on
gcc -c spi_w25q16.c -o spi_w25q16.o
gcc -o spi_w25q16.exe spi_w25q16.o -L. -lMPSSE
echo Running the application ...
spi_w25q16
echo off
