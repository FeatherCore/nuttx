#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "usage: $0 <STM32_SigningTool_CLI> <fsbl.bin> <output.bin>" >&2
  exit 64
fi

signing_tool=$1
input=$2
output=$3

if [ ! -x "$signing_tool" ]; then
  echo "error: signing tool is not executable: $signing_tool" >&2
  exit 69
fi

if [ ! -f "$input" ]; then
  echo "error: FSBL image not found: $input" >&2
  exit 66
fi

input_size=$(stat -c '%s' "$input")
max_size=$((0x7fc00))
if [ "$input_size" -gt "$max_size" ]; then
  printf 'error: FSBL image too large: %s bytes > %s bytes\n' \
    "$input_size" "$max_size" >&2
  exit 75
fi

output_dir=$(dirname "$output")
if [ ! -d "$output_dir" ]; then
  echo "error: output directory not found: $output_dir" >&2
  exit 73
fi

"$signing_tool" \
  -bin "$input" \
  -nk \
  -of 0x80000000 \
  -t fsbl \
  -hv 2.3 \
  -align \
  -o "$output"

echo "FSBL payload size: $input_size bytes / $max_size bytes"
echo "Trusted FSBL image: $output"
echo "Burn address: 0x70000000"
