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

The 'LaunchSandboxMSRC.bat' file has been included to help security researchers test submissions for the MSRC bounty program. It contains a predefined list of capabilities that are eligible for the sandbox escape bounty award, capabilities not included in that list are not eligible for bounty submissions.
