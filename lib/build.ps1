Set-Location ../lib/vendor/github.com/evanw/esbuild
go build -mod=vendor -buildmode=c-shared -o ../../../../build/esbuild.dll pkg/ffiapi/ffiapi.go
Set-Location ../../../..

$bytes = [System.IO.File]::ReadAllBytes("${pwd}/build/esbuild.dll")

$out = 'char const DLL_BYTES[' + $bytes.length + '] = {' + ($bytes -join ', ') + '};'

Set-Content -Path build/esbuild.dll.h -Value $out

# Remove MSVC unsupported Go types from header.
Move-Item -Path build/esbuild.h -Destination build/esbuild.h.orig
Get-Content build/esbuild.h.orig | Where-Object {$_ -notmatch 'GoUintptr|GoComplex64|GoComplex128'} | Set-Content build/esbuild.h
