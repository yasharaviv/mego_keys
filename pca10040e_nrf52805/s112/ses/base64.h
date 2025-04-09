#ifndef _BASE64_H_
#define _BASE64_H_

void base64_encode(const uint8_t *input, size_t input_len, char *output);
int base64_decode(const char *input, uint8_t *output, size_t *output_len);
void test_base64();

#endif //_BASE64_H_