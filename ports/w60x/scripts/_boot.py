# put system init code

# print("")
# print("    __            __")
# print("    \ \    /\    / /")
# print("     \ \  /  \  / /")
# print("      \ \/ /\ \/ / ")
# print("       \  /  \  /")
# print("       / /\  / /\ ")
# print("      / /\ \/ /\ \ ")
# print("     / /  \  /  \ \ ")
# print("    /_/    \/    \_\ ")

from upysh import *
from easyw600 import *
try:
    import wifi_config
    connect(wifi_config.WIFI_SSID, wifi_config.WIFI_PASSWD)
    ftpserver()
except:
    print("No Wifi setting found")
from pye_mp import pye
