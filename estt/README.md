#Edge Sandbox Testing Tool (ESTT)

## How to Compile?

```bash
mkdir build
cd build
cmake ..\src
cmake --build . --config Release
```

## How to Run?
`.\build\estt.exe .\build\custom.exe`

## How To write your code in this project?
Make updates to the function `custom` at `child/renderer/renderer.cc`. The code will run with similar privileges and tokens as Edge (Chromium based) Renderer Process.
