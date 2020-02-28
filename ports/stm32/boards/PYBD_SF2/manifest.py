include('$(PORT_DIR)/boards/manifest.py')
# include('$(MPY_DIR)/extmod/webrepl/manifest.py')

LIB = '$(MPY_DIR)/../micropython-lib'

freeze('$(MPY_DIR)/../pye', 'pye_mp.py')
freeze('$(MPY_DIR)/../uftpd', 'uftpd.py')
# freeze('$(MPY_DIR)/../uping', 'uping.py')
freeze('$(PORT_DIR)/modules-hh', ('upysh.py', 'reload.py', 'station.py'))
# freeze('$(MPY_DIR)/tools', ('upip.py', 'upip_utarfile.py'))
