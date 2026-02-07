#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Remove previous compiled objects and binaries
echo "Cleaning up previous build files..."
rm -f root.o stdio.o main.o calculadora.elf calculadora.bin

echo "Assembling startup.s..."
arm-none-eabi-as -o root.o root.s

echo "Compiling stdio.c..."
arm-none-eabi-gcc -c -o stdio.o stdio.c

echo "Compiling main.c..."
arm-none-eabi-gcc -c -o main.o main.c

echo "Linking object files..."
arm-none-eabi-gcc -T linker.ld -o calculadora.elf root.o stdio.o main.o -nostdlib -lgcc

echo "Converting ELF to binary..."
arm-none-eabi-objcopy -O binary calculadora.elf calculadora.bin

echo "Running QEMU..."
qemu-system-arm -M versatilepb -nographic -kernel calculadora.elf
