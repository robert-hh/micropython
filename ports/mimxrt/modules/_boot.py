# _boot.py
# Try to mount the filesystem, and format the flash if it doesn't exist.
# Note: the flash requires the programming size to be aligned to 256 bytes.

import os
import mimxrt
from machine import SDCard

bdev = mimxrt.Flash()
try:
    vfs = os.VfsLfs2(bdev, progsize=256)
except:
    os.VfsLfs2.mkfs(bdev, progsize=256)
    vfs = os.VfsLfs2(bdev, progsize=256)
os.mount(vfs, "/flash")

sdcard = SDCard(1)
try:
    fat = os.VfsFat(sdcard)
    os.mount(fat, "/sdcard")
except:
    print("Mounting SD card failed!")
