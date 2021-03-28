# Github Actions for building smartparks-rhino-tracker-firmware
The firmware build process is automated using Github Actions, such that the following two actions occur:
* On every push, run a build process to see if build is passing
* On every release created, run a build process to create binary files and attach them to the release

## Instructions to use
For a build check, nothing is required, happens on every push. For release creating, make sure the vX.Y format is used to tag this as the version gets inserted into the firmware upon build automatically.

## Manual operation
The following tools can be used to run this manually: https://github.com/nektos/act
