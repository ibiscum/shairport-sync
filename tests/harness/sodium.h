#pragma once

#include <stdint.h>
#include <sys/types.h>

/* Minimal libsodium surface used by tests/test_rtp_minimal_stubs.c. */
#define crypto_aead_chacha20poly1305_ietf_ABYTES 16U

int crypto_aead_chacha20poly1305_ietf_decrypt(unsigned char *m,
                                               unsigned long long *mlen_p,
                                               unsigned char *nsec,
                                               const unsigned char *c,
                                               unsigned long long clen,
                                               const unsigned char *ad,
                                               unsigned long long adlen,
                                               const unsigned char *npub,
                                               const unsigned char *k);
