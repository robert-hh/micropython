include("manifest_WLAN.py")
# BLE
include(
    "$(MPY_LIB_DIR)/micropython/bluetooth/aioble/manifest.py",
    client=True,
    central=True,
    l2cap=True,
    security=True,
)
