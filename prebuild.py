import subprocess
import os.path
 
arduinoCorePath = os.path.expanduser("~/Library/Arduino15/packages/IRNAS/hardware/stm32l0/0.0.12")
if not os.path.isdir(arduinoCorePath):
    print(f"ERROR: Cannot find Arduino core directory {arduinoCorePath}")
    exit(1)

coreBranch = subprocess.check_output(["git", "branch", "--show-current"], cwd=arduinoCorePath).decode().strip()

if coreBranch != "SingleChannelLongPreamble":
    # Try to switch branch
    subprocess.check_call(["git", "checkout", "SingleChannelLongPreamble"], cwd=arduinoCorePath)

coreBranch = subprocess.check_output(["git", "branch", "--show-current"], cwd=arduinoCorePath).decode().strip()

if coreBranch != "SingleChannelLongPreamble":
    print(f"ERROR: Arduino core should be 'SingleChannelLongPreamble' branch in {arduinoCorePath}")
    exit(1)