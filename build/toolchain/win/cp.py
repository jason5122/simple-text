import os
import shutil
import sys


def main():
    src, dst = sys.argv[1:]

    if os.path.exists(dst):
        if os.path.isdir(dst):
            shutil.rmtree(dst)
        else:
            os.remove(dst)

    if os.path.isdir(src):
        shutil.copytree(src, dst)
    else:
        shutil.copy2(src, dst)
        # work around https://github.com/ninja-build/ninja/issues/1554
        os.utime(dst, None)


if __name__ == '__main__':
    sys.exit(main())
