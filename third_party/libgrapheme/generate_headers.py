#!/usr/bin/env python

"""
Generate a C header from a text file.
"""

import subprocess
import sys

header = open(sys.argv[2], 'w')
code = subprocess.Popen(
    sys.argv[1], shell=True, universal_newlines=True, stdout=header, cwd=sys.argv[3]
)
code.wait()
