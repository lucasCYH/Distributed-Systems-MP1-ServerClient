#!/usr/bin/env bash
# usage: ./cluster.sh {deploy|start|stop|status|collect}
set -u

REPO="git@github.com:illinois-cs-coursework/fa26_cs425_.group-mp_.team-42"
DIR=Distributed-Systems
HOSTS=($(seq 4201 4210))
DOMAIN="fa26-cs425"
SSH="ssh -n -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR"
SCP="scp -o BatchMode=yes -o ConnectTimeout=5"
USER="USER"


deploy() {
  local EXCLUDE_OPTS=(
    --exclude='.git'
    --exclude='*.o'
    --exclude='bins/'
    --exclude='logs/'
  )

  local RSYNC_SSH="ssh -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR"

  for h in "${HOSTS[@]}"; do
    (
      target="${USER}@${DOMAIN}-${h}.cs.illinois.edu"
      $SSH "$target" "mkdir -p ~/$DIR" >/dev/null 2>&1 && \
      rsync -az --delete "${EXCLUDE_OPTS[@]}" \
        -e "$RSYNC_SSH" \
        ./ "$target:~/$DIR/" && \
      $SSH "$target" "cd ~/$DIR && make clean && make" \
      && echo "[ok]   $h" || echo "[FAIL] $h"
    ) &
  done

  ./dispatch_log
  wait
}



# used for unit test, delete if need
push_logs() {
  id=1
  for h in "${HOSTS[@]}"; do
    local_file="./logs/machine.${id}.log"
    if [ ! -f "$local_file" ]; then
      echo "[ERROR] $local_file not found locally!"
      id=$((id+1))
      continue
    fi

    (
      target="fa26-cs425-${h}.cs.illinois.edu"
      ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o BatchMode=yes -o ConnectTimeout=5 \
        "$target" "mkdir -p ~/$DIR/logs && cat > ~/$DIR/logs/machine.${id}.log" < "$local_file" \
        && echo "[push] $h machine.${id}.log" || echo "[FAIL] $h push log"
    ) &
    id=$((id+1))
  done
  wait
}

start() {
  if [ -z "${1:-}" ]; then
  id=1
  for h in "${HOSTS[@]}"; do
    (
      $SSH fa26-cs425-${h}.cs.illinois.edu "{ cd ~/$DIR && nohup ./bins/server -i $id; } >/dev/null 2>&1 </dev/null &" \
        && echo "[start] $h id=$id" || echo "[FAIL] $h"
    ) &
    id=$((id+1))
  done
  else
  target=$1
  $SSH fa26-cs425-${HOSTS[$((target - 1))]}.cs.illinois.edu "{ cd ~/$DIR && nohup ./bins/server -i $target; } >/dev/null 2>&1 </dev/null &" \
        && echo "[start] ${HOSTS[$((target - 1))]} id=$target" || echo "[FAIL] $target"
  fi
  wait
}

stop() {
  if [ -z "${1:-}" ]; then
  for h in "${HOSTS[@]}"; do
    (
      $SSH fa26-cs425-${h}.cs.illinois.edu "pkill -9 -u \$USER -x server >/dev/null 2>&1 || true"
      echo "[stop] $h"
    ) &
  done
  else
    target=$1
    $SSH fa26-cs425-${HOSTS[$((target - 1))]}.cs.illinois.edu "pkill -9 -u \$USER -x server  >/dev/null 2>&1 || true" \
    && echo "[stop] ${HOSTS[$((target - 1))]} id=$target" || echo "[FAIL] $target"
  fi
  wait

  pkill -f \"./bins/server\"
}

case "${1:-}" in
  deploy)    deploy ;;
  start)     start "${2:-}" ;;
  stop)      stop "${2:-}";;
  push_logs) push_logs;;
  *) echo "usage: $0 {deploy|start|stop|push_logs}"; exit 1 ;;
esac