#!/usr/bin/env bash
set -euo pipefail

PIXY_REPO_DIR="${HOME}/pixy"
WRAPPER_PATH="/usr/local/bin/pixy-g++"

echo "[1/7] Installing dependencies..."
sudo apt update
sudo apt install -y \
  git \
  g++ \
  cmake \
  pkg-config \
  libusb-1.0-0-dev \
  libboost-all-dev

echo "[2/7] Cloning Pixy repo..."
if [ ! -d "${PIXY_REPO_DIR}/.git" ]; then
  git clone https://github.com/charmedlabs/pixy.git "${PIXY_REPO_DIR}"
else
  git -C "${PIXY_REPO_DIR}" pull --ff-only
fi

echo "[3/7] Building libpixyusb..."
cd "${PIXY_REPO_DIR}/scripts"
chmod +x ./*.sh
./build_libpixyusb.sh

echo "[4/7] Installing libpixyusb..."
sudo ./install_libpixyusb.sh

echo "[5/7] Refreshing linker cache..."
sudo ldconfig

echo "[6/7] Detecting Pixy include/lib paths..."
PIXY_INCLUDE_DIR=""
PIXY_LIB_DIR=""

if [ -f /usr/local/include/pixy.h ]; then
  PIXY_INCLUDE_DIR="/usr/local/include"
elif [ -f /usr/local/include/libpixyusb/pixy.h ]; then
  PIXY_INCLUDE_DIR="/usr/local/include/libpixyusb"
elif [ -f /usr/include/pixy.h ]; then
  PIXY_INCLUDE_DIR="/usr/include"
elif [ -f /usr/include/libpixyusb/pixy.h ]; then
  PIXY_INCLUDE_DIR="/usr/include/libpixyusb"
else
  FOUND_HEADER="$(find /usr /usr/local -name pixy.h 2>/dev/null | head -n 1 || true)"
  if [ -n "${FOUND_HEADER}" ]; then
    PIXY_INCLUDE_DIR="$(dirname "${FOUND_HEADER}")"
  fi
fi

if [ -f /usr/local/lib/libpixyusb.a ] || [ -f /usr/local/lib/libpixyusb.so ]; then
  PIXY_LIB_DIR="/usr/local/lib"
elif [ -f /usr/lib/libpixyusb.a ] || [ -f /usr/lib/libpixyusb.so ]; then
  PIXY_LIB_DIR="/usr/lib"
else
  FOUND_LIB="$(find /usr /usr/local \( -name 'libpixyusb.a' -o -name 'libpixyusb.so' \) 2>/dev/null | head -n 1 || true)"
  if [ -n "${FOUND_LIB}" ]; then
    PIXY_LIB_DIR="$(dirname "${FOUND_LIB}")"
  fi
fi

if [ -z "${PIXY_INCLUDE_DIR}" ] || [ -z "${PIXY_LIB_DIR}" ]; then
  echo "[ERR] Could not find pixy.h or libpixyusb after install."
  echo "pixy.h found at:"
  find /usr /usr/local -name pixy.h 2>/dev/null || true
  echo "libpixyusb found at:"
  find /usr /usr/local \( -name 'libpixyusb.a' -o -name 'libpixyusb.so' \) 2>/dev/null || true
  exit 1
fi

echo "  pixy include dir: ${PIXY_INCLUDE_DIR}"
echo "  pixy lib dir:     ${PIXY_LIB_DIR}"

echo "[7/7] Creating global compiler wrapper at ${WRAPPER_PATH} ..."
sudo tee "${WRAPPER_PATH}" > /dev/null <<EOF
#!/usr/bin/env bash
set -e
exec g++ -std=c++11 -Wno-psabi \\
  -I"${PIXY_INCLUDE_DIR}" \\
  -I/usr/include/libusb-1.0 \\
  -L"${PIXY_LIB_DIR}" \\
  "\$@" \\
  -lpixyusb -lusb-1.0 -lboost_thread -lboost_system -lboost_chrono -lpthread
EOF

sudo chmod +x "${WRAPPER_PATH}"

echo
echo "[DONE] Pixy environment is ready."
