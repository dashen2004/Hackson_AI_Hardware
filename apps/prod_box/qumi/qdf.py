#!/usr/bin/env python
#encoding=utf-8

import sys,getopt
import os
import shutil

def main(argv):
    option = sys.argv[1]

    if option == "menuconfig":
        os.system('python tools/kconfig/menuconfig.py  ./qm_kconfig')
        if os.path.isfile(".config"):
            os.system('python tools/kconfig/genconfig.py')
        if os.path.isfile("config.h"):
            shutil.move("config.h","./include/config/qm_config.h")
        

if __name__ == '__main__':
    main(sys.argv[1:])








