#!/usr/bin/env bash
set -euo pipefail

DIR=$(cd "$(dirname "$0")" && pwd)
OUT="$DIR/certs"
mkdir -p "$OUT"

CN_ROOT="VSOMEIP Test Root CA"
CN_INTER="VSOMEIP Test Intermediate CA"
CN_SERVER="localhost"
CN_CLIENT="vsomeip-client"

days=3650

openssl genrsa -out "$OUT/root.key" 4096
openssl req -x509 -new -nodes -key "$OUT/root.key" -sha256 -days $days -subj "/CN=$CN_ROOT" -out "$OUT/root.crt"

openssl genrsa -out "$OUT/intermediate.key" 4096
openssl req -new -key "$OUT/intermediate.key" -subj "/CN=$CN_INTER" -out "$OUT/intermediate.csr"
openssl x509 -req -in "$OUT/intermediate.csr" -CA "$OUT/root.crt" -CAkey "$OUT/root.key" -CAcreateserial -out "$OUT/intermediate.crt" -days $days -sha256 -extfile <(printf "basicConstraints=CA:TRUE\nkeyUsage=keyCertSign,cRLSign\n")

cat "$OUT/intermediate.crt" > "$OUT/ca_chain.crt"

openssl genrsa -out "$OUT/server.key" 2048
openssl req -new -key "$OUT/server.key" -subj "/CN=$CN_SERVER" -out "$OUT/server.csr"
openssl x509 -req -in "$OUT/server.csr" -CA "$OUT/intermediate.crt" -CAkey "$OUT/intermediate.key" -CAcreateserial -out "$OUT/server.crt" -days $days -sha256 -extfile <(printf "subjectAltName=DNS:localhost,IP:127.0.0.1\n")
cat "$OUT/server.crt" > "$OUT/server_chain.crt"

openssl genrsa -out "$OUT/client.key" 2048
openssl req -new -key "$OUT/client.key" -subj "/CN=$CN_CLIENT" -out "$OUT/client.csr"
openssl x509 -req -in "$OUT/client.csr" -CA "$OUT/intermediate.crt" -CAkey "$OUT/intermediate.key" -CAcreateserial -out "$OUT/client.crt" -days $days -sha256
cat "$OUT/client.crt" > "$OUT/client_chain.crt"

echo "Generated certificates under $OUT"

