#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#define ENCODE 0
#define DECODE 1

char encode_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* 
 * convert decimal integer to binary string
 * @decimal: decimal integer
 */
char *decimal_to_binary(int decimal) 
{
    char *binary = malloc(8);
    if (binary == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    int count;
    for (count = 7; count >= 0; count--) {
        *(binary + (7 - count)) = ((decimal >> count) & 1) + '0';
    }
    return binary;
}

/* 
 * convert binary string to decimal integer
 * @binary: a null-terminated byte string
 */
int binary_to_decimal(const char *binary)
{
    int length = strlen(binary);
    int idx, decimal = 0;
    for (idx = length - 1; idx >= 0; idx--) {
        decimal += (binary[idx] - '0') * (int)pow(2, (length - 1) - idx);
    }
    return decimal;
}

/* 
 * check valid base64 string
 * @base64: a null-terminated base64 string to be decoded
 */
int is_valid_base64(const char *base64)
{
    int len = strlen(base64);
    if (len % 4 > 0) {
        return 0;
    } else {
        int i;
        char c;
        for (i = 0; (c = base64[i]) != '\0'; i++) {
            if (strchr(encode_chars, c) == NULL) {
                if (c == '=') {
                    if (i < len - 2) {
                        return 0;
                    }
                } else {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/* 
 * Base64 encode
 * @plainbytes: bytes stand for plain data
 * @len: length of plain data
 */
char *encode(char *plainbytes, size_t len)
{
    int i, j, k;
    int idx, pos = 0;
    char transform_bits[24];
    char base64_bits[7] = {};
    char *binary;
    char *base64 = NULL;
    int base64bytes_count = 0;
    while (pos < len) {
        for (i = 0; i < 24; i++) {
            transform_bits[i] = '0';
        }
        for (j = pos, k = 0; (j < pos + 3) && (j < len); j++, k++) {
            binary = decimal_to_binary((int)plainbytes[j]);
            strncpy(transform_bits + k * 8, binary, 8);
            free(binary);
        }
        for (k = 0; k < (4 - (pos + 3 - j)); k++) {
            strncpy(base64_bits, transform_bits + k * 6, 6);
            idx = binary_to_decimal(base64_bits);
            base64bytes_count += 1;
            base64 = realloc(base64, base64bytes_count);
            base64[base64bytes_count - 1] = encode_chars[idx]; 
        }
        if (j < len) {
            pos += 3;
        } else {
            break;
        }
    }
    int extra_bytes = 3 - len % 3;
    while (extra_bytes > 0) {
        base64bytes_count += 1;
        base64 = realloc(base64, base64bytes_count);
        base64[base64bytes_count - 1] = extra_bytes == 0 ? '\0': '=';
        extra_bytes -= 1;
    }
   return base64;
}


/* 
 * Base64 decode
 * @base64: a null-terminated base64 byte string
 */
char *decode(char *base64)
{
    if (!is_valid_base64(base64)) {
        fprintf(stderr, "Invalid base64 string.\n");
        exit(EXIT_FAILURE);
    }
    strtok(base64, "=");
    char *binary, *chrpos;
    char *plainbytes = NULL;
    char transform_bits[24];
    char byte_bits[9] = {};
    int j, k;
    int idx, pos = 0;
    int decimal, plainbytes_count = 0;
    while (base64[pos] != '\0') {
        for (j = pos, k =0 ; (j < pos + 4) && base64[j] != '\0'; j++, k++) {
            chrpos = strchr(encode_chars, base64[j]);
            idx = (int)(chrpos - encode_chars);
            binary = decimal_to_binary(idx);
            strncpy(transform_bits + k * 6, binary + 2, 6);
            free(binary);
        }
        for (k = 0; k < (3 - (pos + 4 - j)); k++) {
            strncpy(byte_bits, transform_bits + k * 8, 8);
            decimal = binary_to_decimal(byte_bits);
            plainbytes_count += 1;
            plainbytes = realloc(plainbytes, plainbytes_count + 1);
            plainbytes[plainbytes_count - 1] = decimal;
        }
        pos += 4;
    }
    plainbytes_count += 1;
    plainbytes = realloc(plainbytes, plainbytes_count + 1);
    plainbytes[plainbytes_count - 1] = '\0';
    return plainbytes;
}

/* 
 * process text from stdio
 * @operation: ENCODE->encode text, DECODE->decode text
 */
void base64_stdio(int operation) 
{
    size_t nbytes;
    int bytes_read;
    char *output = NULL, *input = NULL;
    printf("Type here: ");
    fflush(stdout);
    bytes_read = getline(&input, &nbytes, stdin);
    if (bytes_read < 0) {
        free(input);
        perror("getline");
        exit(EXIT_FAILURE);
    } else if (bytes_read > 1) {
        if (operation == ENCODE) {
            output = encode(input, bytes_read - 1);
        } else if (operation == DECODE) {
            input[bytes_read - 1] = '\0';
            output = decode(input);
        }
        printf("%s", output);
        free(input);
        free(output);
    }
}

/* 
 * encode binary file
 * @file: absolute or relative path of file
 */
void base64_file(const char *file) 
{
    // file size should less than 5 MB
    FILE *fp = fopen(file, "rb");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    } else {
        struct stat *buf = malloc(sizeof(struct stat));
        stat(file, buf);
        if (buf->st_size > 5 * 1024 * 1024) {
            fprintf(stderr, "Only process file less than 5 MB.\n");
            exit(EXIT_FAILURE);
        } else {
            int c, bytes_count = 0;
            char *base64bytes, *plainbytes = NULL;
            while ((c = fgetc(fp)) != EOF) {
                bytes_count += 1;
                plainbytes = realloc(plainbytes, bytes_count);
                plainbytes[bytes_count - 1] = c;
            }
            base64bytes = encode(plainbytes, bytes_count);
            printf("%s", base64bytes);
            fflush(stdout);
            fclose(fp);
            free(buf);
            free(plainbytes);
            free(base64bytes);
        }
    }
}

/* 
 * display help information
 */
void show_tips(int exit_code)
{
    fprintf(stdout, "Usage: base64 <-h|-e|-d|file>\n");
    fprintf(stdout, "  -h   display help information\n");
    fprintf(stdout, "  -e   encode input\n");
    fprintf(stdout, "  -d   decode input\n");
    fprintf(stdout, "  file encode file\n");
    exit(exit_code);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        show_tips(EXIT_FAILURE);
    } else if (strcmp(argv[1], "-h") == 0) {
        show_tips(EXIT_SUCCESS);
    } else if (strcmp(argv[1], "-e") == 0) {
        base64_stdio(ENCODE);
    } else if (strcmp(argv[1], "-d") == 0) {
        base64_stdio(DECODE);
    } else if (argv[1][0] == '-') {
        show_tips(EXIT_FAILURE);
    } else {
        base64_file(argv[1]);
    }
    return 0;
}