//see Boneh, Boyen and Shacham, "Short Group Signatures"
#include "pbc_sig.h"
#include <pbc/pbc_test.h>

int main(int argc, char **argv)
{
    bbs_sys_param_t sp;
    bbs_group_public_key_t gpk;
    bbs_manager_private_key_t gmsk;
    bbs_group_private_key_t gsk[5];
    pairing_t pairing;
    unsigned char *sig;
    int result;
    element_t A;
    double t0, t1;

    pbc_demo_pairing_init(pairing, argc, argv);

    printf("gen sys param...\n");
    bbs_gen_sys_param(sp, pairing);
    printf("gen keys...\n");
    t0 = pbc_get_time();
    bbs_gen(gpk, gmsk, 5, gsk, sp);
    t1 = pbc_get_time();
    printf("%fs elapsed\n", t1 - t0);
    t0 = t1;
    printf("sign...\n");
    sig = (unsigned char *) pbc_malloc(sp->signature_length);
    bbs_sign(sig, 0, NULL, gpk, gsk[0]);
    t1 = pbc_get_time();
    printf("%fs elapsed\n", t1 - t0);
    t0 = t1;
    printf("verify...\n");
    result = bbs_verify(sig, 0, NULL, gpk);
    if (result) {
	printf("signature verifies\n");
    } else {
	printf("signature does not verify\n");
    }
    t1 = pbc_get_time();
    printf("%fs elapsed\n", t1 - t0);
    t0 = t1;
    element_init_G1(A, pairing);
    bbs_open(A, gpk, gmsk, 0, NULL, sig);
    element_printf("open A = %B\n", A);
    element_printf("gsk0 A = %B\n", gsk[0]->A);
    t1 = pbc_get_time();
    printf("%fs elapsed\n", t1 - t0);
    t0 = t1;

    return 0;
}
