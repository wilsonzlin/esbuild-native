(Get-ChildItem -Path lib/vendor/github.com/evanw/esbuild -Filter '*.go' -Recurse -File | Resolve-Path -Relative) -replace '[\\/]', '/'
