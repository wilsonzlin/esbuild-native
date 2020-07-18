{
  "targets": [
    {
      "target_name": "binding",
      "sources": [
        "native/binding.c"
      ],
      "include_dirs": [
        "lib/MemoryModule/",
        "lib/build/",
      ],
      "conditions": [
        ["OS=='mac'", {
          "libraries": [
            "CoreFoundation.framework",
            "Security.framework",
          ],
        }],
        ["OS!='win'", {
          "actions": [
            {
              "action_name": "build esbuild-lib static library",
              "inputs": ["<!@(find lib/vendor/github.com/evanw/esbuild -type f -name '*.go')"],
              "outputs": ["lib/build/libesbuild.h", "lib/build/libesbuild.a"],
              "action": ['bash', 'lib/build.sh'],
            },
          ],
          "libraries": [
            "lib/build/libesbuild.a",
          ],
        }],
        ["OS=='win'", {
          "actions": [
            {
              "action_name": "build esbuild-lib DLL",
              "inputs": ["<!@(cd lib/vendor/github.com/evanw/esbuild & dir *.go /B /S)"],
              "outputs": ["lib/build/esbuild.h", "lib/build/esbuild.dll", "lib/build/esbuild.dll.h"],
              "action": ['lib/build.ps1'],
            },
          ],
        }],
      ],
    },
  ],
}
