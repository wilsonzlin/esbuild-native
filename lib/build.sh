#!/usr/bin/env bash

set -e

pushd "$(dirname "$0")/vendor/github.com/evanw/esbuild/"

go build -mod=vendor -buildmode=c-archive -o ../../../../build/libesbuild.a pkg/ffiapi/ffiapi.go

popd
