#!/usr/bin/env python

import argparse
import os
import pickle
import sys
import subprocess

import os
from os.path import dirname, isdir, isfile


parser = argparse.ArgumentParser()
parser.add_argument("--version", type=str, default=None)
parser.add_argument("--python", type=str, default=None)
args = parser.parse_args()

def pwd():
    return os.getcwd()

def cd(d):
    if d:
        os.chdir(d)
    print("pwd: " + pwd())

def mkcd(d):
    if not os.path.exists(d):
        os.makedirs(d)
    cd(d)

def subshell(cmd):
    print("+ " + cmd)
    out = subprocess.check_output(cmd, shell=True, executable="/bin/bash").strip()
    print(" -> {}".format(out))
    return out

def call(cmd):
    print("+ " + cmd)
    subprocess.check_call(cmd, shell=True, executable="/bin/bash")

def call_status(cmd):
    print("+ " + cmd)
    return subprocess.call(cmd, shell=True, executable="/bin/bash")

def source(file):
    # Hoist environment post-sourcing of a bash file into this process.
    raw = subprocess.check_output("""
        source {};
        python -c 'import os, pickle, sys; sys.stdout.write(pickle.dumps(dict(os.environ)))'
    """.format(file), shell=True, executable="/bin/bash")
    env = pickle.loads(raw)
    os.environ.update(env)

if args.python is None:
    python = subshell("which python")
else:
    python = args.path

if args.version is None:
    # Get numpy version from system python
    np_ver = subshell(python + " -c 'import numpy as np; print(np.version.version)'")
    np_git_rev = subshell(python + " -c 'import numpy as np; print(np.version.git_revision)'")
else:
    np_ver = args.version
    if np_ver[0] in "0123456789":
        np_git_rev = "v" + args.version
    else:
        np_git_rev = np_ver

fork = "https://github.com/numpy/numpy"
feature_commit_attempts = [
    'patches/feature_v1.11.0.patch',
    'patches/feature_v1.15.0.dev0+3ec8875.patch',
]
dev_patch_features_commit = 'patches/dev_patch_features.patch'
fix_9351_commit = 'patches/fix_9351_v1.13.1.x.patch'

# Clone fork
cd(dirname(__file__))
cur_dir = pwd()

def apply_patch_cmd(rel):
    patch_path = os.path.join(cur_dir, rel)
    return "git am --committer-date-is-author-date < {}".format(patch_path)

def apply_patch(rel):
    return call(apply_patch_cmd(rel))

def try_apply_patch(rel):
    if call_status(apply_patch_cmd(rel)) == 0:
        return True
    else:
        call("git am --abort")
        return False

# TODO(eric.cousineau): Figure out how to get a base version from a Git commit...

mkcd("tmp/patch_{}".format(np_ver))
if isdir("numpy"):
    cd("numpy")
    call("git clean -fxd")
    call("git checkout --force -B tmp_patch {}".format(np_git_rev))
else:
    call("git clone {}".format(fork))
    cd("numpy")
    call("git checkout -B tmp_patch {}".format(np_git_rev))

good = False
# Try out commits in order
for commit in feature_commit_attempts:
    if try_apply_patch(commit):
        good = True
        break
if not good:
    raise Exception(
        "Could not apply any patch. Please try to resolve merge conflict.")

try_apply_patch(fix_9351_commit)
apply_patch(dev_patch_features_commit)

# Virtualenv
mkcd("../env")
if not isfile("bin/activate"):
    call("virtualenv --system-site-packages .")
source("bin/activate")
os.environ["NUMPY_PATCH"] = "1"

# Build
cd("..")

cd("numpy")
call("python setup.py build -j 8 install")
cd("..")
# Ensure we have the patch.
call("""python -c '
import numpy as np; has = "prefer_user_copyswap" in getattr(np.lib, "dev_patch_features", []);
exit(not has)'""")
# Test
call("python -c 'import numpy; numpy.test()'")

# Test `pybind11`
pybind11 = "/home/eacousineau/proj/tri/repo/repro/externals/pybind11"
commit = "feature/numpy_dtype_user-wip"

if isdir("pybind11"):
    cd("pybind11")
    call("git checkout {}".format(commit))
    call("git clean -fxd")
else:
    call("git clone {} pybind11 -b {}".format(pybind11, commit))
    cd("pybind11")
mkcd("build")
call("cmake .. -DPYTHON_EXECUTABLE=$(which python)")
call("make -j pytest")
