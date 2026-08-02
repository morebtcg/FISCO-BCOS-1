//Cha-Cheon Identity-Based Signatures demo
//Sakai-Kasahara-Schnorr code has been commented out due to patent
#include "pbc_sig.h"
#include <pbc/pbc_test.h>

int main(int argc, char **argv)
{
    pairing_t pairing;
    ib_sys_param_t param;
    ib_master_key_t master;
    ib_private_key_t sk;
    unsigned char *sig;

    printf("reading pairing from stdin...\n");
    pbc_demo_pairing_init(pairing, argc, argv);
    printf("generating identity-based system parameters...\n");
    ib_sys_param_init(param, pairing);
    ib_master_key_init(master, param);
    ib_setup(param, master);
    printf("extracting private key...\n");

    ib_private_key_init(sk, param);
    ib_extract(sk, 5, (unsigned char *) "alice", master);

    sig = (unsigned char *) pbc_malloc(cc_signature_length(param));

    printf("Cha-Cheon signatures:\n");
    printf("signing...\n");
    cc_sign(sig, 11, (unsigned char *) "hello world", sk);

    printf("verifying...\n");
    if (cc_verify(sig, 11, (unsigned char *) "hello world",
		5, (unsigned char *) "alice", param)) {
	printf("signature verifies\n");
    } else {
	printf("signature does not verify\n");
    }

    pbc_free(sig);

    /* Cannot use this scheme: it is patented
    sig = (unsigned char *) pbc_malloc(skschnorr_signature_length(param));

    printf("Sakai-Kasahara-Schnorr signatures:\n");
    printf("signing...\n");
    skschnorr_sign(sig, 11, (unsigned char *) "hello world", sk);

    {
	int i;
	int n = skschnorr_signature_length(param);
	printf("signature: ");
	for (i=0; i<n; i++) {
	    printf("%02X", (unsigned int) sig[i]);
	}
	printf("\n");
    }

    printf("verifying...\n");
    if (skschnorr_verify(sig, 11, (unsigned char *) "hello world",
		5, (unsigned char *) "alice", param)) {
	printf("signature verifies\n");
    } else {
	printf("signature does not verify\n");
    }
    pbc_free(sig);
    */

    ib_private_key_clear(sk);
    ib_master_key_clear(master);
    ib_sys_param_clear(param);
    pairing_clear(pairing);

    return 0;
}
