include('../manifest.py')

# uasyncio
include("$(MPY_DIR)/extmod/uasyncio/manifest.py")

# drivers
freeze("$(MPY_DIR)/drivers/display", "ssd1306.py")

# Libraries from micropython-lib, include only if the library directory exists
if os.path.isdir(convert_path("$(MPY_LIB_DIR)")):
    # umqtt
    freeze("$(MPY_LIB_DIR)/micropython/umqtt.simple", "umqtt/simple.py")
    freeze("$(MPY_LIB_DIR)/micropython/umqtt.robust", "umqtt/robust.py")
freeze('$(MPY_DIR)/../pye', 'pye_mp.py')
freeze('$(MPY_DIR)/../uftpd', 'uftpd.py')
# freeze('$(MPY)/../uping', 'uping.py')
