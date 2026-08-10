# zlib-ng

## Changes

```bash
cp src/zconf.h.in zconf.h
sed 's/@ZLIB_SYMBOL_PREFIX@//g' src/zlib.h.in > zlib.h
cp src/zlib_name_mangling.h.empty zlib_name_mangling.h
```
