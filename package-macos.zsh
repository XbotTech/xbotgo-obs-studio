#!/usr/bin/env zsh

builtin emulate -L zsh
setopt ERR_EXIT
setopt NO_UNSET
setopt PIPE_FAIL

readonly script_name=${0:t}
readonly project_root=${0:A:h}
readonly build_dir=${project_root}/build_macos
readonly cache_dir=${build_dir}/CompilationCache.noindex
readonly build_script=${project_root}/.github/scripts/build-macos
readonly package_script=${project_root}/.github/scripts/package-macos

typeset config=Release
typeset -i codesign=0
typeset -i notarize=0
typeset -i skip_build=0

usage() {
  print "Usage: ${script_name} [options]"
  print
  print 'Build, package, and verify the arm64 macOS application DMG.'
  print
  print 'Options:'
  print '  --codesign          Use Developer ID signing credentials from the environment.'
  print '  --notarize          Sign and notarize the DMG (implies --codesign).'
  print '  --skip-build        Package the existing build_macos/OBS.app.'
  print '  --config <name>     Build configuration (default: Release).'
  print '  -h, --help          Show this help message.'
}

fail() {
  print -u2 "Error: ${1}"
  exit 1
}

require_command() {
  command -v "${1}" >/dev/null 2>&1 || fail "Required command not found: ${1}"
}

require_environment() {
  local variable_name=${1}
  [[ -n ${(P)variable_name:-} ]] || fail "${variable_name} must be set; see docs/macos-packaging.md."
}

while (( $# > 0 )); do
  case ${1} in
    --codesign)
      codesign=1
      shift
      ;;
    --notarize)
      codesign=1
      notarize=1
      shift
      ;;
    --skip-build)
      skip_build=1
      shift
      ;;
    --config)
      (( $# >= 2 )) || fail 'Missing value for --config.'
      config=${2}
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "Unknown option: ${1}"
      ;;
  esac
done

case ${config} in
  Debug|RelWithDebInfo|Release|MinSizeRel) ;;
  *) fail "Unsupported build configuration: ${config}" ;;
esac

autoload -Uz is-at-least
is-at-least 5.9 || fail "Zsh 5.9 or newer is required; found ${ZSH_VERSION}."
[[ $(uname -s) == Darwin ]] || fail 'This script must be run on macOS.'
[[ $(uname -m) == arm64 ]] || fail 'Only Apple Silicon (arm64) is supported.'

require_command cmake
require_command codesign
require_command file
require_command git
require_command hdiutil
require_command xcodebuild
[[ -x ${build_script} ]] || fail "Build script is missing or not executable: ${build_script}"
[[ -x ${package_script} ]] || fail "Package script is missing or not executable: ${package_script}"

if (( codesign )); then
  require_environment CODESIGN_IDENT
  require_environment CODESIGN_TEAM
  require_environment PROVISIONING_PROFILE
  [[ ${CODESIGN_IDENT} != '-' ]] || fail 'CODESIGN_IDENT must be a Developer ID Application identity.'
fi

if (( notarize )); then
  require_command spctl
  require_command xcrun
  require_environment CODESIGN_IDENT_USER
  require_environment CODESIGN_IDENT_PASS
fi

cd ${project_root}

if (( ! skip_build )); then
  print '==> Initializing Git submodules'
  git submodule update --init --recursive

  print "==> Building macOS arm64 application (${config})"
  mkdir -p ${cache_dir}
  typeset -a build_args=(--target macos-arm64 --config ${config})
  (( codesign )) && build_args+=(--codesign)
  env CI=1 GITHUB_EVENT_NAME=workflow_dispatch GITHUB_REF_NAME=local "XCODE_CAS_PATH=${cache_dir}" \
    ${build_script} ${build_args}
else
  [[ -d ${build_dir}/OBS.app ]] || fail 'build_macos/OBS.app does not exist; remove --skip-build and try again.'
  print '==> Reusing build_macos/OBS.app'
fi

print '==> Creating macOS arm64 DMG'
typeset -a package_args=(--target macos-arm64 --config ${config} --package)
(( codesign )) && package_args+=(--codesign)
(( notarize )) && package_args+=(--notarize)
env CI=1 ${package_script} ${package_args}

typeset -a dmg_files=(${build_dir}/obs-studio-*-macos-apple.dmg(N.om))
(( ${#dmg_files} > 0 )) || fail 'Packaging completed without producing a macOS Apple DMG.'
readonly package_dmg_path=${dmg_files[1]}
readonly package_date=$(date +%Y%m%d)
readonly package_dmg_name=${package_dmg_path:t}
readonly dmg_name_prefix=${package_dmg_name%-macos-apple.dmg}
typeset dmg_path=${build_dir}/${dmg_name_prefix}-${package_date}-macos-apple.dmg
typeset -i duplicate_number=2
while [[ -e ${dmg_path} ]]; do
  dmg_path=${build_dir}/${dmg_name_prefix}-${package_date}-${duplicate_number}-macos-apple.dmg
  (( ++duplicate_number ))
done
mv ${package_dmg_path} ${dmg_path}
readonly dmg_path

readonly app_path=${build_dir}/OBS.app
readonly app_binary=${app_path}/Contents/MacOS/OBS
readonly falconm_plugin=${app_path}/Contents/PlugIns/xbotogo-falconM.plugin
readonly media_sdk=${falconm_plugin}/Contents/Frameworks/libmedia_sdk.1.0.0.dylib

print "==> DMG renamed with package date: ${dmg_path:t}"
print '==> Verifying DMG and application bundle'
hdiutil verify ${dmg_path}
codesign --verify --deep --strict --verbose=2 ${app_path}

[[ -f ${app_binary} ]] || fail "Application executable is missing: ${app_binary}"
typeset app_architecture=$(file ${app_binary})
print ${app_architecture}
[[ ${app_architecture} == *arm64* ]] || fail 'The application executable is not arm64.'

[[ -d ${falconm_plugin} ]] || fail "FalconM plugin is missing: ${falconm_plugin}"
[[ -f ${media_sdk} ]] || fail "FalconM Media SDK is missing: ${media_sdk}"
typeset sdk_architecture=$(file ${media_sdk})
print ${sdk_architecture}
[[ ${sdk_architecture} == *arm64* ]] || fail 'FalconM Media SDK is not arm64.'

if (( notarize )); then
  print '==> Verifying notarization and Gatekeeper acceptance'
  xcrun stapler validate ${dmg_path}
  spctl -a -vv -t open --context context:primary-signature ${dmg_path}
fi

print
print "DMG created successfully: ${dmg_path}"
