import argparse
import os
import plistlib
import shutil
import subprocess
import sys


def ReadPlistFromString(plist_bytes):
    """Parse property list from given |plist_bytes|.

    Args:
      plist_bytes: contents of property list to load. Must be bytes in python 3.

    Returns:
      The contents of property list as a python object.
    """
    if sys.version_info.major == 2:
        return plistlib.readPlistFromString(plist_bytes)
    else:
        return plistlib.loads(plist_bytes)


def LoadPlistFile(plist_path):
    """Loads property list file at |plist_path|.

    Args:
      plist_path: path to the property list file to load.

    Returns:
      The content of the property list file as a python object.
    """
    if sys.version_info.major == 2:
        return plistlib.readPlistFromString(
            subprocess.check_output(['xcrun', 'plutil', '-convert', 'xml1', '-o', '-', plist_path])
        )
    else:
        with open(plist_path, 'rb') as fp:
            return plistlib.load(fp)


def CreateSymlink(value, location):
    """Creates symlink with value at location if the target exists."""
    target = os.path.join(os.path.dirname(location), value)
    if os.path.exists(location):
        os.unlink(location)
    os.symlink(value, location)


class Bundle(object):
    """Wraps a bundle."""

    def __init__(self, bundle_path, platform):
        """Initializes the Bundle object with data from bundle Info.plist file."""
        self._path = bundle_path
        self._kind = Bundle.Kind(platform, os.path.splitext(bundle_path)[-1])
        self._data = None

    def Load(self):
        self._data = LoadPlistFile(self.info_plist_path)

    @staticmethod
    def Kind(platform, extension):
        if platform == 'iphonesimulator' or platform == 'iphoneos':
            return 'ios'
        if platform == 'macosx':
            if extension == '.framework':
                return 'mac_framework'
            return 'mac'
        raise ValueError('unknown bundle type %s for %s' % (extension, platform))

    @property
    def kind(self):
        return self._kind

    @property
    def path(self):
        return self._path

    @property
    def contents_dir(self):
        if self._kind == 'mac':
            return os.path.join(self.path, 'Contents')
        if self._kind == 'mac_framework':
            return os.path.join(self.path, 'Versions/A')
        return self.path

    @property
    def executable_dir(self):
        if self._kind == 'mac':
            return os.path.join(self.contents_dir, 'MacOS')
        return self.contents_dir

    @property
    def resources_dir(self):
        if self._kind == 'mac' or self._kind == 'mac_framework':
            return os.path.join(self.contents_dir, 'Resources')
        return self.path

    @property
    def info_plist_path(self):
        if self._kind == 'mac_framework':
            return os.path.join(self.resources_dir, 'Info.plist')
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


def GenerateBundleInfoPlist(bundle, plist_compiler, partial_plist):
    """Generates the bundle Info.plist for a list of partial .plist files.

    Args:
      bundle: a Bundle instance
      plist_compiler: string, path to the Info.plist compiler
      partial_plist: list of path to partial .plist files to merge
    """

    # Filter empty partial .plist files (this happens if an application
    # does not compile any asset catalog, in which case the partial .plist
    # file from the asset catalog compilation step is just a stamp file).
    filtered_partial_plist = []
    for plist in partial_plist:
        plist_size = os.stat(plist).st_size
        if plist_size:
            filtered_partial_plist.append(plist)

    # Invoke the plist_compiler script. It needs to be a python script.
    subprocess.check_call(
        [
            'python3',
            plist_compiler,
            'merge',
            '-f',
            'binary1',
            '-o',
            bundle.info_plist_path,
        ]
        + filtered_partial_plist
    )


class Action(object):
    """Class implementing one action supported by the script."""

    @classmethod
    def Register(cls, subparsers):
        parser = subparsers.add_parser(cls.name, help=cls.help)
        parser.set_defaults(func=cls._Execute)
        cls._Register(parser)


class CodeSignBundleAction(Action):
    """Class implementing the code-sign-bundle action."""

    name = 'code-sign-bundle'
    help = 'perform code signature for a bundle'

    @staticmethod
    def _Register(parser):
        parser.add_argument(
            '--entitlements',
            '-e',
            dest='entitlements_path',
            help='path to the entitlements file to use',
        )
        parser.add_argument('path', help='path to the iOS bundle to codesign')
        parser.add_argument('--identity', '-i', required=True, help='identity to use to codesign')
        parser.add_argument('--binary', '-b', required=True, help='path to the iOS bundle binary')
        parser.add_argument(
            '--framework',
            '-F',
            action='append',
            default=[],
            dest='frameworks',
            help='install and resign system framework',
        )
        parser.add_argument(
            '--disable-code-signature',
            action='store_true',
            dest='no_signature',
            help='disable code signature',
        )
        parser.add_argument(
            '--platform', '-t', required=True, help='platform the signed bundle is targeting'
        )
        parser.add_argument(
            '--partial-info-plist',
            '-p',
            action='append',
            default=[],
            help='path to partial Info.plist to merge to create bundle Info.plist',
        )
        parser.add_argument(
            '--plist-compiler-path',
            '-P',
            action='store',
            help='path to the plist compiler script (for --partial-info-plist)',
        )
        parser.set_defaults(no_signature=False)

    @staticmethod
    def _Execute(args):
        if not args.identity:
            args.identity = '-'

        bundle = Bundle(args.path, args.platform)

        if args.partial_info_plist:
            GenerateBundleInfoPlist(bundle, args.plist_compiler_path, args.partial_info_plist)

        # The bundle Info.plist may have been updated by GenerateBundleInfoPlist()
        # above. Load the bundle information from Info.plist after the modification
        # have been written to disk.
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

        # Delete existing embedded mobile provisioning.
        embedded_provisioning_profile = os.path.join(bundle.path, 'embedded.mobileprovision')
        if os.path.isfile(embedded_provisioning_profile):
            os.unlink(embedded_provisioning_profile)

        # Delete existing code signature.
        if os.path.exists(bundle.signature_dir):
            shutil.rmtree(bundle.signature_dir)

        # Copy main binary into bundle.
        if not os.path.isdir(bundle.executable_dir):
            os.makedirs(bundle.executable_dir)
        shutil.copy(args.binary, bundle.binary_path)

        if bundle.kind == 'mac_framework':
            # Create Versions/Current -> Versions/A symlink
            CreateSymlink('A', os.path.join(bundle.path, 'Versions/Current'))

            # Create $binary_name -> Versions/Current/$binary_name symlink
            CreateSymlink(
                os.path.join('Versions/Current', bundle.binary_name),
                os.path.join(bundle.path, bundle.binary_name),
            )

            # Create optional symlinks.
            for name in ('Headers', 'Resources', 'Modules'):
                target = os.path.join(bundle.path, 'Versions/A', name)
                if os.path.exists(target):
                    CreateSymlink(
                        os.path.join('Versions/Current', name), os.path.join(bundle.path, name)
                    )
                else:
                    obsolete_path = os.path.join(bundle.path, name)
                    if os.path.exists(obsolete_path):
                        os.unlink(obsolete_path)

        if args.no_signature:
            return

        codesign_extra_args = []

        CodeSignBundle(bundle.path, args.identity, codesign_extra_args)


def Main():
    parser = argparse.ArgumentParser('codesign iOS bundles')
    subparsers = parser.add_subparsers()

    CodeSignBundleAction.Register(subparsers)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    sys.exit(Main())
