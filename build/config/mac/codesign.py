import argparse
import os
import plistlib
import shutil
import subprocess
import sys


def LoadPlistFile(plist_path):
    """Loads property list file at |plist_path|.

    Args:
      plist_path: path to the property list file to load.

    Returns:
      The content of the property list file as a python object.
    """
    with open(plist_path, 'rb') as fp:
        return plistlib.load(fp)


class Bundle(object):
    """Wraps a bundle."""

    def __init__(self, bundle_path):
        """Initializes the Bundle object with data from bundle Info.plist file."""
        self._path = bundle_path
        self._data = None

    def Load(self):
        self._data = LoadPlistFile(self.info_plist_path)

    @property
    def path(self):
        return self._path

    @property
    def contents_dir(self):
        return os.path.join(self.path, 'Contents')

    @property
    def executable_dir(self):
        return os.path.join(self.contents_dir, 'MacOS')

    @property
    def resources_dir(self):
        return os.path.join(self.contents_dir, 'Resources')

    @property
    def info_plist_path(self):
        return os.path.join(self.contents_dir, 'Info.plist')

    @property
    def signature_dir(self):
        return os.path.join(self.contents_dir, '_CodeSignature')

    @property
    def identifier(self):
        return self._data['CFBundleIdentifier']

    @property
    def binary_name(self):
        return self._data['CFBundleExecutable']

    @property
    def binary_path(self):
        return os.path.join(self.executable_dir, self.binary_name)

    def Validate(self, expected_mappings):
        """Checks that keys in the bundle have the expected value.

        Args:
          expected_mappings: a dictionary of string to object, each mapping will
          be looked up in the bundle data to check it has the same value (missing
          values will be ignored)

        Returns:
          A dictionary of the key with a different value between expected_mappings
          and the content of the bundle (i.e. errors) so that caller can format the
          error message. The dictionary will be empty if there are no errors.
        """
        errors = {}
        for key, expected_value in expected_mappings.items():
            if key in self._data:
                value = self._data[key]
                if value != expected_value:
                    errors[key] = (value, expected_value)
        return errors


def CodeSignBundle(bundle_path, identity, extra_args):
    process = subprocess.Popen(
        ['xcrun', 'codesign', '--force', '--sign', identity, '--timestamp=none']
        + list(extra_args)
        + [bundle_path],
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    _, stderr = process.communicate()
    if process.returncode:
        sys.stderr.write(stderr)
        sys.exit(process.returncode)
    for line in stderr.splitlines():
        if line.endswith(': replacing existing signature'):
            # Ignore warning about replacing existing signature as this should only
            # happen when re-signing system frameworks (and then it is expected).
            continue
        sys.stderr.write(line)
        sys.stderr.write('\n')


def Execute(args):
    if not args.identity:
        args.identity = '-'

    bundle = Bundle(args.path)
    bundle.Load()

    # According to Apple documentation, the application binary must be the same
    # as the bundle name without the .app suffix. See crbug.com/740476 for more
    # information on what problem this can cause.
    #
    # To prevent this class of error, fail with an error if the binary name is
    # incorrect in the Info.plist as it is not possible to update the value in
    # Info.plist at this point (the file has been copied by a different target
    # and ninja would consider the build dirty if it was updated).
    #
    # Also checks that the name of the bundle is correct too (does not cause the
    # build to be considered dirty, but still terminate the script in case of an
    # incorrect bundle name).
    #
    # Apple documentation is available at:
    # https://developer.apple.com/library/content/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html
    bundle_name = os.path.splitext(os.path.basename(bundle.path))[0]
    errors = bundle.Validate(
        {
            'CFBundleName': bundle_name,
            'CFBundleExecutable': bundle_name,
        }
    )
    if errors:
        for key in sorted(errors):
            value, expected_value = errors[key]
            sys.stderr.write(
                '%s: error: %s value incorrect: %s != %s\n'
                % (bundle.path, key, value, expected_value)
            )
        sys.stderr.flush()
        sys.exit(1)

    # Delete existing code signature.
    if os.path.exists(bundle.signature_dir):
        shutil.rmtree(bundle.signature_dir)

    # Copy main binary into bundle.
    if not os.path.isdir(bundle.executable_dir):
        os.makedirs(bundle.executable_dir)
    shutil.copy(args.binary, bundle.binary_path)

    if args.no_signature:
        return

    codesign_extra_args = []

    CodeSignBundle(bundle.path, args.identity, codesign_extra_args)


def Main():
    parser = argparse.ArgumentParser('codesign')

    parser.set_defaults(func=Execute)

    parser.add_argument('path', help='path to the macOS bundle to codesign')
    parser.add_argument('--identity', '-i', required=True, help='identity to use to codesign')
    parser.add_argument('--binary', '-b', required=True, help='path to the macOS bundle binary')
    parser.add_argument(
        '--disable-code-signature',
        action='store_true',
        dest='no_signature',
        help='disable code signature',
    )
    parser.set_defaults(no_signature=False)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    sys.exit(Main())
