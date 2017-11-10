#!/usr/bin/env python

import os.path
import subprocess
import argparse
import platform

os_name = platform.system()

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
parser.add_argument('actions', choices=['build', 'clean', 'run', 'config'], default='build',
                   nargs='*', help='Actions to perform')

parser.add_argument('--buildsystem', choices=['ninja', 'make', 'xcode'], default='ninja',
                   help='Build system to use')

parser.add_argument('--config', choices=['release', 'debug', 'usan', 'asan', 'tsan'], default='release',
                   help='Release or Debug config')

parser.add_argument('--output', default='builds',
                   help='Output directory')

parser.add_argument('--target', choices=['native', 'raspberry', 'windows', 'android'], default='native',
                   help='(Cross) compilation target')

args = parser.parse_args()

configs = { 'release' : [ 'release', ['-DCMAKE_BUILD_TYPE=Release'] ],
            'debug' : [ 'debug', ['-DCMAKE_BUILD_TYPE=Debug'] ],
            'usan' : [ 'usan', ['-DCMAKE_BUILD_TYPE=Debug', '-DSAN=undefined'] ]
          }
buildsystems = { 'make' : ['-GUnix Makefiles'],
                 'ninja' : [ '-GNinja',  ]
               }

buildTool = args.buildsystem;
buildArgs = []
buildArgs += configs[args.config][1]
buildArgs.append(buildsystems[args.buildsystem][0])
#buildArgs.append('-DCMAKE_TOOLCHAIN_FILE=clang.cmake')
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
        if not os.path.isfile(os.path.join(outputDir, 'build.ninja')) :
            subprocess.call(['cmake', '-B' + outputDir, '-H.'] + buildArgs)
        args = [buildTool, '-C', outputDir]
        if buildTool == 'make' :
            args.append('-j8')
        subprocess.call(args)
    elif a == 'config' :
        subprocess.call(['cmake', '-B' + outputDir, '-H.'] + buildArgs)
    elif a == 'clean' :
        subprocess.call([buildTool, '-C', outputDir, 'clean'])
    elif a == 'run' :
        exe = os.path.join(outputDir, 'chipmachine')
        print exe
        os.system(exe + ' -d')

