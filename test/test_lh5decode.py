#!/usr/bin/env python

import os, sys, os.path
import ctypes

LH5_SO="libhd5dec.so"

def compile_library():
    if os.path.exists(LH5_SO):
        return
    # -Wall -Werror
    c = os.system("gcc -O0 -g -fpic -shared -o %s ../src/lh5dec.c" % (LH5_SO))
    if c <> 0:
        raise RuntimeError("unable to compile .so")

def load_library():
    global liblh5dec
    liblh5dec = ctypes.CDLL(LH5_SO)
    # print dir(liblh5dec)

def get_lh5_compressed(path):
    #  lha.exe a tune.lzh tune.bin






if __name__ == "__main__":
    compile_library()
    load_library()

    # try to compress and decompress lot of samples
    for f in os.listdir("/usr/bin"):
        p = os.path.join("/usr/bin", f)
        if not os.path.isfile(p):
            continue
        sz = os.path.getsize(p)
        if sz < 1000 or sz > 1000000:
            continue
        print f

        compressed = get_lh5_compressed(p)


