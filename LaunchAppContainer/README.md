# LaunchAppContainer
The LaunchAppContainer tool can be used to run applications in AppContainer or Less Privileged AppContainer (LPAC) sandboxes.

## How to run
```
LaunchAppContainer.exe Usage:
        -m Moniker -i ExeToLaunch -c Capability1;Capability2;CapabilityN -w -r -l

Required Arguments:
        -i : Path of the exe to launch
        -m : Package moniker

Optional Arguments:
        -c : Capabilities (or SIDs) to set on the AppContainer token
        -d : Display name for the app container profile
        -w : Wait for AppContainer process to exit
        -r : Retain AppContainer profile after process exit
        -l : Create process as less-privileged AppContainer (LPAC)
        -k : Create process with the disallow win32k process mitigation enabled
```

### Moniker
The application moniker is used to uniquely identify the AppContainer package. It typically consists of the application version, architecture, locale, and name. You can use any existing application moniker, create your own, or use the following test moniker:
```
1.0.0.0_x86_en-us_TestProgram_wvx3sa3v3dj1m
```

### Capabilities
AppContainer and LPAC sandboxes are granted access to various system resources through capability SIDs. You can use the '-c' parameter to specify the capability SIDs you want to grant the AppContainer or LPAC process. Multiple SIDs may be specified by concatenating them with a semicolon. For example: to run cmd.exe in the LPAC sandbox you will need to grant it the lpacCom and registryRead capabilities:
```
LaunchAppContainer.exe -m 1.0.0.0_x86_en-us_TestProgram_wvx3sa3v3dj1m -c S-1-15-3-1024-2405443489-874036122-4286035555-1823921565-1746547431-2453885448-3625952902-991631256;S-1-15-3-1024-1065365936-1281604716-3511738428-1654721687-432734479-3232135806-4053264122-3456934681 -w -l -i cmd.exe
```

### Process mitigations
The following arguments can be used to enable process mitigations for the sandbox process:
```
        -k : Create process with the disallow win32k process mitigation enabled
```

When the win32k lockdown mitigation policy is enabled the sandbox process will not be able to make any system calls to win32k, nor to create any UI elements. For console applications the standard output pipe has been redirected to the parent process console window.
Applications run with the win32k lockdown mitigation policy should be compiled without linking to any dlls that perform system calls to win32k, and should link the VC runtime statically instead of dynamically.

### MSRC bounty submission
The 'LaunchSandboxMSRC.bat' file has been included to help security researchers test submissions for the MSRC bounty program. 
It contains required command line options and a predefined list of capabilities that are eligible for the sandbox escape bounty award. Changes to the command line options or use of capabilities not included in the batch file are not eligible for bounty submissions. 
MSRC bounty submissions are required to run and reproduce with the disallow win32k command line argument present (set by default in the batch file). This may require you compile your submission application using special compiler options. Please see the "Process mitigations" section for more info.
