#!/usr/bin/env bash
# Scenario runner for rbk_shutdown_demo. Usage: demo.sh [path/to/rbk_shutdown_demo]
# Each scenario runs in its own temp directory; the directories of failed scenarios are kept.
set -uo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$(realpath "${1:-$DIR/../../build/demo/shutdown/rbk_shutdown_demo}")"
[[ -x "$BIN" ]] || { echo "not executable: $BIN" >&2; exit 2; }

PORT_BASE=8097
failed=0
pid="" wd="" port=0

# ── helpers ────────────────────────────────────────────────────────────────
now() { echo "$EPOCHREALTIME"; }
since() { awk -v a="$1" -v b="$(now)" 'BEGIN { printf "%.2f", b - a }'; }
between() { awk -v x="$1" -v lo="$2" -v hi="$3" 'BEGIN { exit !(x >= lo && x <= hi) }'; }

# gone PID: true when the process no longer exists or is a zombie.
gone() {
  [[ ! -e "/proc/$1" ]] && return 0
  grep -q '^State:[[:space:]]*Z' "/proc/$1/status" 2>/dev/null
}

port_open() { (exec 3<>"/dev/tcp/127.0.0.1/$1") 2>/dev/null; }

# start N ARGS...: launch the demo in a new temp dir, on port PORT_BASE+N.
start() {
  local n="$1"; shift
  port=$((PORT_BASE + n))
  wd="$(mktemp -d "${TMPDIR:-/tmp}/rbk-shutdown-demo.S$n.XXXXXX")"
  (cd "$wd" && exec "$BIN" --port "$port" "$@" >out.txt 2>&1) &
  pid=$!
}

# wait_ready: wait for "main: ready" and the port, at most 10 s.
wait_ready() {
  local i
  for i in $(seq 200); do
    if grep -q "main: ready" "$wd/out.txt" 2>/dev/null && port_open "$port"; then
      return 0
    fi
    gone "$pid" && return 1
    sleep 0.05
  done
  return 1
}

# wait_exit: wait at most 10 s for the demo to end, then set rc and t (seconds since $1).
# On timeout: kill -9, rc=timeout.
wait_exit() {
  local t0="$1" i
  for i in $(seq 500); do
    if gone "$pid"; then
      t="$(since "$t0")"
      wait "$pid"
      rc=$?
      return 0
    fi
    sleep 0.02
  done
  t="$(since "$t0")"
  kill -9 "$pid" 2>/dev/null
  wait "$pid"
  rc=timeout
  return 1
}

has() { grep -qF -- "$1" "$wd/out.txt"; }

# report NAME EXPECT GOT OK
report() {
  local name="$1" expect="$2" got="$3" ok="$4"
  if [[ "$ok" == 1 ]]; then
    printf '%-4s %-44s %-34s %6ss  ok\n' "$name" "$expect" "$got" "$t"
    rm -rf -- "$wd"
  else
    printf '%-4s %-44s %-34s %6ss  FAIL (%s)\n' "$name" "$expect" "$got" "$t" "$wd"
    failed=1
  fi
}

# ── scenarios ──────────────────────────────────────────────────────────────
s_signal() { # S1 / S2: signal when ready
  local n="$1" sig="$2" name="$3" ok=1 t0
  start "$n"
  wait_ready || ok=0
  t0="$(now)"
  kill -"$sig" "$pid"
  wait_exit "$t0"
  [[ "$rc" == 0 ]] && between "$t" 0 1 || ok=0
  has "shutdown: SIG$sig received" || ok=0
  has "main: return 0" || ok=0
  if [[ "$n" == 1 ]]; then
    grep -qF "main: return 0" "$wd"/log/*.log 2>/dev/null || ok=0
  fi
  report "$name" "SIG$sig: rc 0 <1s, return 0" "rc $rc" "$ok"
}

s3() {
  local ok=1 t0
  start 3
  wait_ready || ok=0
  t0="$(now)"
  # The reply may be lost: stopping the io_context drops in-flight requests (design R5).
  curl -s -m 2 "http://127.0.0.1:$port/exit" >/dev/null || true
  wait_exit "$t0"
  [[ "$rc" == 0 ]] && between "$t" 0 1 || ok=0
  has "shutdown: request received" || ok=0
  report S3 "curl /exit: rc 0 <1s, request received" "rc $rc" "$ok"
}

s4() {
  local ok=1 t0
  start 4
  wait_ready || ok=0
  curl -s -m 2 "http://127.0.0.1:$port/job?ms=1500" >/dev/null || ok=0
  sleep 0.2
  t0="$(now)"
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 0 ]] && between "$t" 1.0 2.0 || ok=0
  has "worker: job 1 done" || ok=0
  [[ "$(cat "$wd/demo.state" 2>/dev/null)" == "jobsDone=1" ]] || ok=0
  report S4 "job 1.5s in hand: rc 0 in 1-2s, jobsDone=1" "rc $rc, $(cat "$wd/demo.state" 2>/dev/null)" "$ok"
}

s5() {
  local ok=1 t0
  start 5 --stuck --deadline 2
  wait_ready || ok=0
  t0="$(now)"
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 143 ]] && between "$t" 2.0 2.6 || ok=0
  has "forced after 2s" || ok=0
  grep "forced after" "$wd/out.txt" | grep -q stuck || ok=0
  has "main: return 0" && ok=0
  report S5 "stuck: rc 143 in 2-2.6s, names stuck" "rc $rc" "$ok"
}

s6() {
  local ok=1 t0
  start 6 --stuck --deadline 30
  wait_ready || ok=0
  kill -TERM "$pid"
  sleep 0.5
  t0="$(now)"
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 143 ]] && between "$t" 0 0.5 || ok=0
  report S6 "2nd SIGTERM: rc 143 <0.5s" "rc $rc" "$ok"
}

s7() {
  local ok=1 t0
  t0="$(now)"
  start 7 --slow-start 2
  sleep 0.5
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 0 ]] && between "$t" 0 3 || ok=0
  has "http: listen returned" || ok=0
  report S7 "SIGTERM in startup: rc 0 <3s from launch" "rc $rc" "$ok"
}

s8() {
  local ok=1 t0
  start 8 --no-install
  wait_ready || ok=0
  t0="$(now)"
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 143 ]] || ok=0
  has "main: exec returned" && ok=0
  report S8 "--no-install: rc 143 (old path)" "rc $rc" "$ok"
}

s9() { # the child keeps the normal SIGTERM behaviour
  local ok=1 t0 cpid mask
  start 9 --child
  wait_ready || ok=0
  cpid="$(sed -n 's/.*child: QProcess pid=\([0-9]*\).*/\1/p' "$wd/out.txt")"
  mask="$(sed -n 's/.*child: QProcess .*sigblk=\([0-9a-f]*\).*/\1/p' "$wd/out.txt")"
  if [[ -z "$cpid" || -z "$mask" ]]; then
    ok=0
  else
    (( (16#$mask & 0x4002) == 0 )) || ok=0
    kill -TERM "$cpid" 2>/dev/null
    sleep 0.5
    gone "$cpid" || ok=0
  fi
  t0="$(now)"
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 0 ]] || ok=0
  report S9 "child mask 0, child stops on SIGTERM, rc 0" "rc $rc, mask $(printf '%x' "0x${mask:-0}")" "$ok"
}

s12() { # like systemd stop: SIGTERM to the demo and its child at the same time
  local ok=1 t0 cpid
  start 12 --child
  wait_ready || ok=0
  cpid="$(sed -n 's/.*child: QProcess pid=\([0-9]*\).*/\1/p' "$wd/out.txt")"
  [[ -n "$cpid" ]] || ok=0
  t0="$(now)"
  kill -TERM "$pid" ${cpid:+"$cpid"}
  wait_exit "$t0"
  [[ "$rc" == 0 ]] && between "$t" 0 1 || ok=0
  [[ -n "$cpid" ]] && ! gone "$cpid" && ok=0
  report S12 "SIGTERM demo + child: rc 0 <1s, child gone" "rc $rc" "$ok"
}

s10() {
  local ok=1 t0
  start 10 --stuck-callback --deadline 1
  wait_ready || ok=0
  t0="$(now)"
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 142 ]] && between "$t" 5.5 7 || ok=0
  report S10 "stuck callback: rc 142 in 5.5-7s" "rc $rc" "$ok"
}

s11() {
  local ok=1 t0 i done_
  start 11
  wait_ready || ok=0
  for i in $(seq 20); do
    curl -s -m 2 "http://127.0.0.1:$port/job?ms=10" >/dev/null || ok=0
  done
  t0="$(now)"
  kill -TERM "$pid"
  wait_exit "$t0"
  [[ "$rc" == 0 ]] || ok=0
  done_="$(sed -n 's/^jobsDone=//p' "$wd/demo.state" 2>/dev/null)"
  [[ -n "$done_" ]] && ((done_ <= 20)) || ok=0
  report S11 "20 jobs + SIGTERM: rc 0, jobsDone<=20" "rc $rc, jobsDone=${done_:-?}" "$ok"
}

# ── run ────────────────────────────────────────────────────────────────────
printf '%-4s %-44s %-34s %7s  %s\n' "#" "expected" "got" "time" "result"
# 2>/dev/null: bash reaps the demo by itself and prints "Alarm clock" / "Terminated" job notices
# on stderr. Results go to stdout.
{
  s_signal 1 TERM S1
  s_signal 2 INT S2
  s3
  s4
  s5
  s6
  s7
  s8
  s9
  s10
  s11
  s12
} 2>/dev/null
exit "$failed"
