#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "usage: $0 <nuttx.bin> <output.img> [version]" >&2
  exit 64
fi

input=$1
output=$2
version=${3:-1.0.0}

if [ ! -f "$input" ]; then
  echo "error: input image not found: $input" >&2
  exit 66
fi

output_dir=$(dirname "$output")
if [ ! -d "$output_dir" ]; then
  echo "error: output directory not found: $output_dir" >&2
  exit 73
fi

script_dir=$(cd "$(dirname "$0")" && pwd)
topdir=$(cd "$script_dir/../../../../.." && pwd)
appsdir=$(cd "$topdir/../apps" && pwd)
nximage="$appsdir/boot/nxboot/tools/nximage.py"

if python3 -c 'import semantic_version' >/dev/null 2>&1; then
  if [ ! -f "$nximage" ]; then
    echo "error: nximage.py not found: $nximage" >&2
    exit 69
  fi

  python3 "$nximage" \
    --header_size 0x400 \
    --identifier 0x4e363537 \
    --version "$version" \
    "$input" "$output"
else
  echo "warning: python semantic_version module not found; using local NXboot header writer" >&2
  python3 - "$input" "$output" "$version" <<'PY'
import re
import struct
import sys
import zlib

source, output, version = sys.argv[1:4]
match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)(?:[-+].*)?", version)
if not match:
    raise SystemExit(f"error: version must start with MAJOR.MINOR.PATCH: {version}")

major, minor, patch = (int(value) for value in match.groups())
with open(source, "rb") as src:
    payload = src.read()

header_size = 0x400
identifier = 0x4E363537
header = bytearray()
header += b"NXOS"
header += struct.pack("<H", 0)
header += struct.pack("<H", header_size)
header += struct.pack("<I", 0xFFFFFFFF)
header += struct.pack("<I", len(payload))
header += struct.pack("<Q", identifier)
header += struct.pack("<I", 0)
header += struct.pack("<H", major)
header += struct.pack("<H", minor)
header += struct.pack("<H", patch)
header += struct.pack("94s", b"")
header += b"\xff" * (header_size - len(header))

image = header + payload
crc = zlib.crc32(image[12:]) & 0xFFFFFFFF
struct.pack_into("<I", image, 8, crc)

with open(output, "wb") as dest:
    dest.write(image)
PY
fi

python3 - "$output" <<'PY'
import struct
import sys

path = sys.argv[1]
with open(path, "rb") as f:
    data = f.read(0x408)

if len(data) < 0x408:
    raise SystemExit(f"error: image too small: {path}")

magic, flags, header_size, crc, image_size, identifier = struct.unpack_from(
    "<4sHHIIQ", data, 0
)
msp, reset = struct.unpack_from("<II", data, header_size)
reset_addr = reset & ~1

if magic != b"NXOS":
    raise SystemExit(f"error: bad NXboot magic: {magic!r}")

if header_size != 0x400:
    raise SystemExit(f"error: bad NXboot header size: 0x{header_size:x}")

if identifier != 0x4E363537:
    raise SystemExit(f"error: bad platform identifier: 0x{identifier:x}")

if not (0x34000000 <= msp < 0x34200000):
    raise SystemExit(f"error: MSP outside STM32N6570 app SRAM: 0x{msp:08x}")

if not (0x70100400 <= reset_addr < 0x72100000):
    raise SystemExit(
        f"error: reset vector outside primary XIP slot: 0x{reset:08x}"
    )

print(f"NXboot header: size=0x{header_size:x} crc=0x{crc:08x} "
      f"image_size={image_size} identifier=0x{identifier:08x}")
print(f"App vector: msp=0x{msp:08x} reset=0x{reset:08x}")
PY

echo "NXboot app image: $output"
echo "Burn address: 0x70100000"
