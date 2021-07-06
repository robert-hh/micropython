# _boot.py
# Try to mount the filesystem, and format the flash if it doesn't exist.
# Note: the flash requires the programming size to be aligned to 256 bytes.

import os
import sys
import mimxrt
from machine import SDCard

bdev = mimxrt.Flash()
try:
    vfs = os.VfsLfs2(bdev, progsize=256)
except:
    os.VfsLfs2.mkfs(bdev, progsize=256)
    vfs = os.VfsLfs2(bdev, progsize=256)
os.mount(vfs, "/flash")
os.chdir("/flash")

# do not mount the SD card if SKIPSD exists.
try:
    os.stat("SKIPSD")
except:
    sdcard = SDCard(1)
    if sdcard:
        try:
            fat = os.VfsFat(sdcard)
            os.mount(fat, "/sdcard")
            os.chdir("/sdcard")
            sys.path.append("/sdcard")
        except:
            print("Mounting SD card failed")

sys.path.append("/flash")
