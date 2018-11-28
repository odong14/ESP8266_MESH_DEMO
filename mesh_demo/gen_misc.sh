#!/bin/bash
export SDK_BASE=$(dirname $(pwd))

echo "gen_misc.sh version 20150511"
#"STEP 1: choose boot version(old=boot_v1.1, new=boot_v1.2+, else=none)"
    boot=none
#"STEP 2: choose bin generate(0=eagle.flash.bin+eagle.irom0text.bin, 1=user1.bin, 2=user2.bin)"
    app=0
#"STEP 3: choose spi speed(0=20MHz, 1=26.7MHz, 2=40MHz, 3=80MHz)"
    spi_speed=40
#"STEP 4: choose spi mode(0=QIO, 1=QOUT, 2=DIO, 3=DOUT)"
    spi_mode=QIO
#"STEP 5: choose spi size and map"
#echo "    0= 512KB( 256KB+ 256KB)"
#echo "    2=1024KB( 512KB+ 512KB)"
#echo "    3=2048KB( 512KB+ 512KB)"
#echo "    4=4096KB( 512KB+ 512KB)"
#echo "    5=2048KB(1024KB+1024KB)"
#echo "    6=4096KB(1024KB+1024KB)"
    spi_size_map=6  #untuk nodemcu
	#spi_size_map=2  #untuk ESP01


make clean

make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map
