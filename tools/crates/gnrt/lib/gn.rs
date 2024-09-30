// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! GN build file generation.

use crate::config::BuildConfig;
use crate::crates::CrateFiles;
use crate::crates::*;
use crate::deps::{self, DepOfDep};
use crate::paths;
use crate::platforms;

use std::collections::HashMap;
use std::fmt::Write;
use std::path::Path;

use serde::Serialize;

/// Describes a BUILD.gn file for a single crate epoch. Each file may have
/// multiple rules, including:
/// * A :lib target for normal dependents
/// * A :test_support target for first-party testonly dependents
/// * A :cargo_tests_support target for building third-party tests
/// * A :buildrs_support target for third-party build script dependents
/// * Binary targets for crate executables
#[derive(Serialize)]
pub struct BuildFile {
    pub rules: Vec<(String, Rule)>,
}

/// Identifies a package version. A package's dependency list uses this to refer
/// to other targets.
#[derive(Clone, Debug, Eq, Ord, PartialEq, PartialOrd, Serialize)]
pub struct PackageId {
    /// Package name in normalized form, as used in GN target and path names.
    pub name: String,
    /// Package epoch if relevant (i.e. when needed as part of target paths).
    pub epoch: Option<String>,
}

#[derive(Debug, Serialize)]
pub struct RuleCommon {
    pub testonly: bool,
    /// Controls the visibility constraint on the GN target. If this is true, no
    /// visibility constraint is generated. If false, it's defined so that only
    /// other third party Rust crates can depend on this target.
    pub public_visibility: bool,
}

#[derive(Clone, Debug, Default, Serialize)]
pub struct RuleConcrete {
    pub crate_name: Option<String>,
    pub epoch: Option<Epoch>,
    pub crate_type: String,
    pub crate_root: String,
    pub sources: Vec<String>,
    pub inputs: Vec<String>,
    pub edition: String,
    pub cargo_pkg_version: String,
    pub cargo_pkg_authors: Option<String>,
    pub cargo_pkg_name: String,
    pub cargo_pkg_description: Option<String>,
    pub deps: Vec<DepGroup>,
    pub build_deps: Vec<DepGroup>,
    pub aliased_deps: Vec<(String, String)>,
    pub features: Vec<String>,
    pub build_root: Option<String>,
    pub build_script_sources: Vec<String>,
    pub build_script_inputs: Vec<String>,
    pub build_script_outputs: Vec<String>,
    pub extra_kv: HashMap<String, serde_json::Value>,
    /// Whether this rule depends on the main lib target in its group (e.g. a
    /// bin target alongside a lib inside a package).
    pub dep_on_lib: bool,
}

/// Describes a single GN build rule for a crate configuration. Each field
/// corresponds directly to a argument to the `cargo_crate()` template defined
/// in build/rust/cargo_crate.gni.
///
/// For undocumented fields, refer to the docs in the above file.
#[derive(Debug, Serialize)]
#[serde(rename_all = "snake_case")]
#[allow(clippy::large_enum_variant)]
pub enum Rule {
    Concrete {
        #[serde(flatten)]
        common: RuleCommon,
        #[serde(flatten)]
        details: RuleConcrete,
    },
    /// The rule is an alias to a different concrete rule.
    Group {
        #[serde(flatten)]
        common: RuleCommon,
        concrete_target: String,
    },
}

/// Set of rule dependencies with a shared condition.
#[derive(Clone, Debug, Serialize)]
pub struct DepGroup {
    /// `if` condition for GN, or `None` for unconditional deps.
    cond: Option<Condition>,
    /// Packages to depend on. The build file template determines the exact name
    /// based on the identified package and context.
    packages: Vec<PackageId>,
}

/// Extra metadata influencing GN output for a particular crate.
#[derive(Clone, Debug, Default)]
pub struct PerCrateMetadata {
    /// Names of files the build.rs script may output.
    pub build_script_outputs: Vec<String>,
    /// Extra GN code pasted literally into the build rule.
    pub gn_variables: Option<String>,
    /// GN target visibility.
    pub visibility: Visibility,
}

/// Generate `BuildFile` descriptions for each third party crate in the
/// dependency graph.
///
/// * `deps` is the result of dependency resolution from the `deps` module.
/// * `metadata` contains the package metadata for each third party crate.
/// * `build_script_outputs` is the list of files generated by the build.rs
///   script for each package.
/// * `deps_visibility` is the visibility for each package, defining if it can
///   be used outside of third-party code and outside of tests.
pub fn build_files_from_chromium_deps<'a, 'b, Iter, GetFiles>(
    deps: Iter,
    paths: &paths::ChromiumPaths,
    metadatas: &HashMap<VendoredCrate, PerCrateMetadata>,
    get_files: GetFiles,
) -> HashMap<VendoredCrate, BuildFile>
where
    Iter: IntoIterator<Item = &'a deps::Package>,
    GetFiles: Fn(&VendoredCrate) -> &'b CrateFiles,
{
    deps.into_iter()
        .filter_map(|dep| {
            let crate_id = dep.crate_id();
            let metadata = metadatas.get(&crate_id).cloned().unwrap_or_default();
            make_build_file_for_chromium_dep(dep, paths, get_files(&crate_id), metadata)
        })
        .collect()
}

pub fn build_file_from_std_deps<'a, 'b, Iter, GetFiles>(
    deps: Iter,
    paths: &'b paths::ChromiumPaths,
    extra_config: &'b BuildConfig,
    get_files: GetFiles,
) -> BuildFile
where
    Iter: IntoIterator<Item = &'a deps::Package>,
    GetFiles: Fn(&VendoredCrate) -> &'b CrateFiles,
{
    let rules = deps
        .into_iter()
        .map(|dep| {
            let crate_id = dep.crate_id();
            build_rule_from_std_dep(dep, paths, get_files(&crate_id), extra_config)
        })
        .collect();

    BuildFile { rules }
}

pub fn build_rule_from_std_dep(
    dep: &deps::Package,
    paths: &paths::ChromiumPaths,
    details: &CrateFiles,
    extra_config: &BuildConfig,
) -> (String, Rule) {
    let lib_target = dep.lib_target.as_ref().expect("dependency had no lib target");
    let crate_root_from_src = paths.to_gn_abs_path(&lib_target.root).unwrap();
    let build_script_from_src = dep.build_script.as_ref().map(|p| paths.to_gn_abs_path(p).unwrap());
    let cargo_pkg_authors =
        if dep.authors.is_empty() { None } else { Some(dep.authors.join(", ")) };

    // Get deps to exclude from resolved deps.
    let exclude_deps: Vec<String> = extra_config
        .per_crate_config
        .get(&*dep.package_name)
        .iter()
        .flat_map(|c| &c.exclude_deps_in_gn)
        .chain(&extra_config.all_config.exclude_deps_in_gn)
        .cloned()
        .collect();

    // Get the config's extra (key, value) pairs, which are passed as-is to the
    // build file template engine.
    let mut extra_kv = extra_config.all_config.extra_kv.clone();
    if let Some(per_crate) = extra_config.per_crate_config.get(&*dep.package_name) {
        extra_kv.extend(per_crate.extra_kv.iter().map(|(k, v)| (k.clone(), v.clone())));
    }

    let mut rule = RuleConcrete {
        crate_type: "rlib".to_string(),
        crate_root: format!("//{crate_root_from_src}"),
        sources: details
            .sources
            .iter()
            .map(|p| format!("//{}", paths.to_gn_abs_path(p).unwrap().to_string()))
            .collect(),
        inputs: details
            .inputs
            .iter()
            .map(|p| format!("//{}", paths.to_gn_abs_path(p).unwrap().to_string()))
            .collect(),
        edition: dep.edition.clone(),
        cargo_pkg_version: dep.version.to_string(),
        cargo_pkg_authors,
        cargo_pkg_name: dep.package_name.to_string(),
        cargo_pkg_description: dep.description.as_ref().map(|s| s.trim_end().to_string()),
        build_root: build_script_from_src.as_ref().map(|p| format!("//{p}")),
        build_script_sources: build_script_from_src
            .as_ref()
            .map(|p| format!("//{p}"))
            .into_iter()
            .chain(
                details
                    .build_script_sources
                    .iter()
                    .map(|p| format!("//{}", paths.to_gn_abs_path(p).unwrap().to_string())),
            )
            .collect(),
        build_script_inputs: details
            .build_script_inputs
            .iter()
            .map(|p| format!("//{}", paths.to_gn_abs_path(p).unwrap().to_string()))
            .collect(),
        extra_kv,
        ..Default::default()
    };

    rule.features = dep
        .dependency_kinds
        .get(&deps::DependencyKind::Normal)
        .map(|pki| pki.features.clone())
        .unwrap_or(vec![]);
    rule.features.sort_unstable();
    rule.features.dedup();

    // Add only normal and build dependencies: we don't run unit tests.
    let dep_deps: Vec<&DepOfDep> = dep
        .dependencies
        .iter()
        .filter(|d| !exclude_deps.iter().any(|e| e.as_str() == &*d.package_name))
        .collect();
    let build_deps: Vec<&DepOfDep> = dep
        .build_dependencies
        .iter()
        .filter(|d| !exclude_deps.iter().any(|e| e.as_str() == &*d.package_name))
        .collect();

    // Group the dependencies by condition, where the unconditional deps come
    // first. `group_deps` always returns at least one element, even if the
    // first set is empty.
    rule.deps = group_deps(&dep_deps, |d| PackageId {
        name: normalize_target_name(&d.package_name),
        epoch: None,
    });
    rule.build_deps = group_deps(&build_deps, |d| PackageId {
        name: normalize_target_name(&d.package_name),
        epoch: None,
    });

    for dep in dep_deps {
        let target_name = normalize_target_name(&dep.package_name);
        if target_name != dep.use_name {
            rule.aliased_deps.push((dep.use_name.clone(), format!(":{target_name}__rlib")));
        }
    }
    // TODO: No support for aliased_build_deps in the `cargo_crate` GN template as
    // there's been no usage needed.

    // If there are still no deps after `extra_deps`, simply clear the list.
    if rule.deps.len() == 1 && rule.deps[0].packages.len() == 0 {
        rule.deps.clear();
    }

    (
        normalize_target_name(&dep.package_name),
        Rule::Concrete {
            common: RuleCommon { testonly: false, public_visibility: true },
            details: rule,
        },
    )
}

/// Generate the `BuildFile` for `dep`, or return `None` if no rules would be
/// present.
fn make_build_file_for_chromium_dep(
    dep: &deps::Package,
    paths: &paths::ChromiumPaths,
    details: &CrateFiles,
    metadata: PerCrateMetadata,
) -> Option<(VendoredCrate, BuildFile)> {
    let crate_id = dep.crate_id();
    let crate_path_from_chromium_src =
        paths.third_party.join(ThirdPartySource::build_path(&crate_id));
    let crate_abs_path = paths.root.join(&crate_path_from_chromium_src);

    let to_gn_path = |abs_path: &Path| {
        abs_path.strip_prefix(&crate_abs_path).unwrap().to_string_lossy().into_owned()
    };

    let cargo_pkg_description = dep.description.as_ref().map(|s| s.trim_end().to_string());
    let cargo_pkg_authors =
        if dep.authors.is_empty() { None } else { Some(dep.authors.join(", ")) };

    // Template for all the rules in a build file. Several fields are
    // the same for all of a package's rules.
    let mut rule_template = RuleConcrete {
        edition: dep.edition.to_string(),
        cargo_pkg_version: dep.version.to_string(),
        cargo_pkg_authors,
        cargo_pkg_name: dep.package_name.clone(),
        cargo_pkg_description,
        build_root: dep.build_script.as_ref().map(|p| to_gn_path(p.as_path())),
        build_script_sources: dep
            .build_script
            .as_ref()
            .map(|p| to_gn_path(p.as_path()))
            .into_iter()
            .collect(),
        build_script_outputs: metadata.build_script_outputs,
        ..Default::default()
    };

    // Enumerate the dependencies of each kind for the package.
    for (gn_deps, cargo_deps) in [
        (&mut rule_template.deps, &dep.dependencies),
        (&mut rule_template.build_deps, &dep.build_dependencies),
    ] {
        let cargo_deps: Vec<_> = cargo_deps.iter().collect();

        // Group the dependencies by condition, where the unconditional deps come
        // first. `group_deps` always returns at least one element, even if the
        // first set is empty.
        *gn_deps = group_deps(&cargo_deps, |d| {
            let crate_id = d.crate_id();
            let normalized_name = crate_id.normalized_name().to_string();
            let epoch = Some(Epoch::from_version(&crate_id.version).to_string());
            PackageId { name: normalized_name, epoch }
        });

        // If there are still no deps after `extra_deps`, simply clear the list.
        if gn_deps.len() == 1 && gn_deps[0].packages.len() == 0 {
            gn_deps.clear();
        }
    }

    rule_template.aliased_deps.sort_unstable();

    let mut rules: Vec<(String, Rule)> = Vec::new();

    // Generate rules for each binary the package provides.
    for bin_target in &dep.bin_targets {
        let mut bin_rule = rule_template.clone();
        bin_rule.crate_type = "bin".to_string();
        bin_rule.crate_root = to_gn_path(bin_target.root.as_path());
        bin_rule.sources =
            details.sources.iter().map(|p| format!("//{}", p.display().to_string())).collect();
        bin_rule.inputs =
            details.inputs.iter().map(|p| format!("//{}", p.display().to_string())).collect();
        bin_rule.features = match dep.dependency_kinds.get(&deps::DependencyKind::Normal) {
            Some(per_kind_info) => per_kind_info.features.clone(),
            // As a hack, fill in empty feature set. This happens
            // because binary-only workspace members aren't the target
            // of any edge in the dependency graph: so, they have no
            // requested features.
            //
            // TODO(crbug.com/1291994): find a way to specify features
            // for these deps in third_party.toml.
            None => Vec::new(),
        };

        if dep.lib_target.is_some() {
            bin_rule.dep_on_lib = true;
            if bin_rule.deps.is_empty() {
                bin_rule.deps.push(DepGroup { cond: None, packages: Vec::new() });
            }
        }

        rules.push((
            NormalizedName::from_crate_name(&bin_target.name).to_string(),
            Rule::Concrete {
                common: RuleCommon { testonly: false, public_visibility: true },
                details: bin_rule,
            },
        ));
    }

    // Generate the rule for the main library target, if it exists.
    if let Some(lib_target) = &dep.lib_target {
        use deps::DependencyKind::*;
        // Generate the rules for each dependency kind. We use a stable
        // order instead of the hashmap iteration order.
        for dep_kind in [Normal, Build] {
            let per_kind_info = match dep.dependency_kinds.get(&dep_kind) {
                Some(x) => x,
                None => continue,
            };

            let lib_rule_name = match dep_kind {
                deps::DependencyKind::Normal => "lib",
                deps::DependencyKind::Build => "buildrs_support",
                _ => unreachable!(),
            }
            .to_string();

            let mut lib_details = rule_template.clone();
            lib_details.crate_name = Some(crate_id.normalized_name().to_string());
            lib_details.epoch = Some(Epoch::from_version(&crate_id.version));
            lib_details.crate_type = lib_target.lib_type.to_string();
            lib_details.crate_root = to_gn_path(lib_target.root.as_path());
            lib_details.sources =
                details.sources.iter().map(|p| format!("//{}", p.display().to_string())).collect();
            lib_details.inputs =
                details.inputs.iter().map(|p| format!("//{}", p.display().to_string())).collect();
            lib_details.features = per_kind_info.features.clone();

            let testonly = dep_kind == deps::DependencyKind::Development;
            let visibility = metadata.visibility;

            let lib_rule = Rule::Concrete {
                common: RuleCommon {
                    testonly,
                    public_visibility: match visibility {
                        Visibility::Public => true,
                        Visibility::ThirdParty | Visibility::TestOnlyAndThirdParty => false,
                    },
                },
                details: lib_details,
            };

            rules.push((lib_rule_name.clone(), lib_rule));

            // A package may be available to first-party tests. In this case limit :lib's
            // visibility to third-party but provide a testonly alias visible
            // everywhere. The :lib target must still exist for third-party
            // transitive deps, which aren't testonly.
            if dep_kind == Normal && visibility == Visibility::TestOnlyAndThirdParty {
                let test_support_rule = Rule::Group {
                    common: RuleCommon { testonly: true, public_visibility: true },
                    concrete_target: lib_rule_name,
                };
                rules.push(("test_support".to_string(), test_support_rule));
            }
        }
    }

    if rules.is_empty() { None } else { Some((crate_id, BuildFile { rules })) }
}

/// Group dependencies by condition, with unconditional deps first. The first
/// element is always present even if its set is empty.
fn group_deps<F: Fn(&DepOfDep) -> PackageId>(deps: &[&DepOfDep], target_name: F) -> Vec<DepGroup>
where
    F: Fn(&DepOfDep) -> PackageId,
{
    let mut groups = HashMap::<Option<Condition>, Vec<_>>::new();
    for dep in deps {
        let cond = match &dep.platform {
            None => None,
            Some(p) => Some(platform_to_condition(p)),
        };

        groups.entry(cond).or_default().push(target_name(dep));
    }

    groups.entry(None).or_default();

    let mut groups: Vec<DepGroup> =
        groups.into_iter().map(|(cond, rules)| DepGroup { cond, packages: rules }).collect();

    for group in groups.iter_mut() {
        group.packages.sort_unstable();
    }
    groups.sort_unstable_by(|l, r| l.cond.cmp(&r.cond));
    groups
}

fn normalize_target_name(package_name: &str) -> String {
    package_name.replace('-', "_")
}

pub fn escape_for_handlebars(x: &str) -> String {
    let mut out = String::new();
    for c in x.chars() {
        match c {
            // Note: we don't escape '$' here because we sometimes want to use
            // $var syntax.
            c @ ('"' | '\\') => write!(out, "\\{c}").unwrap(),
            // GN strings can encode literal ASCII with "$0x<hex_code>" syntax,
            // so we could embed newlines with "$0x0A". However, GN seems to
            // escape these incorrectly in its Ninja output so we just replace
            // it with a space.
            '\n' => out.push(' '),
            c => out.push(c),
        }
    }
    out
}

/// Describes a condition for some GN declaration.
#[derive(Clone, Debug, Hash, Eq, Ord, PartialEq, PartialOrd, Serialize)]
pub struct Condition(pub String);

impl Condition {
    pub fn from_platform_set(platforms: platforms::PlatformSet) -> Option<Self> {
        let platforms = match platforms {
            platforms::PlatformSet::All => return None,
            platforms::PlatformSet::Platforms(platforms) => platforms,
        };

        Some(Condition(
            platforms
                .iter()
                .map(|p| format!("({})", platform_to_condition(p).0))
                .collect::<Vec<_>>()
                .join(" || "),
        ))
    }
}

/// Map a cargo `Platform` constraint to a GN conditional expression.
pub fn platform_to_condition(platform: &platforms::Platform) -> Condition {
    Condition(match platform {
        platforms::Platform::Name(triple) => triple_to_condition(triple).to_string(),
        platforms::Platform::Cfg(cfg_expr) => cfg_expr_to_condition(cfg_expr),
    })
}

pub fn cfg_expr_to_condition(cfg_expr: &cargo_platform::CfgExpr) -> String {
    match cfg_expr {
        cargo_platform::CfgExpr::Not(expr) => {
            format!("!({})", cfg_expr_to_condition(expr))
        }
        cargo_platform::CfgExpr::All(exprs) => {
            let mut conds = exprs
                .iter()
                .map(|expr| format!("({})", cfg_expr_to_condition(expr)))
                .collect::<Vec<String>>();
            conds.sort();
            conds.dedup();
            conds.join(" && ")
        }
        cargo_platform::CfgExpr::Any(exprs) => {
            let mut conds = exprs
                .iter()
                .map(|expr| format!("({})", cfg_expr_to_condition(expr)))
                .collect::<Vec<String>>();
            conds.sort();
            conds.dedup();
            conds.join(" || ")
        }
        cargo_platform::CfgExpr::Value(cfg) => cfg_to_condition(cfg),
    }
}

pub fn cfg_to_condition(cfg: &cargo_platform::Cfg) -> String {
    match cfg {
        cargo_platform::Cfg::Name(name) => match name.as_str() {
            // Note that while Fuchsia is not a unix, rustc sets the unix cfg
            // anyway. We must be consistent with rustc. This may change with
            // https://github.com/rust-lang/rust/issues/58590
            "unix" => "!is_win",
            "windows" => "is_win",
            _ => unreachable!(),
        },
        cargo_platform::Cfg::KeyPair(key, value) => {
            assert_eq!(key, "target_os");
            target_os_to_condition(value)
        }
    }
    .to_string()
}

fn triple_to_condition(triple: &str) -> &'static str {
    for (t, c) in TRIPLE_TO_GN_CONDITION {
        if *t == triple {
            return c;
        }
    }

    panic!("target triple {triple} not found")
}

fn target_os_to_condition(target_os: &str) -> &'static str {
    for (t, c) in TARGET_OS_TO_GN_CONDITION {
        if *t == target_os {
            return c;
        }
    }

    panic!("target os {target_os} not found")
}

static TRIPLE_TO_GN_CONDITION: &[(&str, &str)] = &[
    ("i686-linux-android", "is_android && target_cpu == \"x86\""),
    ("x86_64-linux-android", "is_android && target_cpu == \"x64\""),
    ("armv7-linux-android", "is_android && target_cpu == \"arm\""),
    ("aarch64-linux-android", "is_android && target_cpu == \"arm64\""),
    ("aarch64-fuchsia", "is_fuchsia && target_cpu == \"arm64\""),
    ("x86_64-fuchsia", "is_fuchsia && target_cpu == \"x64\""),
    ("aarch64-apple-ios", "is_ios && target_cpu == \"arm64\""),
    ("armv7-apple-ios", "is_ios && target_cpu == \"arm\""),
    ("x86_64-apple-ios", "is_ios && target_cpu == \"x64\""),
    ("i386-apple-ios", "is_ios && target_cpu == \"x86\""),
    ("i686-pc-windows-msvc", "is_win && target_cpu == \"x86\""),
    ("x86_64-pc-windows-msvc", "is_win && target_cpu == \"x64\""),
    ("i686-unknown-linux-gnu", "(is_linux || is_chromeos) && target_cpu == \"x86\""),
    ("x86_64-unknown-linux-gnu", "(is_linux || is_chromeos) && target_cpu == \"x64\""),
    ("x86_64-apple-darwin", "is_mac && target_cpu == \"x64\""),
    ("aarch64-apple-darwin", "is_mac && target_cpu == \"arm64\""),
];

static TARGET_OS_TO_GN_CONDITION: &[(&str, &str)] = &[
    ("android", "is_android"),
    ("darwin", "is_mac"),
    ("fuchsia", "is_fuchsia"),
    ("ios", "is_ios"),
    ("linux", "is_linux || is_chromeos"),
    ("windows", "is_win"),
];

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn platform_to_condition() {
        use crate::platforms::{Platform, PlatformSet};
        use cargo_platform::CfgExpr;
        use std::str::FromStr;

        // Try an unconditional filter.
        assert_eq!(Condition::from_platform_set(PlatformSet::one(None)), None);

        // Try a target triple.
        assert_eq!(
            Condition::from_platform_set(PlatformSet::one(Some(Platform::Name(
                "x86_64-pc-windows-msvc".to_string()
            ))))
            .unwrap()
            .0,
            "(is_win && target_cpu == \"x64\")"
        );

        // Try a cfg expression.
        assert_eq!(
            Condition::from_platform_set(PlatformSet::one(Some(Platform::Cfg(
                CfgExpr::from_str("any(windows, target_os = \"android\")").unwrap()
            ))))
            .unwrap()
            .0,
            "((is_android) || (is_win))"
        );

        // Redundant cfg expression.
        assert_eq!(
            Condition::from_platform_set(PlatformSet::one(Some(Platform::Cfg(
                CfgExpr::from_str("any(windows, windows)").unwrap()
            ))))
            .unwrap()
            .0,
            "((is_win))"
        );

        // Try a PlatformSet with multiple filters.
        let mut platform_set = PlatformSet::empty();
        platform_set.add(Some(Platform::Name("armv7-linux-android".to_string())));
        platform_set.add(Some(Platform::Cfg(CfgExpr::from_str("windows").unwrap())));
        assert_eq!(
            Condition::from_platform_set(platform_set).unwrap().0,
            "(is_android && target_cpu == \"arm\") || (is_win)"
        );
    }

    #[test]
    fn string_excaping() {
        assert_eq!("foo bar", format!("{}", escape_for_handlebars("foo bar")));
        assert_eq!("foo bar ", format!("{}", escape_for_handlebars("foo\nbar\n")));
        assert_eq!(r#"foo \"bar\""#, format!("{}", escape_for_handlebars(r#"foo "bar""#)));
        assert_eq!("foo 'bar'", format!("{}", escape_for_handlebars("foo 'bar'")));
    }
}
