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
        for path in os.environ['PATH'].split(os.pathsep):
            path = path.strip("'")
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


parser = argparse.ArgumentParser(description='Build chipmachine')
parser.add_argument('actions', choices=['build', 'clean', 'run'], default='build',
                   nargs='*', help='Actions to perform')

parser.add_argument('--buildsystem', choices=['ninja', 'make', 'xcode'], default='ninja',
                   help='Build system to use')

parser.add_argument('--config', choices=['release', 'debug'], default='release',
                   help='Release or Debug config')

parser.add_argument('--output', default='builds',
                   help='Output directory')

parser.add_argument('--target', choices=['native', 'raspberry', 'windows', 'android'], default='native',
                   help='(Cross) compilation target')

args = parser.parse_args()

configs = { 'release' : [ 'release', '-DCMAKE_BUILD_TYPE=Release' ],
            'debug' : [ 'debug', '-DCMAKE_BUILD_TYPE=Debug' ]
          }
buildsystems = { 'ninja' : [ '-GNinja',  ] }
buildArgs = []
buildArgs.append(configs[args.config][1])
buildArgs.append(buildsystems[args.buildsystem][0])

outputDir = os.path.join(args.output, configs[args.config][0])

try :
	os.makedirs(outputDir)
except :
	pass

if args.actions == 'build' :
    args.actions = [ 'build' ]

for a in args.actions :
    a = a.strip()
    if a == 'build' :
        if not os.path.isfile(os.path.join(outputDir, 'CMakeCache.txt')) :
            subprocess.call(['cmake', '-B' + outputDir, '-H.'] + buildArgs)
        subprocess.call(['ninja', '-C', outputDir])
    elif a == 'clean' :
        subprocess.call(['ninja', '-C', outputDir, 'clean'])
    elif a == 'run' :
        exe = os.path.join(outputDir, 'Chipmachine.app/Contents/MacOS/chipmachine')
        os.system(exe + ' -d')

