#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    if (sodium_init() < 0) {
        fprintf(stderr, "Libsodium initialization failed\n");
        return 1;
    }

    // Generate recipient's key pair
    unsigned char recipient_public[crypto_box_PUBLICKEYBYTES];
    unsigned char recipient_secret[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(recipient_public, recipient_secret);

    // Sender encrypts a message using the recipient's public key
    const char *message = "Hello, this is a secret message!";
    size_t message_len = strlen(message) + 1; // Include NULL terminator

    // Encrypt (sender side)
    unsigned char ciphertext[crypto_box_SEALBYTES + message_len];
    crypto_box_seal(ciphertext, (const unsigned char *)message, message_len, recipient_public);

    // Simulate sending ciphertext over UDP
    // (In real code, send the raw bytes of `ciphertext`)

    // Decrypt (recipient side)
    unsigned char decrypted[message_len];
    if (crypto_box_seal_open(decrypted, ciphertext, sizeof(ciphertext),
                             recipient_public, recipient_secret) != 0) {
        fprintf(stderr, "Decryption failed\n");
        return 1;
    }

    printf("Decrypted: %s\n", decrypted);
    return 0;
}
