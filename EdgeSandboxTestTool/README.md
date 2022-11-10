#Edge Sandbox Testing Tool (ESTT)

## How to Compile?

```bash
mkdir build
cd build
cmake ..\src
cmake --build . --config Release
```

## How to Run?
`.\Release\estt.exe .\Release\renderer.exe`

## How To write your code in this project?
Make updates to the function `custom` at `child/renderer/renderer.cc`. The code will run with similar privileges and tokens as Edge (Chromium based) Renderer Process.

_Note: Using any other executable is not supported and it may not create the sandbox properly_