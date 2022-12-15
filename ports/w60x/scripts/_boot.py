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
    import wifi_config
    from easyw600 import *
    connect(wifi_config.WIFI_SSID, wifi_config.WIFI_PASSWD)
    ftpserver()
except:
    print("No Wifi setting found")
