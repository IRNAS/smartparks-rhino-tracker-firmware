import os
import sys
import time
import re


# CONFIGURE the version manually
firmware_version = sys.argv[1] #"v2.2"
print(firmware_version)
# Split this into major and minor
numbers= re.split('v(\d+)\.(\d+)', firmware_version)
print(numbers)
#inject version
command_sed="sed -i \"/#define FW_VERSION_MAJOR/c\#define " + "FW_VERSION_MAJOR "+str(numbers[1]) +"\" main/board.h"
compile_output = os.popen(command_sed).read()
command_sed="sed -i \"/#define FW_VERSION_MINOR/c\#define " + "FW_VERSION_MINOR "+str(numbers[2]) +"\" main/board.h"
compile_output = os.popen(command_sed).read()

firmware_compile = "arduino-cli compile --fqbn IRNAS:stm32l0:IRNAS-env-module-L072Z main --output-dir "
path = "build"
firmware_name = "smartparks-rhino-tracker-firmware"
board_list = {"VER2_3_DROPOFF","VER2_3_FENCE","VER2_3_LION","VER2_2_4_RHINO"}

for board in board_list:
    command=firmware_compile+path
    command_sed="sed -i \"/#define VER/c\#define " + board +"\" main/board.h"
    compile_output = os.popen(command_sed).read()
    compile_output = os.popen(command).read()
    if "Sketch uses" in compile_output:
        print("Compile successful: "+ board)
    else:
        print("Compile failed: "+ board)
    # generate bin
    command_bin = "arm-none-eabi-objcopy -O binary "+path+"/main.ino.elf "+path+"/main.ino.bin"
    compile_output = os.popen(command_bin).read()
    # generate hex
    command_hex = "arm-none-eabi-objcopy -O ihex "+path+"/main.ino.elf "+path+"/main.ino.hex"
    compile_output = os.popen(command_hex).read()
    # rename and move all files to the format
    filename = firmware_name+"-"+firmware_version+"-"+board
    os.popen("mv "+path+"/main.ino.elf "+filename+".elf").read()
    os.popen("mv "+path+"/main.ino.bin "+filename+".bin").read()
    os.popen("mv "+path+"/main.ino.dfu "+filename+".dfu").read()
    os.popen("mv "+path+"/main.ino.hex "+filename+".hex").read()
    