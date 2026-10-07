#!/usr/bin/env bash
# Build first; shared staging owns payload validation and final executable binding.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
[[ $# -ge 3 ]] || {
  echo "usage: $0 <build-dir> <artifact-tag> <emitter-build-dir> [shared-stage-args...]" >&2
  exit 2
}
build=$1 artifact=$2 emitter=$3
shift 3
framework=${PSXRECOMP_ROOT:-$root/psxrecomp}
exec bash "$framework/tools/package_game_release.sh" \
  --root "$root" --build-dir "$build" --artifact "$artifact" \
  --zip-prefix medievil-recomp --exe-name MediEvil_Recompiled \
  --display-name "MediEvil Recompiled" --recompiler-build "$emitter" \
  --disc-hint "your legally owned USA MediEvil disc" \
  --doc docs/PARITY_QUALIFICATION.md "$@"
