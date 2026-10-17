#!/usr/bin/env bash
# Add this script to the Collabora source checkout under scripts/.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
target="$root/engine/translations"
commit=17fb3698d5e86169d44076f7a992c572f8f780f1
mirror=https://github.com/LibreOffice/translations.git
if test -d "$target/.git"; then
 test "$(git -C "$target" rev-parse HEAD)" = "$commit" || { echo 'Existing translations use a different revision; inspect before replacing.' >&2; exit 1; }
else
 test ! -e "$target" || { echo 'Translations directory exists; refusing to overwrite.' >&2; exit 1; }
 git init "$target"
 git -C "$target" remote add origin "$mirror"
 git -C "$target" fetch --depth=1 --filter=blob:none origin "$commit"
 git -C "$target" sparse-checkout init --cone
 git -C "$target" sparse-checkout set source/zh-CN
 git -C "$target" checkout --detach FETCH_HEAD
fi
test -s "$target/source/zh-CN/officecfg/registry/data/org/openoffice/Office/UI.po"
printf '%s\n' "Chinese translation source ready: $commit"
