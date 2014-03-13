#ifndef _DES_H
#define _DES_H

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned long int
#endif

#ifdef __cplusplus
extern "C"
{
#endif
  typedef struct
  {
    uint32 esk[32];		// DES encryption subkeys 
    uint32 dsk[32];		// DES decryption subkeys 
  } des_context;

  typedef struct
  {
    uint32 esk[96];		// Triple-DES encryption subkeys 
    uint32 dsk[96];		// Triple-DES decryption subkeys 
  } des3_context;

  int des_set_key (des_context * ctx, const uint8 key[8]);
  void des_encrypt (des_context * ctx, const uint8 input[8], uint8 output[8]);
  void des_decrypt (des_context * ctx, const uint8 input[8], uint8 output[8]);
  int des3_set_2keys (des3_context * ctx, const uint8 key1[8],
		      const uint8 key2[8]);

/*
int  des3_set_3keys( des3_context *ctx, uint8 key1[8], uint8 key2[8],
                                        uint8 key3[8] );
*/
  void des3_encrypt (des3_context * ctx, uint8 input[8], uint8 output[8]);
  void des3_decrypt (des3_context * ctx, uint8 input[8], uint8 output[8]);

// Encrypt input data, using key, storing ciphered bytes to output
// Using 3DES and Outer CBC
// - length of key have to be 16 bytes
// - length of input have to be a multiple of 8 bytes !
  void encrypt_3des (const unsigned char *key, unsigned char *input,
		     int inputLength, unsigned char **output,
		     int *outputLength);

// Decrypt input data, using key, storing ciphered bytes to output
// Using 3DES and Outer CBC
// - length of key have to be 16 bytes
// - length of input have to be a multiple of 8 bytes !
  void decrypt_3des (const unsigned char *key, unsigned char *input,
		     int inputLength, unsigned char **output,
		     int *outputLength);


  void encrypt_des (const unsigned char *key, unsigned char *input,
		    int inputLength, unsigned char **output,
		    int *outputLength);

  void decrypt_des (const unsigned char *key, unsigned char *input,
		    int inputLength, unsigned char **output,
		    int *outputLength);

#ifdef __cplusplus
}
#endif
#endif				/* des.h */
