#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string.h>
#include <initguid.h>
#include <knownfolders.h>
#include "decryptor.h"
#include "walker.h"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")

// 복호화 대상 파일인지 확인
int is_target_file(const char* filename) {
    return (StrStrIA(filename, ".adr") != NULL);
}

// 시스템 경로 여부 확인
int is_system_path(const char* path) {
    return (
        StrStrIA(path, "\\Windows\\") ||
        StrStrIA(path, "\\Program Files") ||
        StrStrIA(path, "\\AppData\\")
        );
}

// 지정된 디렉토리 내 재귀적 복호화 수행
int scan_and_decrypt(const char* dir, const unsigned char* key, const unsigned char* iv) {
    int success = 0;
    if (is_system_path(dir)) return 0;

    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char fullpath[MAX_PATH];
        snprintf(fullpath, MAX_PATH, "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            success += scan_and_decrypt(fullpath, key, iv);
        }
        else {
            if (is_target_file(fd.cFileName)) {
                if (decrypt_file(fullpath, key, iv)) {
                    success++;
                }
            }
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    return success;
}

// 주요 사용자 폴더 및 현재 폴더 복호화 수행
int decrypt_all_files() {
    unsigned char key[32], iv[16];
    restore_key(key);
    restore_iv(iv);

    int total_success = 0;

    const int folders[] = {
        CSIDL_DESKTOP, CSIDL_PERSONAL, CSIDL_MYPICTURES,
        CSIDL_MYMUSIC, CSIDL_MYVIDEO, CSIDL_PROFILE
    };

    char path[MAX_PATH];
    for (int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
        if (SHGetFolderPathA(NULL, folders[i], NULL, 0, path) == S_OK) {
            total_success += scan_and_decrypt(path, key, iv);
        }
    }

    // Downloads 폴더 복호화
    PWSTR wpath = NULL;
    if (SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &wpath) == S_OK) {
        wcstombs(path, wpath, MAX_PATH);
        total_success += scan_and_decrypt(path, key, iv);
        CoTaskMemFree(wpath);
    }

    // 현재 폴더 복호화
    GetCurrentDirectoryA(MAX_PATH, path);
    total_success += scan_and_decrypt(path, key, iv);

    return total_success;
}
