#ifndef __PBC_SIG_H__
#define __PBC_SIG_H__

//pairing-based signatures library

#include <pbc/pbc.h>

struct bls_sys_param_s {
    pairing_ptr pairing;
    element_t g;
    int signature_length;
};
typedef struct bls_sys_param_s bls_sys_param_t[1];
typedef struct bls_sys_param_s *bls_sys_param_ptr;

struct bls_private_key_s {
    bls_sys_param_ptr param;
    element_t x;
};
typedef struct bls_private_key_s bls_private_key_t[1];
typedef struct bls_private_key_s *bls_private_key_ptr;

struct bls_public_key_s {
    bls_sys_param_ptr param;
    element_t gx;
};
typedef struct bls_public_key_s bls_public_key_t[1];
typedef struct bls_public_key_s *bls_public_key_ptr;

void bls_gen_sys_param(bls_sys_param_ptr param, pairing_ptr pairing);
void bls_clear_sys_param(bls_sys_param_ptr param);
void bls_gen(bls_public_key_ptr pk, bls_private_key_ptr sk, bls_sys_param_ptr param);
void bls_clear_public_key(bls_public_key_ptr pk);
void bls_clear_private_key(bls_private_key_ptr sk);
void bls_sign(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	bls_private_key_ptr sk);
int bls_verify(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	bls_public_key_ptr pk);

struct bb_sys_param_s {
    pairing_ptr pairing;
    int signature_length;
};
typedef struct bb_sys_param_s bb_sys_param_t[1];
typedef struct bb_sys_param_s *bb_sys_param_ptr;

struct bb_private_key_s {
    bb_sys_param_ptr param;
    element_t x, y;
};
typedef struct bb_private_key_s bb_private_key_t[1];
typedef struct bb_private_key_s *bb_private_key_ptr;

struct bb_public_key_s {
    bb_sys_param_ptr param;
    element_t g1, g2, u, v, z;
};
typedef struct bb_public_key_s bb_public_key_t[1];
typedef struct bb_public_key_s *bb_public_key_ptr;

void bb_gen_sys_param(bb_sys_param_ptr param, pairing_ptr pairing);
void bb_gen(bb_public_key_ptr pk, bb_private_key_ptr sk, bb_sys_param_ptr param);
void bb_sign(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	bb_public_key_ptr pk, bb_private_key_ptr sk);
int bb_verify(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	bb_public_key_ptr pk);

struct ib_sys_param_s {
    pairing_ptr pairing;
    element_t g, gx;
};
typedef struct ib_sys_param_s ib_sys_param_t[1];
typedef struct ib_sys_param_s *ib_sys_param_ptr;

struct ib_master_key_s {
    ib_sys_param_ptr param;
    element_t x;
};
typedef struct ib_master_key_s ib_master_key_t[1];
typedef struct ib_master_key_s *ib_master_key_ptr;

struct ib_private_key_s {
    ib_sys_param_ptr param;
    element_t q;
    element_t d;
};
typedef struct ib_private_key_s ib_private_key_t[1];
typedef struct ib_private_key_s *ib_private_key_ptr;

void ib_sys_param_print(FILE *fp, ib_sys_param_t param);
void ib_master_key_print(FILE *fp, ib_master_key_t key);
void ib_private_key_print(FILE *fp, ib_private_key_t key);

void ib_setup(ib_sys_param_ptr param, ib_master_key_ptr pkgpriv);
void ib_extract(ib_private_key_ptr priv, unsigned int idlen, unsigned char *id,
	ib_master_key_ptr sk);
int cc_signature_length(ib_sys_param_ptr param);
void cc_sign(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	ib_private_key_ptr sk);
int cc_verify(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	unsigned int idlen, unsigned char *id, ib_sys_param_ptr param);

static inline void ib_sys_param_init(ib_sys_param_t param, pairing_ptr pairing)
{
    param->pairing = pairing;
    element_init(param->g, pairing->G2);
    element_init(param->gx, pairing->G2);
}

static inline void ib_master_key_init(ib_master_key_ptr master, ib_sys_param_ptr param)
{
    element_init(master->x, param->pairing->Zr);
    master->param = param;
}

static inline void ib_master_key_clear(ib_master_key_ptr master)
{
    element_clear(master->x);
}

static inline void ib_private_key_init(ib_private_key_ptr sk, ib_sys_param_ptr param)
{
    element_init(sk->d, param->pairing->G1);
    element_init(sk->q, param->pairing->G1);
}

static inline void ib_private_key_clear(ib_private_key_ptr sk)
{
    element_clear(sk->q);
    element_clear(sk->d);
}

static inline void ib_sys_param_clear(ib_sys_param_ptr param)
{
    element_clear(param->g);
    element_clear(param->gx);
}

void ib_sys_param_scan(ib_sys_param_t param, FILE *fp);

/* This scheme is patented
void skschnorr_sign(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	ib_private_key_ptr sk);
int skschnorr_signature_length(ib_sys_param_ptr param);
int skschnorr_verify(unsigned char *sig, unsigned int hashlen, unsigned char *hash,
	unsigned int idlen, unsigned char *id, ib_sys_param_ptr param);
*/

struct bbs_sys_param_s {
    pairing_ptr pairing;
    int signature_length;
};
typedef struct bbs_sys_param_s bbs_sys_param_t[1];
typedef struct bbs_sys_param_s *bbs_sys_param_ptr;

struct bbs_group_public_key_s {
    bbs_sys_param_ptr param;
    element_t g1, g2;
    element_t h, u, v, w;
    /* and precomputed values */
    element_t pr_g1_g2;
    element_t pr_h_g2;
    element_t pr_h_w;
    element_t pr_g1_g2_inv;
};
typedef struct bbs_group_public_key_s bbs_group_public_key_t[1];
typedef struct bbs_group_public_key_s *bbs_group_public_key_ptr;

struct bbs_group_private_key_s {
    bbs_sys_param_ptr param;
    element_t A;
    element_t x;
    /* and precomputed values */
    element_t pr_A_g2;
};
typedef struct bbs_group_private_key_s bbs_group_private_key_t[1];
typedef struct bbs_group_private_key_s *bbs_group_private_key_ptr;

struct bbs_manager_private_key_s {
    bbs_sys_param_ptr param;
    element_t xi1, xi2;
};
typedef struct bbs_manager_private_key_s bbs_manager_private_key_t[1];
typedef struct bbs_manager_private_key_s *bbs_manager_private_key_ptr;

void bbs_gen_sys_param(bbs_sys_param_ptr param, pairing_ptr pairing);
void bbs_gen(bbs_group_public_key_ptr gpk, bbs_manager_private_key_ptr gmsk,
	int n, bbs_group_private_key_t *gsk, bbs_sys_param_ptr param);
void bbs_sign(unsigned char *sig,
	int hashlen, void *hash,
	bbs_group_public_key_ptr gpk, bbs_group_private_key_ptr gsk);
int bbs_verify(unsigned char *sig,
	int hashlen, void *hash,
	bbs_group_public_key_ptr gpk);
int bbs_open(element_ptr A, bbs_group_public_key_ptr gpk,
	bbs_manager_private_key_ptr gmsk,
	int hashlen, void *hash, unsigned char *sig);
#endif //__PBC_SIG_H__
