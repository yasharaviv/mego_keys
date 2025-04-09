#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Function to encode data to Base64
void base64_encode(const uint8_t *input, size_t input_len, char *output) {
    int i, j;
    for (i = 0, j = 0; i < input_len; i += 3) {
        uint32_t octet_a = i < input_len ? input[i] : 0;
        uint32_t octet_b = i + 1 < input_len ? input[i + 1] : 0;
        uint32_t octet_c = i + 2 < input_len ? input[i + 2] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output[j++] = base64_table[(triple >> 18) & 0x3F];
        output[j++] = base64_table[(triple >> 12) & 0x3F];
        output[j++] = (i + 1 < input_len) ? base64_table[(triple >> 6) & 0x3F] : '=';
        output[j++] = (i + 2 < input_len) ? base64_table[triple & 0x3F] : '=';
    }
    output[j] = '\0';  // Null-terminate the output
}

// Function to decode Base64-encoded data
int base64_decode(const char *input, uint8_t *output, size_t *output_len) {
    int input_len = strlen(input);
    if (input_len % 4 != 0) return -1;  // Invalid input length

    int i, j;
    uint32_t buffer = 0;
    int buffer_len = 0;
    *output_len = 0;

    for (i = 0, j = 0; i < input_len; i++) {
        char c = input[i];

        // Ignore '=' padding
        if (c == '=') break;

        // Find character index in base64 table
        char *pos = strchr(base64_table, c);
        if (!pos) return -1;  // Invalid character

        buffer = (buffer << 6) | (pos - base64_table);
        buffer_len += 6;

        if (buffer_len >= 8) {
            buffer_len -= 8;
            output[j++] = (buffer >> buffer_len) & 0xFF;
        }
    }
    *output_len = j;
    return 0;  // Success
}

// Test the Base64 encoding and decoding
void test_base64() {
    const char *test_str = "Hello, nRF!";
    char encoded[64];
    uint8_t decoded[64];
    size_t decoded_len;

    // Encode
    base64_encode((const uint8_t *)test_str, strlen(test_str), encoded);
    printf("Encoded: %s\n", encoded);

    // Decode
    if (base64_decode(encoded, decoded, &decoded_len) == 0) {
        decoded[decoded_len] = '\0';  // Null-terminate decoded string
        printf("Decoded: %s\n", decoded);
    } else {
        printf("Base64 decoding failed!\n");
    }
}