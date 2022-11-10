# Edge Sandbox Testing Tool (ESTT)
The Edge Sandbox Testing Tool can be used to test code running inside the Chromium renderer process sandbox. This tool exposes an empty C++ function that you can fill in with your own code to run inside of the Chromium renderer sandbox environment.

## How to Compile

```bash
mkdir build
cd build
cmake ..\src
cmake --build . --config Release
```

This project produces two executables: estt.exe and renderer.exe. The renderer.exe application is the sandbox process that mimics the restricted environment of the Chromium renderer process. The estt.exe application is used to start the renderer.exe sandbox process.

## How to Run
`.\Release\estt.exe .\Release\renderer.exe`

_Note: The sandbox process must go through a specific initialization routine in order to run with the proper process token. Using ESTT to run an application other than renderer.exe is not supported, and may not run with the same restrictions as the real Chromium renderer process sandbox._

## Running your own code
Make updates to the function `custom` in the `child/renderer/renderer.cc` file. This code will run after the sandbox process has completed its initialization and dropped to the secure sandbox token.
