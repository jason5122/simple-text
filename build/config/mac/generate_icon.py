"""
Creates an icon file from an iconset.
"""

import argparse
import os
import subprocess
import sys


def generate_icon(iconset, out):
    """Generates |iconset| iconset to |out|."""
    subprocess.check_call(['iconutil', '-c', 'icns', '-o', out, iconset])


def parse_args(argv):
    """Parses command line arguments."""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument('input', help='iconset for the application')
    parser.add_argument(
        '-o',
        '--output',
        required=True,
        help='path to the result; - means stdout',
    )

    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)

    generate_icon(
        os.path.abspath(args.input),
        os.path.abspath(args.output),
    )


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
