#include "kerl.h"
#include "conversion.h"
#include "os.h"
#include "cx.h"
#include "sha3.h"

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif

#ifndef BYTE_ORDER
// TODO: adapt byte order if necessary
#define BYTE_ORDER LITTLE_ENDIAN
#endif

//sha3 is 424 bytes long
// cx_sha3_t sha3;
SHA3_CTX ctx;
static unsigned char sha3_bytes_out[48] = {0};

int kerl_initialize(void)
{
    keccak_384_Init(&ctx);
    //cx_keccak_init(&sha3, 384);
    return 0;
}

int kerl_absorb_bytes(unsigned char *bytes_in, uint16_t len)
{
    //cx_hash((cx_hash_t *)&sha3, CX_LAST, bytes_in, len, sha3_bytes_out);
    keccak_Update(&ctx, bytes_in, len);
    return 0;
}

int kerl_finalize(unsigned char *bytes_out, uint16_t len)
{
    //memcpy(bytes_out, sha3_bytes_out, len);
    keccak_Final(&ctx, bytes_out);
    return 0;
}

uint32_t change_endianess(uint32_t i) {
    return ((i & 0xFF) << 24) |
    ((i & 0xFF00) << 8) |
    ((i >> 8) & 0xFF00) |
    ((i >> 24) & 0xFF);
}

void words_to_bytes(const int32_t *words, unsigned char *bytes,
                    const uint8_t num_words) {

    memcpy(bytes, words, num_words * 4);

#if BYTE_ORDER == LITTLE_ENDIAN
    // swap endianness on little-endian hardware
    uint32_t *p = (uint32_t *)bytes;
    for (uint8_t i = 0; i < num_words; i++) {
        *p = change_endianess(*p);
        p++;
    }
#endif // BYTE_ORDER == LITTLE_ENDIAN
}

void bytes_to_words(const unsigned char *bytes, int32_t *words,
                    const uint8_t num_words) {

    memcpy(words, bytes, num_words * 4);

#if BYTE_ORDER == LITTLE_ENDIAN
    // swap endianness on little-endian hardware
    uint32_t *p = words;
    for (uint8_t i = 0; i < num_words; i++) {
        *p = change_endianess(*p);
        p++;
    }
#endif // BYTE_ORDER == LITTLE_ENDIAN
}

int kerl_absorb_trits_single(const trit_t *trits_in)
{
    // First, convert to bytes
    int32_t words[12];
    unsigned char bytes[48];
    trits_to_words_u(trits_in, words);
    words_to_bytes(words, bytes, 12);

    return kerl_absorb_bytes(bytes, 48);
}

int kerl_absorb_trits(trit_t *trits_in, uint16_t len)
{
    //absorbing trits happens in 243 trit chunks
    for (uint8_t i = 0; i < (len/243); i++) {
        kerl_absorb_trits_single(trits_in + i*243);
    }

    return 0;
}

int kerl_squeeze_trits_single(trit_t *trits_out)
{
    unsigned char bytes_out[48];
    int32_t words[12];

    kerl_finalize(bytes_out, 48);

    bytes_to_words(bytes_out, words, 12);
    words_to_trits_u(words, trits_out);

    // Last trit zero
    trits_out[242] = 0;

    // Flip bytes
    for (uint8_t i = 0; i < 48; i++) {
        bytes_out[i] = ~bytes_out[i];
    }

    keccak_384_Init(&ctx);
    keccak_Update(&ctx, bytes_out, 48);

    return 0;
}

int kerl_squeeze_trits(trit_t *trits_out, uint16_t len)
{
    for (uint8_t i = 0; i < (len/243); i++) {
        kerl_squeeze_trits_single(trits_out + i*243);
    }

    return 0;
}

int kerl_absorb_trints_single(trint_t *trints_in)
{
    // First, convert to bytes
    uint32_t words[12] = {0};
    unsigned char bytes[48];

    //Convert straight from trints to words
    trints_to_words_u_mem(trints_in, words);
    words_to_bytes(words, bytes, 12);

    return kerl_absorb_bytes(bytes, 48);
}

int kerl_absorb_trints(trint_t *trints_in, uint16_t len) {
    for (uint8_t i = 0; i < (len / 49); i++) {
        kerl_absorb_trints_single(trints_in + i * 49);
    }

    return 0;
}

int kerl_squeeze_trints_single(trint_t *trints_out)
{
    unsigned char bytes_out[48];
    int32_t words[12];

    kerl_finalize(bytes_out, 48);

    bytes_to_words(bytes_out, words, 12);
    words_to_trints_u_mem(words, &trints_out[0]);

    //-- Setting last trit to 0
    trit_t trits[3];
    //grab and store last clump of 3 trits
    trint_to_trits(trints_out[48], &trits[0], 3);
    trits[2] = 0; //set last trit to 0
    //convert new trit set back to trint and store
    trints_out[48] = trits_to_trint(&trits[0], 3);

    // Flip bytes
    for (uint8_t i = 0; i < 48; i++) {
        bytes_out[i] = ~bytes_out[i];
    }

    kerl_initialize();
    kerl_absorb_bytes(bytes_out, 48);

    return 0;
}

int kerl_squeeze_trints(trint_t *trints_out, uint16_t len)
{
    for (uint8_t i = 0; i < (len / 49); i++) {
        kerl_squeeze_trints_single(trints_out + i * 49);
    }

    return 0;
}
