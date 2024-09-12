#ifndef JWT_H
#define JWT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h> // You should have Jansson library for JSON handling
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#define SECRET_KEY "default_key"

// Base64 URL encoding (removes +, / and pads with =)
char *base64url_encode(const unsigned char *input, int length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // Prevent newline characters

    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    
    // Create a new string for base64
    char *base64Encoded = (char *)malloc(bufferPtr->length);
    memcpy(base64Encoded, bufferPtr->data, bufferPtr->length);
    
    // Now we need to convert to URL-safe base64
    for (int i = 0; i < bufferPtr->length; i++) {
        if (base64Encoded[i] == '+') base64Encoded[i] = '-';
        else if (base64Encoded[i] == '/') base64Encoded[i] = '_';
    }

    // Remove padding '=' characters
    int encodedLen = bufferPtr->length;
    while (encodedLen > 0 && base64Encoded[encodedLen - 1] == '=') {
        encodedLen--;
    }
    base64Encoded[encodedLen] = '\0';

    BIO_free_all(bio);
    
    return base64Encoded;
}

// Function to create JWT token
char *generate_jwt(const char *headerJson, const char *payloadJson) {
    // Encode header
    char *encodedHeader = base64url_encode((unsigned char *)headerJson, strlen(headerJson));
    
    // Encode payload
    char *encodedPayload = base64url_encode((unsigned char *)payloadJson, strlen(payloadJson));
    
    // Create token before signing
    char *unsignedToken = (char *)malloc(strlen(encodedHeader) + strlen(encodedPayload) + 2);
    sprintf(unsignedToken, "%s.%s", encodedHeader, encodedPayload);
    
    // Create HMAC SHA256 signature
    unsigned char *sig = HMAC(EVP_sha256(), SECRET_KEY, strlen(SECRET_KEY), 
                              (unsigned char *)unsignedToken, strlen(unsignedToken), 
                              NULL, NULL);
    
    // Base64 URL encode signature
    char *encodedSignature = base64url_encode(sig, 32); // SHA256 produces 32-byte hash

    // Construct final JWT
    char *jwt = (char *)malloc(strlen(unsignedToken) + strlen(encodedSignature) + 2);
    sprintf(jwt, "%s.%s", unsignedToken, encodedSignature);
    
    // Clean up
    free(encodedHeader);
    free(encodedPayload);
    free(unsignedToken);
    free(encodedSignature);

    return jwt;
}

#endif
