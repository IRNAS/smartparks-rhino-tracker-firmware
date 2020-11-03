import os
import sys
import time

# CONFIGURE the version manually
firmware_version ="v2.0.2"

firmware_compile = "arduino-cli compile --fqbn TleraCorp:stm32l0:IRNAS-env-module-L072Z src/main.ino --output "
firmware_name = "build/smartparks-rhino-tracker-firmware"
board_list = {"VER2_3_DROPOFF","VER2_3_FENCE","VER2_3_LION","VER2_2_4_RHINO"}

for board in board_list:
    command=firmware_compile+firmware_name+"-"+firmware_version+"-"+board
    command_sed="sed -i \"/#define VER/c\#define " + board +"\" src/board.h"
    compile_output = os.popen(command_sed).read()
    compile_output = os.popen(command).read()
    if "Sketch uses" in compile_output:
        print("Compile successful!")
    else:
        print("Compile failed!")
    filename = firmware_name+"-"+firmware_version+"-"+board
    # generate bin
    command_bin = "arm-none-eabi-objcopy -O binary "+filename+".elf "+filename+".bin"
    compile_output = os.popen(command_bin).read()
    # generate hex
    command_hex = "arm-none-eabi-objcopy -O ihex "+filename+".elf "+filename+".hex"
    compile_output = os.popen(command_hex).read()
    