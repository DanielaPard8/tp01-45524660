#!/usr/bin/env bash

set -uo pipefail

PORT=$((20000 + ($$ % 20000)))
SERVER_PID=""
SERVER_STDOUT="$(mktemp)"
SERVER_STDERR="$(mktemp)"
PASSED=0

cleanup() {
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -f "$SERVER_STDOUT" "$SERVER_STDERR"
}

trap cleanup EXIT

fail() {
    echo "FALLO: $*" >&2
    if [[ -s "$SERVER_STDERR" ]]; then
        echo "--- stderr del servidor ---" >&2
        tail -n 30 "$SERVER_STDERR" >&2
    fi
    exit 1
}

pass() {
    PASSED=$((PASSED + 1))
    echo "OK $PASSED - $*"
}

open_connection() {
    local fd="$1"
    eval "exec ${fd}<>/dev/tcp/127.0.0.1/${PORT}"
}

close_connection() {
    local fd="$1"
    eval "exec ${fd}>&-"
    eval "exec ${fd}<&-"
}

read_line() {
    local fd="$1"
    local variable="$2"
    local value

    if ! IFS= read -r -t 2 -u "$fd" value; then
        fail "no se recibió una línea dentro del tiempo esperado"
    fi
    printf -v "$variable" '%s' "$value"
}

expect_line() {
    local fd="$1"
    local expected="$2"
    local actual

    read_line "$fd" actual
    [[ "$actual" == "$expected" ]] ||
        fail "se esperaba '$expected' y se recibió '$actual'"
}

./servidor "$PORT" >"$SERVER_STDOUT" 2>"$SERVER_STDERR" &
SERVER_PID=$!

READY=0
for _ in {1..30}; do
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        fail "el servidor finalizó durante el arranque"
    fi
    if { exec 9<>"/dev/tcp/127.0.0.1/$PORT"; } 2>/dev/null; then
        exec 9>&-
        exec 9<&-
        READY=1
        break
    fi
    sleep 0.1
done

[[ "$READY" -eq 1 ]] || fail "el servidor no comenzó a escuchar en el puerto $PORT"
pass "inicio del servidor"

open_connection 3 || fail "no se pudo abrir la primera conexión"

printf 'PING\n' >&3
expect_line 3 "OK PONG"
pass "PING"

printf 'ECHO Sistemas distribuidos\n' >&3
expect_line 3 "OK Sistemas distribuidos"
pass "ECHO"

printf 'TIME\n' >&3
read_line 3 time_response
[[ "$time_response" =~ ^OK[[:space:]]TIME[[:space:]][0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]][0-9]{2}:[0-9]{2}:[0-9]{2}$ ]] ||
    fail "respuesta TIME inválida: '$time_response'"
pass "TIME"

printf 'COUNT\n' >&3
expect_line 3 "OK COUNT 4"
pass "COUNT"

printf 'COMANDO_INEXISTENTE\n' >&3
expect_line 3 "ERROR UNKNOWN_COMMAND"
pass "comando desconocido"

printf 'COUNT\n' >&3
expect_line 3 "OK COUNT 5"
pass "el comando desconocido no incrementa COUNT"

printf '\n' >&3
expect_line 3 "ERROR EMPTY_COMMAND"
pass "línea vacía"

printf 'ECHO\n' >&3
expect_line 3 "ERROR INVALID_ARGUMENT"
pass "ECHO sin argumento"

printf 'QUIT\n' >&3
expect_line 3 "OK BYE"
close_connection 3
pass "QUIT"

open_connection 4 || fail "no se pudo abrir la conexión de fragmentación"
printf 'PI' >&4
sleep 0.1
printf 'NG\n' >&4
expect_line 4 "OK PONG"
pass "comando fragmentado"

printf 'ECHO dos mensajes\nCOUNT\n' >&4
expect_line 4 "OK dos mensajes"
expect_line 4 "OK COUNT 3"
pass "varios comandos recibidos juntos"

LONG_LINE="$(printf '%*s' 1100 '' | tr ' ' A)"
printf '%s\n' "$LONG_LINE" >&4
expect_line 4 "ERROR LINE_TOO_LONG"
printf 'PING\n' >&4
expect_line 4 "OK PONG"
pass "línea demasiado larga y resincronización"

close_connection 4
sleep 0.1
kill -0 "$SERVER_PID" 2>/dev/null ||
    fail "el servidor terminó ante una desconexión sin QUIT"
pass "desconexión sin QUIT"

CLIENT_OUTPUT="$(printf 'PING\nQUIT\n' | timeout 5 ./cliente 127.0.0.1 "$PORT")" ||
    fail "el cliente terminó con error"

EXPECTED_OUTPUT=$'OK PONG\nOK BYE'
[[ "$CLIENT_OUTPUT" == "$EXPECTED_OUTPUT" ]] ||
    fail "salida inesperada del cliente: '$CLIENT_OUTPUT'"
pass "cliente completo"

echo "Todas las pruebas visibles fueron superadas: $PASSED"

