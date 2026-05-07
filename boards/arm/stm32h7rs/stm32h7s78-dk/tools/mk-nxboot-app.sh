#!/usr/bin/env bash
############################################################################
# boards/arm/stm32h7rs/stm32h7s78-dk/tools/mk-nxboot-app.sh
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

set -euo pipefail

usage()
{
  cat <<EOF
Usage: $0 [OPTIONS]

Create an STM32H7S78-DK NXboot application image:

  [NXboot header][NuttX app binary]

The result is intended to be programmed at XSPI2 NOR address 0x70000000.
Run this after building stm32h7s78-dk:nxboot-app.

Options:
  -i, --input PATH        Input app binary (default: <nuttx>/nuttx.bin)
  -o, --output PATH       Output image (default: <nuttx>/nuttx-nxboot-app.bin)
  -v, --version VERSION   Semantic version (default: 0.1.0)
      --header-size SIZE  Header size (default: .config or 0x400)
      --identifier ID     Platform identifier (default: .config or 0x48735378)
      --force             Allow packaging even if .config is nxboot-loader
  -h, --help              Show this help
EOF
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
nuttx_root="$(cd "${script_dir}/../../../../.." && pwd)"
apps_root="$(cd "${nuttx_root}/../apps" && pwd)"

input="${nuttx_root}/nuttx.bin"
output="${nuttx_root}/nuttx-nxboot-app.bin"
version="0.1.0"
header_size=""
identifier=""
force=0

config_value()
{
  local key="$1"
  local file="${nuttx_root}/.config"

  if [[ -f "${file}" ]]; then
    sed -n "s/^${key}=//p" "${file}" | tail -n 1
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--input)
      input="$2"
      shift 2
      ;;
    -o|--output)
      output="$2"
      shift 2
      ;;
    -v|--version)
      version="$2"
      shift 2
      ;;
    --header-size)
      header_size="$2"
      shift 2
      ;;
    --identifier)
      identifier="$2"
      shift 2
      ;;
    --force)
      force=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ -z "${header_size}" ]]; then
  header_size="$(config_value CONFIG_NXBOOT_HEADER_SIZE)"
  header_size="${header_size:-0x400}"
fi

if [[ -z "${identifier}" ]]; then
  identifier="$(config_value CONFIG_NXBOOT_PLATFORM_IDENTIFIER)"
  identifier="${identifier:-0x48735378}"
fi

if [[ ! -f "${input}" ]]; then
  echo "ERROR: input image not found: ${input}" >&2
  echo "Build stm32h7s78-dk:nxboot-app first." >&2
  exit 1
fi

if [[ ! -f "${apps_root}/boot/nxboot/tools/nximage.py" ]]; then
  echo "ERROR: NXboot image tool not found under apps: ${apps_root}" >&2
  exit 1
fi

if [[ -f "${nuttx_root}/.config" ]] &&
   grep -q '^CONFIG_NXBOOT_BOOTLOADER=y$' "${nuttx_root}/.config" &&
   [[ "${force}" -eq 0 ]]; then
  echo "ERROR: current .config is nxboot-loader, not nxboot-app." >&2
  echo "Run ./tools/configure.sh stm32h7s78-dk:nxboot-app && make first." >&2
  echo "Use --force only if you intentionally want to package this binary." >&2
  exit 1
fi

if python3 -c 'import semantic_version' >/dev/null 2>&1; then
  python3 "${apps_root}/boot/nxboot/tools/nximage.py" \
    --version "${version}" \
    --header_size "${header_size}" \
    --identifier "${identifier}" \
    "${input}" \
    "${output}"
else
  echo "WARNING: Python module 'semantic_version' is not installed." >&2
  echo "WARNING: using built-in NXboot header fallback." >&2

  python3 - "${input}" "${output}" "${version}" "${header_size}" \
    "${identifier}" <<'PY'
import io
import os
import re
import struct
import sys
import zlib

src_path, dst_path, version, header_size, identifier = sys.argv[1:6]
header_size = int(header_size, 0)
identifier = int(identifier, 0)

if header_size < 128:
    raise SystemExit("ERROR: NXboot header size must be at least 128 bytes")

match = re.fullmatch(r"([0-9]+)\.([0-9]+)\.([0-9]+)(?:-([0-9A-Za-z.-]+))?",
                     version)
if not match:
    raise SystemExit("ERROR: version must be MAJOR.MINOR.PATCH[-prerelease]")

major, minor, patch = (int(match.group(i)) for i in range(1, 4))
prerelease = (match.group(4) or "").encode("utf-8")
if len(prerelease) > 94:
    raise SystemExit("ERROR: NXboot prerelease string is longer than 94 bytes")

size = os.stat(src_path).st_size
crc = 0

with open(src_path, "rb") as src, open(dst_path, "wb") as dst:
    dst.write(b"\x4e\x58\x4f\x53")
    dst.write(struct.pack("<H", 0))
    dst.write(struct.pack("<H", header_size))
    dst.write(struct.pack("<I", 0xffffffff))
    dst.write(struct.pack("<I", size))
    dst.write(struct.pack("<Q", identifier))
    dst.write(struct.pack("<I", 0))
    dst.write(struct.pack("<H", major))
    dst.write(struct.pack("<H", minor))
    dst.write(struct.pack("<H", patch))
    dst.write(struct.pack("@94s", prerelease))
    dst.write(bytearray(b"\xff") * (header_size - 128))

    while True:
        data = src.read(io.DEFAULT_BUFFER_SIZE)
        if not data:
            break
        dst.write(data)

with open(dst_path, "r+b") as image:
    image.seek(12)
    while True:
        data = image.read(io.DEFAULT_BUFFER_SIZE)
        if not data:
            break
        crc = zlib.crc32(data, crc)
    image.seek(8)
    image.write(struct.pack("<I", crc))
PY
fi

printf 'Created NXboot app image:\n'
printf '  input:       %s\n' "${input}"
printf '  output:      %s\n' "${output}"
printf '  version:     %s\n' "${version}"
printf '  header_size: %s\n' "${header_size}"
printf '  identifier:  %s\n' "${identifier}"
printf '  program at:  0x70000000\n'
