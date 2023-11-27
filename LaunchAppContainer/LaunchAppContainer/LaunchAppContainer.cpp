// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See License.md in the project root for license information.

#include <Windows.h>
#include <UserEnv.h>
#include <sddl.h>
#include <iostream>
#include <vector>
#include <string>
#pragma comment(lib, "onecoreuap.lib") // for DeriveCapabilitySidsFromName

/** RAII wrapper of SID_AND_ATTRIBUTES that adds a destructor to avoid memory leaks. */
struct SidAttrWrap : public SID_AND_ATTRIBUTES {
    SidAttrWrap(SID_AND_ATTRIBUTES obj) : SID_AND_ATTRIBUTES(obj) {
    }

    SidAttrWrap(SidAttrWrap&& obj) {
        Sid = 0;
        Attributes = 0;
        std::swap(obj.Sid, Sid);
        std::swap(obj.Attributes, Attributes);
    }

    ~SidAttrWrap() {
        if (Sid) {
            LocalFree(Sid);
            Sid = nullptr;
        }
    }
};

// Parsed command line arguments:
WCHAR* ExeToLaunch = nullptr;
std::wstring PackageMoniker;
std::wstring PackageDisplayName;
std::vector<SidAttrWrap> CapabilityList;
bool WaitForExit = false;
bool RetainProfile = false;
bool LaunchAsLpac = false;

void PrintUsage()
{
    wprintf(L"LaunchAppContainer.exe Usage:\r\n");
    wprintf(L"\t-m Moniker -i ExeToLaunch -c Capability1;Capability2;CapabilityN -w -r -l\r\n");
    wprintf(L"\r\n");
    wprintf(L"Required Arguments:\r\n");
    wprintf(L"\t-i : Path of the exe to launch\r\n");
    wprintf(L"\t-m : Package moniker\r\n");
    wprintf(L"\r\n");
    wprintf(L"Optional Arguments:\r\n");
    wprintf(L"\t-c : Capabilities (or SIDs) to set on the AppContainer token\r\n");
    wprintf(L"\t-d : Display name for the app container profile\r\n");

    wprintf(L"\t-w : Wait for AppContainer process to exit\r\n");
    wprintf(L"\t-r : Retain AppContainer profile after process exit\r\n");
    wprintf(L"\t-l : Create process as less-privileged AppContainer (LPAC)\r\n");
}


void FreeSidArray(PSID* sids, ULONG count)
{
    for (ULONG i = 0; i < count; i++) {
        LocalFree(sids[i]);
        sids[i] = nullptr;
    }

    LocalFree(sids);
    sids = nullptr;
}

bool ParseCapabilityList(WCHAR* psCapabilities)
{
    WCHAR* psNextToken = nullptr;

    // Tokenize the string and build the SID list.
    WCHAR* psCap = wcstok_s(psCapabilities, L";", &psNextToken);
    while (psCap != nullptr)
    {
        {
            SID_AND_ATTRIBUTES SidInfo = { 0 };
            SidInfo.Attributes = SE_GROUP_ENABLED;
            if (ConvertStringSidToSid(psCap, &SidInfo.Sid))
            {
                CapabilityList.push_back(SidInfo);

                psCap = nullptr; // signal that capability have been parsed
            }
        }

        if (psCap)
        {
            PSID * cap_group_sids = nullptr;
            DWORD cap_group_sids_len = 0;
            PSID * cap_sids = nullptr;
            DWORD cap_sids_len = 0;
            if (DeriveCapabilitySidsFromName(psCap, &cap_group_sids, &cap_group_sids_len, &cap_sids, &cap_sids_len))
            {
                // use capability SIDs
                for (size_t i = 0; i < cap_sids_len; ++i)
                    CapabilityList.push_back(SID_AND_ATTRIBUTES{cap_sids[i], SE_GROUP_ENABLED});

                // clean up cap_group_sids
                FreeSidArray(cap_group_sids, cap_group_sids_len);
                // clean up cap_sids object, but not entries who are needed when using CapabilityList later
                LocalFree(cap_sids);

                psCap = nullptr; // signal that capability have been parsed
            }
        }

        if (psCap)
        {
            wprintf(L"Error parsing capability list\r\n");
            return false;
        }

        psCap = wcstok_s(psNextToken, L";", &psNextToken);
    }

    return true;
}

bool ParseArguments(int argc, WCHAR** argv)
{
    // Loop and parse arguments.
    for (int i = 1; i < argc; i++)
    {
        // Check for argument switch characters.
        if (argv[i][0] != '-' && argv[i][0] != '/')
            continue;

        // Parse the argument accordingly.
        switch (argv[i][1])
        {
        case 'i':
        {
            if (i + 1 >= argc)
            {
                PrintUsage();
                return false;
            }

            ExeToLaunch = argv[++i];
            break;
        }
        case 'm':
        {
            if (i + 1 >= argc)
            {
                PrintUsage();
                return false;
            }

            PackageMoniker = argv[++i];
            break;
        }
        case 'c':
        {
            if (i + 1 >= argc)
            {
                PrintUsage();
                return false;
            }

            ParseCapabilityList(argv[++i]);
            break;
        }
        case 'd':
        {
            if (i + i >= argc)
            {
                PrintUsage();
                return false;
            }

            PackageDisplayName = argv[++i];
            break;
        }
        case 'w':
        {
            WaitForExit = true;
            break;
        }
        case 'r':
        {
            RetainProfile = true;
            break;
        }
        case 'l':
        {
            LaunchAsLpac = true;
            break;
        }
        }
    }

    return true;
}

DWORD CreateAppContainerProfileWithMoniker(PSID* pAppContainerSid)
{
    DWORD Result = ERROR_SUCCESS;
    PSID PackageSid = nullptr;

    // Create the app container profile.
    PCWSTR pDisplayString = PackageMoniker.size() == 0 ? PackageDisplayName.c_str() : PackageMoniker.c_str();
    HRESULT hr = CreateAppContainerProfile(PackageMoniker.c_str(), 
                                           pDisplayString, 
                                           pDisplayString, 
                                           CapabilityList.data(), 
                                           static_cast<DWORD>(CapabilityList.size()), 
                                           &PackageSid);
    if (FAILED(hr))
    {
        // Check if the app container profile already exists.
        if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            // Get the SID corresponding to the moniker.
            hr = DeriveAppContainerSidFromAppContainerName(PackageMoniker.c_str(), &PackageSid);
            if (FAILED(hr))
            {
                Result = HRESULT_CODE(hr);
                wprintf(L"Failed to obtain SID for existing app container profile %d\r\n", Result);
                goto Cleanup;
            }
        }
        else
        {
            Result = HRESULT_CODE(hr);
            wprintf(L"CreateAppContainerProfile failed with %d\r\n", Result);
            goto Cleanup;
        }
    }

    // App container profile created successfully or already exists.
    *pAppContainerSid = PackageSid;

Cleanup:

    if (Result != ERROR_SUCCESS && PackageSid != nullptr)
        FreeSid(PackageSid);

    return Result;
}

DWORD DeleteAppContainerProfileWithMoniker()
{
    HRESULT hr = DeleteAppContainerProfile(PackageMoniker.c_str());
    if (FAILED(hr))
    {
        wprintf(L"Failed to delete app container profile %d\r\n", HRESULT_CODE(hr));
        return HRESULT_CODE(hr);
    }

    return ERROR_SUCCESS;
}

DWORD LaunchProcess(PSID PackageSid)
{
    DWORD Result = ERROR_SUCCESS;
    DWORD AttributeCount = 1;
    LPPROC_THREAD_ATTRIBUTE_LIST AttributeList = nullptr;
    SIZE_T AttributeListSize = 0;
    SECURITY_CAPABILITIES SecurityCapabilities;
    DWORD AllApplicationPackagesPolicy = PROCESS_CREATION_ALL_APPLICATION_PACKAGES_OPT_OUT;
    STARTUPINFOEX StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };
    ULONGLONG ullMitigationPolicies[2] = { 0 };

    // If the process is running as LPAC account for the ALL_APPLICATION_PACKAGES_OPT_OUT and
    // PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY attributes.
    if (LaunchAsLpac == true)
        AttributeCount += 2;

    // Allocate and initialize the thread attribute list.
    if (InitializeProcThreadAttributeList(nullptr, AttributeCount, 0, &AttributeListSize) == FALSE)
    {
        Result = GetLastError();
        if (Result != ERROR_INSUFFICIENT_BUFFER)
        {
            wprintf(L"Failed to get thread attribute list size %d\r\n", Result);
            goto Cleanup;
        }
    }

    AttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, AttributeListSize);
    if (AttributeList == nullptr)
    {
        wprintf(L"Out of memory initializing thread attribute list\r\n");
        goto Cleanup;
    }

    if (InitializeProcThreadAttributeList(AttributeList, AttributeCount, 0, &AttributeListSize) == FALSE)
    {
        wprintf(L"Failed to initialize thread attribute list %d\r\n", Result);
        goto Cleanup;
    }

    // Setup the app container capabilities attribute.
    SecurityCapabilities.CapabilityCount = static_cast<DWORD>(CapabilityList.size());
    SecurityCapabilities.Capabilities = CapabilityList.data();
    SecurityCapabilities.AppContainerSid = PackageSid;
    SecurityCapabilities.Reserved = 0;

    if (UpdateProcThreadAttribute(AttributeList,
                                  0,
                                  PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
                                  &SecurityCapabilities,
                                  sizeof(SecurityCapabilities),
                                  nullptr,
                                  nullptr) == FALSE)
    {
        Result = GetLastError();
        wprintf(L"Failed to update thread attribute list %d\r\n", Result);
        goto Cleanup;
    }

    // If the process is running as LPAC add the ALL_APPLICATION_PACKAGES_OPT_OUT and
    // PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY policy attributes.
    if (LaunchAsLpac == true)
    {
        if (UpdateProcThreadAttribute(AttributeList,
                                      0,
                                      PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY,
                                      &AllApplicationPackagesPolicy,
                                      sizeof(AllApplicationPackagesPolicy),
                                      nullptr,
                                      nullptr) == FALSE)
        {
            Result = GetLastError();
            wprintf(L"Failed to update thread attribute list %d\r\n", Result);
            goto Cleanup;
        }

        // Setup mitigation policies for LPAC process.
        ullMitigationPolicies[0] |= PROCESS_CREATION_MITIGATION_POLICY_WIN32K_SYSTEM_CALL_DISABLE_ALWAYS_ON;

        if (UpdateProcThreadAttribute(AttributeList,
                                      0,
                                      PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY,
                                      &ullMitigationPolicies,
                                      sizeof(ullMitigationPolicies),
                                      nullptr,
                                      nullptr) == FALSE)
        {
            Result = GetLastError();
            wprintf(L"Failed to update thread attribute list %d\r\n", Result);
            goto Cleanup;
        }
    }

    // Setup the process information structure and create the AppContainer process.
    StartupInfo.StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpAttributeList = AttributeList;

    if (CreateProcessAsUser(NULL,
                            nullptr,
                            ExeToLaunch,
                            nullptr,
                            nullptr,
                            FALSE,
                            EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_CONSOLE,
                            nullptr,
                            nullptr,
                            (LPSTARTUPINFOW)&StartupInfo,
                            &ProcInfo) == FALSE)
    {
        Result = GetLastError();
        wprintf(L"Failed to create AppContainer process %d\r\n", Result);
        goto Cleanup;
    }

    // Print the process id for the AppContainer process.
    wprintf(L"Successfully started AppContainer process, pid: %d\r\n", ProcInfo.dwProcessId);
    Result = ERROR_SUCCESS;

    // Check if we should wait for the process to exit.
    if (WaitForExit == true)
    {
        wprintf(L"Waiting for AppContainer process to exit...\r\n");
        WaitForSingleObject(ProcInfo.hProcess, INFINITE);

        // Use the exit code of the AppContainer process for our exit code.
        if (GetExitCodeProcess(ProcInfo.hProcess, &Result) == FALSE)
            Result = GetLastError();
    }

    CloseHandle(ProcInfo.hProcess);
    CloseHandle(ProcInfo.hThread);

Cleanup:

    if (AttributeList != nullptr)
        HeapFree(GetProcessHeap(), 0, AttributeList);

    return Result;
}

int wmain(int argc, WCHAR** argv)
{
    DWORD Result = ERROR_SUCCESS;
    DWORD LowBoxConsoleEnabledValue = 1;
    PSID AppContainerSid = nullptr;

    // Parse command line arguments.
    if (ParseArguments(argc, argv) == false)
        return 0;

    // At a minimum we need the moniker and exe path.
    if (PackageMoniker.size() == 0 || ExeToLaunch == nullptr)
    {
        PrintUsage();
        wprintf(L"\r\n");
        wprintf(L"Package moniker and exe path are required to start an AppContainer process\r\n");
        return 0;
    }

    // Set the registry key to enable lowbox console processes.
    if (RegSetKeyValue(HKEY_CURRENT_USER, 
                       L"Console", 
                       L"LowBoxConsoleEnabled", 
                       REG_DWORD, 
                       &LowBoxConsoleEnabledValue, 
                       sizeof(LowBoxConsoleEnabledValue)) != ERROR_SUCCESS)
    {
        // If we fail to set the registry key log an error and continue anway.
        wprintf(L"Failed to set LowBoxConsoleEnabled registry value, proceeding to launch AppContainer process anyway...\r\n");
    }

    // Create the app container profile using the moniker provided.
    Result = CreateAppContainerProfileWithMoniker(&AppContainerSid);
    if (Result != ERROR_SUCCESS)
        goto Cleanup;

    // Launch the app container process.
    Result = LaunchProcess(AppContainerSid);

Cleanup:

    // Handle waiting for process termination and app container profile cleanup.
    if (WaitForExit == true && RetainProfile == true)
    {
        DeleteAppContainerProfileWithMoniker();
    }

    if (AppContainerSid != nullptr)
        FreeSid(AppContainerSid);

    return Result;
}