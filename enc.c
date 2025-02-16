//sgn

//	Build it as follows
//	gcc enc.c -o enc -lssl -lcrypto

#include <string.h>
#include <openssl/evp.h>
#include <openssl/opensslconf.h>
#include <stddef.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>

#define ONE_MB (1024 * 1024)
#define ENCRYPTED_FILE_NAME		"config_file.bin"

void handleErrors() {}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

int main (int argc, char *argv[])
{
	char plaintext[ONE_MB];
	unsigned char ciphertext[ONE_MB];
	unsigned char decryptedtext[ONE_MB];
	int decryptedtext_len, ciphertext_len, plaintext_len;

	unsigned char *key	= (unsigned char *)"01234567890123456789012345678901";
	unsigned char *iv	= (unsigned char *)"0123456789012345";

	if(argc != 2) {
		printf("Usage: enc plain_text.json\n");
		return 0;
	}

    /* --------------------------------Read the plain text file-------------------------------- */
	FILE *fp	= fopen(argv[1], "r");
	if(NULL == fp) {
		printf("File %s not found\n", argv[1]);
		return 0;
	}

	plaintext_len	= fread(plaintext, 1, ONE_MB, fp);
    plaintext[plaintext_len] = '\0';
	printf("Content of plain text file:\n%s\n", plaintext);
	fclose(fp);




    /* --------------------------------Encrypt the plain text file-------------------------------- */
    ciphertext_len = encrypt(plaintext, strlen((char *)plaintext), key, iv, ciphertext);
	fp	= fopen(ENCRYPTED_FILE_NAME, "wb");
	if(NULL == fp) {
		printf("Failed writing to encrpyted file to %s\n", ENCRYPTED_FILE_NAME);
		return 0;
	}
	fwrite(ciphertext, 1, ciphertext_len, fp);
	fclose(fp);
    printf("Ciphertext is:\n");
    BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);
	printf("\n\nCiphertext is written to %s. Copy it to \"Technospurs_Base_Folder/cfg\"\n\n\n", ENCRYPTED_FILE_NAME); 




	/* --------------------------------Check if decrypting properly-------------------------------- */
	printf("Now reading the %s to make sure it is decrypting properly\n", ENCRYPTED_FILE_NAME);
	fp	= fopen(ENCRYPTED_FILE_NAME, "rb");
	if(NULL == fp) {
		printf("%s not available\n", ENCRYPTED_FILE_NAME);
		return 0;
	}
	ciphertext_len	= fread(ciphertext, 1, ONE_MB, fp);
	fclose(fp);

    decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv, decryptedtext);
    decryptedtext[decryptedtext_len] = '\0';
    printf("Decrypted text length is: %d & text is\n", ciphertext_len);
    printf("%s\n", decryptedtext);


    return 0;
}




















