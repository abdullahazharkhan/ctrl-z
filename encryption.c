
// RC4 Encryption/Decryption Implementation
// You can call it with this encrypt_with_rc4("FilePath", "Key");


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SBOX_SIZE 256

// Key Scheduling Algorithm (KSA)
void rc4_init(unsigned char *S, const unsigned char *key, int key_len) {
    for (int i = 0; i < SBOX_SIZE; i++)
        S[i] = i;

    int j = 0;
    for (int i = 0; i < SBOX_SIZE; i++) {
        j = (j + S[i] + key[i % key_len]) % SBOX_SIZE;
        unsigned char temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

// Pseudo-Random Generation Algorithm (PRGA)
void rc4_crypt(unsigned char *data, int data_len, const unsigned char *key, int key_len) {
    unsigned char S[SBOX_SIZE];
    rc4_init(S, key, key_len);

    int i = 0, j = 0;
    for (int k = 0; k < data_len; k++) {
        i = (i + 1) % SBOX_SIZE;
        j = (j + S[i]) % SBOX_SIZE;

        // Swap
        unsigned char temp = S[i];
        S[i] = S[j];
        S[j] = temp;

        int t = (S[i] + S[j]) % SBOX_SIZE;
        data[k] ^= S[t];  // XOR with keystream
    }
}

// 🧩 Your exposed encryption/decryption function
void encrypt_with_rc4(const char *filepath, const char *key) {
    FILE *f = fopen(filepath, "rb+");
    if (!f) {
        perror("File open failed");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    unsigned char *buffer = malloc(size);
    fread(buffer, 1, size, f);

    rc4_crypt(buffer, size, (const unsigned char *)key, strlen(key));

    rewind(f);
    fwrite(buffer, 1, size, f);
    fclose(f);
    free(buffer);

    printf("[+] RC4 processing done for file: %s\n", filepath);
}
