#!/usr/bin/env python

import os
import shutil
import sys

print(sys.argv[1:])

dst, = sys.argv[1:]

if os.path.exists(dst):
  if os.path.isdir(dst):
    shutil.rmtree(dst)
  else:
    os.remove(dst)