go build -mod=vendor -buildmode=c-shared -o build/esbuild.dll ./vendor/github.com/evanw/esbuild/pkg/ffiapi/ffiapi.go

$bytes = [System.IO.File]::ReadAllBytes("${pwd}/build/esbuild.dll")

$out = 'char const* DLL_BYTES = {' + ($bytes -join ', ') + '}'

Set-Content -Path build/esbuild.dll.h -Value $out
