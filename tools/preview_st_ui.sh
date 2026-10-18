#!/usr/bin/env bash
# Headless LVGL 9.5 render/layout/heap verification. Does not access devices.
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
preview_output="${1:-${repo_root}/build/st-ui-preview}"
if [[ ! -d "${repo_root}/managed_components/lvgl__lvgl" ]]; then
    echo 'Run the ESP-IDF firmware build first to obtain the pinned LVGL component.' >&2
    exit 1
fi
mkdir -p "${preview_output}"
cmake -S "${repo_root}/tests/lvgl_st_preview" -B "${preview_output}/build" -G Ninja
cmake --build "${preview_output}/build" --parallel 4
"${preview_output}/build/preview" "${preview_output}"
