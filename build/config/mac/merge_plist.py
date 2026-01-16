"""
Merges multiple Apple Property List files (.plist) and perform variables
substitutions $(VARIABLE) in the Property List string values.
"""

import argparse
import json
import re
import sys
import plistlib


# Pattern representing a variable to substitute in a string value.
VARIABLE_PATTERN = re.compile(r'\$\(([^)]*)\)')


def load_plist(plist_path):
    """Loads Apple Property List file at |plist_path|."""
    with open(plist_path, 'rb') as fp:
        return plistlib.load(fp)


def save_plist(plist_path, content, fmt_str):
    """Saves |content| as Apple Property List in |format| at |plist_path|."""
    fmt = plistlib.FMT_XML if fmt_str == 'xml' else plistlib.FMT_BINARY

    with open(plist_path, 'wb') as fp:
        plistlib.dump(content, fp, fmt=fmt)


def merge_objects(obj1, obj2):
    """Merges two objects (either dictionary, list, string or numbers)."""
    if type(obj1) != type(obj2):
        return obj2

    if isinstance(obj2, dict):
        result = dict(obj1)
        for key in obj2:
            value1 = obj1.get(key, None)
            value2 = obj2.get(key, None)
            result[key] = merge_objects(value1, value2)
        return result

    if isinstance(obj2, list):
        return obj1 + obj2

    return obj2


def merge_plists(plist_paths):
    """Loads and merges all Apple Property List files at |plist_paths|."""
    plist = {}
    for plist_path in plist_paths:
        plist = merge_objects(plist, load_plist(plist_path))
    return plist


def perform_substitutions(plist, substitutions):
    """Performs variables substitutions in |plist| given by |substitutions|."""
    if isinstance(plist, dict):
        result = dict(plist)
        for key in plist:
            result[key] = perform_substitutions(plist[key], substitutions)
        return result

    if isinstance(plist, list):
        return [perform_substitutions(item, substitutions) for item in plist]

    if isinstance(plist, str):
        result = plist
        while True:
            match = VARIABLE_PATTERN.search(result)
            if not match:
                break

            extent = match.span()
            expand = substitutions[match.group(1)]
            result = result[: extent[0]] + expand + result[extent[1] :]
        return result

    return plist


def perform_substitutions_from(plist, substitutions_path):
    """Performs variable substitutions in |plist| from |substitutions_path|."""
    with open(substitutions_path) as substitutions_file:
        return perform_substitutions(plist, json.load(substitutions_file))


def parse_args(argv):
    """Parses command line arguments."""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument(
        '-s', '--substitutions', help='path to a JSON file containing variable substitutions'
    )
    parser.add_argument(
        '-f',
        '--format',
        default='xml',
        choices=('xml', 'binary'),
        help='format of the generated file',
    )
    parser.add_argument('-o', '--output', default='-', help='path to the result; - means stdout')
    parser.add_argument('inputs', nargs='+', help='path of the input files to merge')

    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)

    data = merge_plists(args.inputs)
    if args.substitutions:
        data = perform_substitutions_from(data, args.substitutions)

    save_plist(args.output, data, args.format)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
