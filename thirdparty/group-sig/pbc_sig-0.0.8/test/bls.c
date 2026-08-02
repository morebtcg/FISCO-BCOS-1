//Boneh-Lynn-Shacham short signatures demo
//This one uses the signature library and is more realistic
#include "pbc_sig.h"
#include <pbc/pbc_test.h>

int main(int argc, char **argv)
{
    pairing_t pairing;
    bls_sys_param_t param;
    bls_public_key_t pk;
    bls_private_key_t sk;
    unsigned char *sig;

    printf("reading pairing from stdin...\n");

    pbc_demo_pairing_init(pairing, argc, argv);
    printf("generating BLS system parameters...\n");
    bls_gen_sys_param(param, pairing);
    printf("generating key pair...\n");
    bls_gen(pk, sk, param);

    sig = (unsigned char *) pbc_malloc(param->signature_length);

    printf("signing...\n");
    bls_sign(sig, 11, (unsigned char *) "hello world", sk);

    printf("verifying...\n");
    if (bls_verify(sig, 11, (unsigned char *) "hello world", pk)) {
	printf("signature verifies\n");
    } else {
	printf("signature does not verify\n");
    }

    pbc_free(sig);
    bls_clear_public_key(pk);
    bls_clear_private_key(sk);
    bls_clear_sys_param(param);
    pairing_clear(pairing);
    return 0;
}
