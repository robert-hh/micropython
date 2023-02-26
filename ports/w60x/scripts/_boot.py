# put system init code
import gc
import os
import w600
import sys

bdev = w600.Flash()

# Try to mount the filesystem, and format the flash if it doesn't exist.

try:
    vfs = os.VfsLfs2(bdev, progsize=256)
    os.mount(vfs, "/")
except:
    os.VfsLfs2.mkfs(bdev, progsize=256)
    vfs = os.VfsLfs2(bdev, progsize=256)
    os.mount(vfs, "/")
    with open("boot.py", "w") as f:
        f.write("# boot.py -- run on boot-up\r\n")
    with open("main.py", "w") as f:
        f.write("# main.py -- put your code here!\r\n")
    os.mkdir("/lib")

sys.path.append("/lib")
del vfs, bdev, os, w600, sys
gc.collect()
del gc
