#!/usr/bin/env python3
"""Generate a single //third_party/abseil-cpp/BUILD.gn from Abseil's BUILD.bazel.

Adapted from Chromium's convert_bazel_to_gn.py. Rather than writing a BUILD.gn
next to each BUILD.bazel (which would land inside the upstream `src` submodule
and so couldn't be committed to this repo), this walks the vendored `src/absl`
tree and emits every library target into ONE committable BUILD.gn in the parent
directory. Because a single file can't hold two targets with the same name (and
Abseil reuses names like `config` across packages), target names are flattened:
`base:config` -> `absl_base_config`, with source paths rooted at `src/absl/...`.

Tests and test-only helpers are omitted; gmock isn't vendored here. This is a
heuristic converter (it doesn't parse Bazel `select()`), so a handful of targets
carry manual tweaks below. Review the generated file as if it were hand-written,
and re-run after every abseil submodule bump.

Usage: run with no arguments from //third_party/abseil-cpp (or pass that dir).
"""

import ast
import logging
import os
import re
import subprocess
import sys

# Curated set of Abseil targets exposed through the aggregate static_library,
# mirroring Chromium's `absl_component_deps` (absl/flags and the full absl/log
# are intentionally excluded). Entries are "package" or "package:name".
_AGGREGATE_TARGETS = [
    "algorithm", "algorithm:container",
    "base", "base:config", "base:core_headers", "base:fast_type_id",
    "base:log_severity", "base:no_destructor", "base:nullability",
    "base:prefetch", "base:throw_delegate",
    "cleanup",
    "container:btree", "container:fixed_array", "container:flat_hash_map",
    "container:flat_hash_set", "container:hash_container_defaults",
    "container:inlined_vector", "container:linked_hash_map",
    "container:linked_hash_set", "container:node_hash_map",
    "container:node_hash_set",
    "crc:crc32c",
    "debugging:failure_signal_handler", "debugging:leak_check",
    "debugging:stacktrace", "debugging:symbolize",
    "functional:any_invocable", "functional:bind_back", "functional:bind_front",
    "functional:function_ref", "functional:overload",
    "hash",
    "log:absl_check", "log:absl_log", "log:absl_vlog_is_on", "log:die_if_null",
    "log:globals", "log:initialize", "log:log_entry", "log:log_sink",
    "log:log_sink_registry",
    "memory",
    "meta:type_traits",
    "numeric:bits", "numeric:int128",
    "random", "random:bit_gen_ref", "random:distributions",
    "random:mocking_access",
    "status", "status:status_builder", "status:status_macros",
    "status:statusor",
    "strings", "strings:charset", "strings:cord",
    "strings:has_ostream_operator", "strings:str_format", "strings:string_view",
    "synchronization",
    "time", "time:clock_interface",
    "types:any_span", "types:optional", "types:optional_ref",
    "types:source_location", "types:span", "types:variant",
    "utility",
]

# Extra targets exposed through absl_full (on top of :abseil-cpp): absl/flags
# and the full absl/log, which the default surface omits because flag
# registration adds static initializers and the LOG/CHECK macros collide.
_ABSL_FULL_EXTRA_TARGETS = [
    "flags:config", "flags:flag", "flags:marshalling", "flags:parse",
    "flags:reflection", "flags:usage",
    "log:log", "log:check",
]

# Benchmark-only helpers that upstream ships as plain cc_library targets even
# though nothing outside a benchmark (or the helper's own test) depends on them.
# We emit a source_set per discovered library, so `ninja all` builds these even
# though the port never references them. nanobenchmark's non-x86/PPC timing
# fallback calls POSIX clock_gettime, which doesn't compile for Windows on Arm,
# so skip it rather than build dead, unportable code. Keyed "package:name".
_SKIP_TARGETS = {
    "random/internal:nanobenchmark",
}

# Comment/config blocks emitted just before a given target. Keyed "package:name".
_EXTRA_PREFIX = {
    "flags:config":
    '''# absl/flags is only pulled in by test-style binaries, so there's no need to
# strip flag names.
config("absl_flags_strip_names") {
  defines = [ "ABSL_FLAGS_STRIP_NAMES=0" ]
}
''',
    "log:check":
    '''# Banned: use absl_check instead. The CHECK macros collide with other CHECK
# macros; libraries should use ABSL_CHECK.''',
    "log:log":
    '''# Banned: use absl_log instead. The LOG macros collide with other LOG macros;
# libraries should use ABSL_LOG.''',
    "log:vlog_is_on":
    '''# Banned: use ABSL_-prefixed macros and absl_vlog_is_on instead.''',
}

# Per-target content appended inside the target. These cover cases BUILD.bazel
# expresses with select(), which this script doesn't parse. Source paths are
# already rooted at src/absl/<package>/.
_EXTRA_CONTENT = {
    "flags:config": 'public_configs += [ ":absl_flags_strip_names" ]',
    "random/internal:seed_material": 'if (is_win) { libs = [ "bcrypt.lib" ] }',
    "strings:strings": 'public_deps = [ ":absl_strings_string_view" ]',
    "time/internal/cctz:time_zone":
    '''
if (is_win) {
  sources += [ "src/absl/time/internal/cctz/src/time_zone_name_win.cc" ]
  public += [ "src/absl/time/internal/cctz/src/time_zone_name_win.h" ]
}
defines = []
if (is_mac) {
  frameworks = [ "Foundation.framework" ]

  # Work-around for https://github.com/llvm/llvm-project/issues/117630
  defines += [ "_XOPEN_SOURCE=700" ]
}''',
}


def _flat_name(package, name):
    return "absl_" + (package + "/" + name).replace("/", "_")


def _resolve_dep(dep, rel_path):
    """Map a Bazel dep label to a flattened ':absl_...' ref, or None to drop it."""
    if dep.startswith(":"):
        return ":" + _flat_name(rel_path, dep[1:])
    if dep.startswith("//absl/"):
        rest = dep[len("//absl/"):]
        if ":" in rest:
            package, name = rest.split(":", 1)
        else:
            package, name = rest, rest.split("/")[-1]
        return ":" + _flat_name(package, name)
    # Anything else (@googletest, @google_benchmark, ...) only appears in tests
    # or test-only targets, which are dropped.
    return None


def _agg_ref(entry):
    if ":" in entry:
        package, name = entry.split(":", 1)
    else:
        package, name = entry, entry.split("/")[-1]
    return ":" + _flat_name(package, name)


def _ast_get_value(node, local_vars):
    if isinstance(node, ast.Constant):
        return node.value
    elif isinstance(node, ast.List):
        return [_ast_get_value(elt, local_vars) for elt in node.elts]
    elif isinstance(node, ast.Name):
        if node.id in local_vars:
            return local_vars[node.id]
        return node.id
    elif isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
        left = _ast_get_value(node.left, local_vars) or []
        right = _ast_get_value(node.right, local_vars) or []
        return left + right
    return None


class _Package:
    def __init__(self, path):
        self.bazel_targets = []
        self.path = path
        m = re.search(r".*/absl/(.*)", path)
        self.rel_path = m.group(1) if m else ""

    def parse_bazel(self, content):
        tree = ast.parse(content)
        local_vars = {
            "ABSL_DEFAULT_LINKOPTS": [],
            "ABSL_DEFAULT_COPTS": [],
            "ABSL_TEST_COPTS": [],
        }
        for node in tree.body:
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name):
                        local_vars[target.id] = _ast_get_value(node.value, local_vars)
            if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
                call = node.value
                if isinstance(call.func, ast.Name) and call.func.id in (
                    "cc_library",
                    "cc_test",
                ):
                    t = {"is_test": call.func.id == "cc_test"}
                    for kw in call.keywords:
                        t[kw.arg] = _ast_get_value(kw.value, local_vars)
                    self.bazel_targets.append(t)

    # Targets that include string_view.h should depend on the string_view target
    # directly rather than relying on the (backwards-compat) :strings re-export.
    def _uses_string_view(self, bt):
        strings_dep = ":strings" if self.rel_path == "strings" else "//absl/strings"
        if strings_dep not in bt.get("deps", []):
            return False
        for s in bt.get("srcs", []) + bt.get("hdrs", []):
            with open(os.path.join(self.path, s), "r", encoding="utf-8") as f:
                if re.search(r'#include "absl/strings/string_view.h"', f.read()):
                    return True
        return False

    def collect(self):
        records = []
        for bt in self.bazel_targets:
            name = bt.get("name")
            if not name:
                continue
            if bt.get("is_test") or bt.get("testonly"):
                continue
            if f"{self.rel_path}:{name}" in _SKIP_TARGETS:
                continue
            deps = bt.get("deps", [])
            if "@google_benchmark//:benchmark_main" in deps:
                continue
            if "//absl/base:exception_safety_testing" in deps:
                continue

            target_name = f"{self.rel_path}:{name}"
            hdrs = sorted(bt.get("hdrs", []) + bt.get("textual_hdrs", []))
            if target_name == "strings:strings":
                # Re-exported via public_deps on :string_view (see _EXTRA_CONTENT).
                hdrs = [h for h in hdrs if h != "string_view.h"]

            gn_deps = set()
            for d in deps:
                r = _resolve_dep(d, self.rel_path)
                if r:
                    gn_deps.add(r)
            if self._uses_string_view(bt):
                gn_deps.add(":" + _flat_name("strings", "string_view"))
            if target_name == "strings:strings":
                # string_view is carried by public_deps, not deps.
                gn_deps.discard(":" + _flat_name("strings", "string_view"))

            records.append({
                "target_name": target_name,
                "flat": _flat_name(self.rel_path, name),
                "base": "src/absl/" + self.rel_path + "/",
                "sources": sorted(bt.get("srcs", [])),
                "public": hdrs,
                "deps": sorted(gn_deps),
            })
        return records


def _emit(records):
    out = [
        "# Copyright 2024 The Chromium Authors",
        "# Use of this source code is governed by a BSD-style license that can be",
        "# found in the LICENSE file.",
        "#",
        "# Generated by convert_bazel_to_gn.py from Abseil's BUILD.bazel. DO NOT EDIT.",
        "",
        "# Puts the vendored source root on the include path so `#include \"absl/...\"`",
        "# resolves, for Abseil's own sources and for consumers of :abseil-cpp.",
        'config("absl_include_config") {',
        '  include_dirs = [ "src" ]',
        "}",
        "",
    ]

    flat_names = {r["flat"] for r in records}
    for r in sorted(records, key=lambda r: r["target_name"]):
        prefix = _EXTRA_PREFIX.get(r["target_name"])
        if prefix:
            out.append(prefix)
        out.append(f'source_set("{r["flat"]}") {{')
        out.append('  public_configs = [ ":absl_include_config" ]')
        if r["sources"]:
            # Abseil is vendored third-party code; compile it with the relaxed
            # third-party warning set instead of our strict first-party one,
            # which -Werrors on upstream's style. Header-only targets compile
            # nothing, so they don't need this.
            out.append(
                '  configs -= [ "//build/config/compiler:simple_text_code" ]')
            out.append(
                '  configs += [ "//build/config/compiler:no_simple_text_code" ]')
            out.append("  sources = [")
            out += [f'    "{r["base"]}{s}",' for s in r["sources"]]
            out.append("  ]")
        if r["public"]:
            out.append("  public = [")
            out += [f'    "{r["base"]}{h}",' for h in r["public"]]
            out.append("  ]")
        # Only visible within this file: consumers depend on :abseil-cpp.
        out.append('  visibility = [ ":*" ]')
        if r["deps"]:
            out.append("  deps = [")
            out += [f'    "{d}",' for d in r["deps"]]
            out.append("  ]")
        extra = _EXTRA_CONTENT.get(r["target_name"])
        if extra:
            out.append(extra)
        out.append("}")
        out.append("")

    agg = sorted({_agg_ref(e) for e in _AGGREGATE_TARGETS})
    missing = [d for d in agg if d[1:] not in flat_names]
    if missing:
        logging.warning("Aggregate references unknown targets: %s", missing)
    # A group, not a static_library: every object lives on a source_set dep, so
    # a static_library would archive nothing. That empty archive is a no-op on
    # Linux/macOS but on Windows lib.exe refuses to write an input-less .lib,
    # breaking downstream links. Chromium's absl_component_deps is a group too.
    out.append("# Default Abseil surface (mirrors Chromium's absl_component_deps).")
    out.append('group("abseil-cpp") {')
    out.append("  public_deps = [")
    out += [f'    "{d}",' for d in agg]
    out.append("  ]")
    out.append("}")
    out.append("")
    # Some third-party code (e.g. fuzztest) needs Abseil parts the default
    # surface omits (flags, full log). absl_full = :abseil-cpp + those.
    full = sorted({_agg_ref(e) for e in _ABSL_FULL_EXTRA_TARGETS})
    missing = [d for d in full if d[1:] not in flat_names]
    if missing:
        logging.warning("absl_full references unknown targets: %s", missing)
    out.append('group("absl_full") {')
    out.append("  public_deps = [")
    out.append('    ":abseil-cpp",')
    out += [f'    "{d}",' for d in full]
    out.append("  ]")
    out.append("}")
    out.append("")
    return "\n".join(out)


def convert(abseil_dir):
    records = []
    for dirpath, _, filenames in os.walk(os.path.join(abseil_dir, "src", "absl")):
        if "BUILD.bazel" not in filenames:
            continue
        with open(os.path.join(dirpath, "BUILD.bazel"), "r", encoding="utf-8") as f:
            content = f.read()
        pkg = _Package(dirpath)
        pkg.parse_bazel(content)
        records.extend(pkg.collect())

    text = _emit(records)
    gn_path = os.path.join(abseil_dir, "BUILD.gn")
    with open(gn_path, "w", encoding="utf-8", newline="") as f:
        subprocess.run(
            ["gn", "format", "--stdin"],
            check=True,
            input=text,
            stdout=f,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    logging.info("Wrote %s with %d targets", gn_path, len(records))


if __name__ == "__main__":
    logging.getLogger().setLevel(logging.INFO)
    abseil_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(
        os.path.abspath(__file__))
    convert(abseil_dir)
