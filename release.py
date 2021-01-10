import os
import sys
import time
import re


# CONFIGURE the version manually
try:
    firmware_version = sys.argv[1] #"v2.2"
    print(firmware_version)
    # Split this into major and minor
    numbers= re.split('v(\d+)\.(\d+)', firmware_version)
except:
    numbers=[0,0,0,0] # backup if the release input fails
    firmware_version="v0.0"
    print("Release version incorrect, defaulting to 0")
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

    # rename and move all files to the format
    filename = firmware_name+"-"+firmware_version+"-"+board
    os.popen("mv "+path+"/main.ino.elf "+path+"/"+filename+".elf").read()
    os.popen("mv "+path+"/main.ino.bin "+path+"/"+filename+".bin").read()
    os.popen("mv "+path+"/main.ino.dfu "+path+"/"+filename+".dfu").read()
    os.popen("mv "+path+"/main.ino.hex "+path+"/"+filename+".hex").read()
    os.popen("rm "+path+"/main.ino.map").read()
    