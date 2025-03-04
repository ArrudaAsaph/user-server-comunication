#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdio.h>

int main() {
    printf("OpenSSL versão: %s\n", SSLeay_version(SSLEAY_VERSION));
    return 0;
}