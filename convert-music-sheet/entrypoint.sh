#!/bin/bash

# docker run --rm -v ~/Downloads:/data sheet2xml:latest '/data/Rain_on_brick-2.pdf' /data/output

set -e

INPUT_PATH="$1"
OUTPUT_DIR="${2:-/data/output}"

if [ -z "$INPUT_PATH" ]; then
	echo "Error: No input file provided."
	echo "Usage: docker run -v \$(pwd):/data sheet2xml /data/your_file.pdf"
	exit 1
fi

mkdir -p /tmp/omr_pages "$OUTPUT_DIR"

# Detect file extension
EXT="${INPUT_PATH##*.}"
EXT_LOWER=$(echo "$EXT" | tr '[:upper:]' '[:lower:]')

# If PDF, extract page 1 to PNG at 200 DPI
if [ "$EXT_LOWER" = "pdf" ]; then
	echo "[1/2] Converting PDF to PNG..."
	pdftoppm -png -r 200 "$INPUT_PATH" /tmp/omr_pages/page
	TARGET_IMG="/tmp/omr_pages/page-1.png"
else
	TARGET_IMG="$INPUT_PATH"
fi

echo "[2/2] Running OMR processing..."
TF_USE_LEGACY_KERAS=1 CUDA_VISIBLE_DEVICES="" oemer "$TARGET_IMG" --use-tf -o "$OUTPUT_DIR"

echo "Done! Output saved to $OUTPUT_DIR"
