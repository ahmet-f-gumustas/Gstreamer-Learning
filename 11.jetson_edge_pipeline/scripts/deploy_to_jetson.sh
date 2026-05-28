#!/usr/bin/env bash
# Cross-compile edilmiş binary'i + config'leri SSH üzerinden Jetson'a kopyala.
# Kullanım:  ./deploy_to_jetson.sh 192.168.1.42 nvidia
set -euo pipefail

HOST="${1:?usage: $0 <jetson-ip> <user> [remote-dir]}"
USER="${2:?usage: $0 <jetson-ip> <user> [remote-dir]}"
REMOTE_DIR="${3:-/home/$USER/jetson_edge}"

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJECT_DIR/build_aarch64/jetson_edge"

if [[ ! -x "$BIN" ]]; then
    echo "Cross-compiled binary yok. Önce scripts/cross_compile.sh çalıştır."
    exit 1
fi

echo "=== $USER@$HOST:$REMOTE_DIR dizinine deploy ==="

ssh "$USER@$HOST" "mkdir -p $REMOTE_DIR/{bin,config,models,scripts,logs}"

scp "$BIN"                                  "$USER@$HOST:$REMOTE_DIR/bin/"
scp -r "$PROJECT_DIR/config"/*              "$USER@$HOST:$REMOTE_DIR/config/"
scp -r "$PROJECT_DIR/scripts"/*.sh          "$USER@$HOST:$REMOTE_DIR/scripts/"
scp -r "$PROJECT_DIR/scripts"/*.py          "$USER@$HOST:$REMOTE_DIR/scripts/" || true

# Engine dosyaları ayrı: TensorRT engine GPU-specific olduğu için Jetson'da
# yeniden build edilmeli.  ONNX'i gönder, calibration cache'i de gönder.
if [[ -d "$PROJECT_DIR/models" ]]; then
    for f in "$PROJECT_DIR/models"/*.onnx "$PROJECT_DIR/models"/*.cache; do
        [[ -e "$f" ]] || continue
        scp "$f" "$USER@$HOST:$REMOTE_DIR/models/"
    done
fi

# systemd unit (boot'ta auto-start için)
cat > /tmp/jetson-edge.service <<EOF
[Unit]
Description=Jetson Edge Pipeline
After=network.target nvargus-daemon.service

[Service]
Type=simple
User=$USER
WorkingDirectory=$REMOTE_DIR
ExecStart=$REMOTE_DIR/bin/jetson_edge --config $REMOTE_DIR/config/pipeline.yaml --headless
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
scp /tmp/jetson-edge.service "$USER@$HOST:/tmp/"
ssh "$USER@$HOST" "sudo mv /tmp/jetson-edge.service /etc/systemd/system/ && sudo systemctl daemon-reload"

echo
echo "=== Deploy tamam ==="
echo "Jetson'da çalıştırmak için:"
echo "  ssh $USER@$HOST"
echo "  cd $REMOTE_DIR"
echo "  python3 scripts/build_int8_engine.py --model yolov8n.pt --precision int8 \\"
echo "          --output models/yolov8n_int8.engine"
echo "  ./bin/jetson_edge --config config/pipeline.yaml"
echo
echo "Boot'ta auto-start: sudo systemctl enable jetson-edge"
