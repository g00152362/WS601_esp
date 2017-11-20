#!/bin/bash
make clean
echo "gen_misc.sh version 20150511"
echo ""

echo "gen user1..............."
 boot=new
 app=1
 spi_speed=40
 spi_mode=DIO
 spi_size_map=4




touch user/user_main.c

make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map

echo "clean..............."

make clean

echo "gen user2..............."
 boot=new
 app=2
 spi_speed=40
 spi_mode=DIO
 spi_size_map=4
 
 

touch user/user_main.c

make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map
