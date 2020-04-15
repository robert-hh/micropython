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

try:
    from upysh import *
except:
    print("Importing upysh failed")
try:
    import wifi_config
    from easyw600 import *
    connect(wifi_config.WIFI_SSID, wifi_config.WIFI_PASSWD)
    ftpserver()
except:
    print("No Wifi setting found")
try:
    from pye import pye
except:
    print("importing pye failed")
