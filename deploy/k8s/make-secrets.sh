#!/bin/sh
# Creates the Secrets for agrisoil-dev with random passwords.
# Never overwrites an existing Secret, so running it again is safe.
# Read a value later with e.g.:
#   kubectl -n agrisoil-dev get secret nodered-auth -o jsonpath='{.data.admin-password}' | base64 -d
set -eu
NS=${NS:-agrisoil-dev}
rand() { head -c 32 /dev/urandom | base64 | tr -dc 'A-Za-z0-9' | head -c 24; }

make() {
  name=$1; shift
  if kubectl -n "$NS" get secret "$name" >/dev/null 2>&1; then
    echo "secret/$name exists, kept"
    return
  fi
  args=""
  for kv in "$@"; do args="$args --from-literal=$kv"; done
  # shellcheck disable=SC2086
  kubectl -n "$NS" create secret generic "$name" $args >/dev/null
  kubectl -n "$NS" label secret "$name" app.kubernetes.io/part-of=agrisoil >/dev/null
  echo "secret/$name created"
}

kubectl get ns "$NS" >/dev/null 2>&1 || kubectl create ns "$NS"
make mosquitto-auth "nodered-password=$(rand)" "device-password=$(rand)"
make postgres-auth "agrisoil-password=$(rand)" "dashboard-password=$(rand)"
make nodered-auth "admin-user=admin" "admin-password=$(rand)" "http-user=agrisoil" "http-password=$(rand)" \
  "credential-secret=$(rand)"
make dashboard-auth "admin-user=admin" "admin-password=$(rand)"
