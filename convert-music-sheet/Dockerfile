# Use Python 3.10 to prevent NumPy 2.0 / Python 3.12 compatibility issues
FROM python:3.10-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
	poppler-utils \
	libgl1 \
	libglib2.0-0 \
	ffmpeg \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Upgrade pip and pin compatible libraries
RUN pip install --no-cache-dir --upgrade pip && \
	pip install --no-cache-dir \
	"numpy<2.0.0" \
	"scipy<1.14.0" \
	"opencv-python-headless<4.10.0" \
	tensorflow \
	tf-keras \
	oemer

# Pre-download oemer model weights during build so execution is offline-ready
RUN python -c "import oemer.checkpoints" || true

COPY entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh

# Environment flags for Keras / CUDA CPU fallback
ENV TF_USE_LEGACY_KERAS=1
ENV CUDA_VISIBLE_DEVICES=""

ENTRYPOINT ["/app/entrypoint.sh"]
