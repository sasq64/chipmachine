#!/usr/bin/python

import os.path
import subprocess
import argparse


def which(program):
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


parser = argparse.ArgumentParser(description='Build chipmachine')
parser.add_argument('--buildsystem', choices=['ninja', 'make', 'xcode'], default='ninja',
                   help='Build system to use')

parser.add_argument('--config', choices=['release', 'debug'], default='release',
                   help='Release or debug')

parser.add_argument('--cross-target', choices=['native', 'raspberry', 'windows', 'adroid'], default='native',
                   help='Cross compilation target')

args = parser.parse_args()

print(args.buildsystem)

configs = { 'release' : [ 'release', '-DCMAKE_BUILD_TYPE=Release' ] }
buildsystems = { 'ninja' : [ '-GNinja',  ] }

buildArgs = []
buildArgs.append(configs[args.config][1])
buildArgs.append(buildsystems[args.buildsystem][0])

print(buildArgs)

try :
	os.mkdir("nbuild")
except :
	pass

if not os.path.isfile("nbuild/CMakeCache.txt") :
	subprocess.call(["cmake", "-Bnbuild", "-H.", "-DCMAKE_BUILD_TYPE=Release", "-GNinja"])

subprocess.call(["ninja", "-C", "nbuild"])

