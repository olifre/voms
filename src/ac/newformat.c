/*********************************************************************
 *
 * Authors: Vincenzo Ciaschini - Vincenzo.Ciaschini@cnaf.infn.it 
 *
 * Copyright (c) Members of the EGEE Collaboration. 2004-2010.
 * See http://www.eu-egee.org/partners/ for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Parts of this code may be based upon or even include verbatim pieces,
 * originally written by other people, in which case the original header
 * follows.
 *
 *********************************************************************/
#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/asn1_mac.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/stack.h>
#include <openssl/safestack.h>
#include "newformat.h"
#include "acerrors.h"
#include "attributes.h"

#ifndef VOMS_MAYBECONST
#if defined(D2I_OF)
#define VOMS_MAYBECONST const
#else
#define VOMS_MAYBECONST
#endif
#endif

int i2d_AC_ATTR(AC_ATTR *a, unsigned char **pp)
{
  char text[1000];

  M_ASN1_I2D_vars(a);

  if (!i2t_ASN1_OBJECT(text,999,a->type))
    return 0;
  else if (!((strcmp(text, "idacagroup") == 0) || (strcmp(text,"idatcap") == 0)))
    return 0;
  
  M_ASN1_I2D_len(a->type, i2d_ASN1_OBJECT);
  M_ASN1_I2D_len_SET_type(AC_IETFATTR, a->ietfattr, i2d_AC_IETFATTR);
  M_ASN1_I2D_seq_total();
  M_ASN1_I2D_put(a->type, i2d_ASN1_OBJECT);
  M_ASN1_I2D_put_SET_type(AC_IETFATTR,a->ietfattr, i2d_AC_IETFATTR);
  M_ASN1_I2D_finish();
}

AC_ATTR *d2i_AC_ATTR(AC_ATTR **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  char text[1000];

  M_ASN1_D2I_vars(a, AC_ATTR *, AC_ATTR_new);
  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->type, d2i_ASN1_OBJECT);

  if (!i2t_ASN1_OBJECT(text,999, ret->type)) {
    c.error = ASN1_R_NOT_ENOUGH_DATA;
    goto err;
  }

  if (strcmp(text, "idatcap") == 0)
    M_ASN1_D2I_get_set_type(AC_IETFATTR, ret->ietfattr, d2i_AC_IETFATTR, AC_IETFATTR_free);
  M_ASN1_D2I_Finish(a, AC_ATTR_free, ASN1_F_D2I_AC_ATTR);
}

AC_ATTR *AC_ATTR_new()
{
  AC_ATTR *ret = NULL;
  ASN1_CTX c;
  M_ASN1_New_Malloc(ret, AC_ATTR);
  M_ASN1_New(ret->type,  ASN1_OBJECT_new);
  M_ASN1_New(ret->ietfattr, sk_AC_IETFATTR_new_null);
  return ret;
  M_ASN1_New_Error(AC_F_ATTR_New);
}

void AC_ATTR_free(AC_ATTR *a)
{
  if (!a)
    return;

  ASN1_OBJECT_free(a->type);
  sk_AC_IETFATTR_pop_free(a->ietfattr, AC_IETFATTR_free);
  OPENSSL_free(a);
}

int i2d_AC_IETFATTR(AC_IETFATTR *a, unsigned char **pp)
{
  M_ASN1_I2D_vars(a);
  M_ASN1_I2D_len_IMP_opt(a->names, i2d_GENERAL_NAMES);
  M_ASN1_I2D_len_SEQUENCE(a->values,    i2d_AC_IETFATTRVAL);
  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put_IMP_opt(a->names, i2d_GENERAL_NAMES, 0);
  M_ASN1_I2D_put_SEQUENCE(a->values,    i2d_AC_IETFATTRVAL);
  M_ASN1_I2D_finish();
}

AC_IETFATTR *d2i_AC_IETFATTR(AC_IETFATTR **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC_IETFATTR *, AC_IETFATTR_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get_IMP_opt(ret->names, d2i_GENERAL_NAMES, 0, V_ASN1_SEQUENCE);
  M_ASN1_D2I_get_seq(ret->values, d2i_AC_IETFATTRVAL, AC_IETFATTRVAL_free);
  M_ASN1_D2I_Finish(a, AC_IETFATTR_free, ASN1_F_D2I_AC_IETFATTR);
}

AC_IETFATTR *AC_IETFATTR_new()
{
  AC_IETFATTR *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret,  AC_IETFATTR);
  M_ASN1_New(ret->values, sk_AC_IETFATTRVAL_new_null);
  M_ASN1_New(ret->names,  sk_GENERAL_NAME_new_null);
  return ret;
  M_ASN1_New_Error(AC_F_IETFATTR_New);
}

void AC_IETFATTR_free (AC_IETFATTR *a)
{
  if (a==NULL) return;

  sk_GENERAL_NAME_pop_free(a->names, GENERAL_NAME_free);
  sk_AC_IETFATTRVAL_pop_free(a->values, AC_IETFATTRVAL_free);
  OPENSSL_free(a);
}

    
int i2d_AC_IETFATTRVAL(AC_IETFATTRVAL *a, unsigned char **pp)
{
  if (a->type == V_ASN1_OCTET_STRING || a->type == V_ASN1_OBJECT ||
      a->type == V_ASN1_UTF8STRING)
    return (i2d_ASN1_bytes((ASN1_STRING *)a, pp, a->type, V_ASN1_UNIVERSAL));

  ASN1err(ASN1_F_I2D_AC_IETFATTRVAL,ASN1_R_WRONG_TYPE);
  return -1;
}

AC_IETFATTRVAL *d2i_AC_IETFATTRVAL(AC_IETFATTRVAL **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  unsigned char tag;
  tag = **pp & ~V_ASN1_CONSTRUCTED;
  if (tag == (V_ASN1_OCTET_STRING|V_ASN1_UNIVERSAL))
    return d2i_ASN1_OCTET_STRING(a, pp, length);
  if (tag == (V_ASN1_OBJECT|V_ASN1_UNIVERSAL))
    return (AC_IETFATTRVAL *)d2i_ASN1_OBJECT((ASN1_OBJECT **)a, pp, length);
  if (tag == (V_ASN1_UTF8STRING|V_ASN1_UNIVERSAL))
    return d2i_ASN1_UTF8STRING(a, pp, length);
  ASN1err(ASN1_F_D2I_AC_IETFATTRVAL, ASN1_R_WRONG_TYPE);
  return (NULL);
}

AC_IETFATTRVAL *AC_IETFATTRVAL_new()
{
  return ASN1_STRING_type_new(V_ASN1_UTF8STRING);
}

void AC_IETFATTRVAL_free(AC_IETFATTRVAL *a)
{
  ASN1_STRING_free(a);
}

int i2d_AC_DIGEST(AC_DIGEST *a, unsigned char **pp)
{
  M_ASN1_I2D_vars(a);
  M_ASN1_I2D_len(a->type,          i2d_ASN1_ENUMERATED);
  M_ASN1_I2D_len(a->oid,           i2d_ASN1_OBJECT);
  M_ASN1_I2D_len(a->algor,         i2d_X509_ALGOR);
  M_ASN1_I2D_len(a->digest,        i2d_ASN1_BIT_STRING);
  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put(a->type,         i2d_ASN1_ENUMERATED);
  M_ASN1_I2D_put(a->oid,          i2d_ASN1_OBJECT);
  M_ASN1_I2D_put(a->algor,        i2d_X509_ALGOR);
  M_ASN1_I2D_put(a->digest,       i2d_ASN1_BIT_STRING);
  M_ASN1_I2D_finish();
}

AC_DIGEST *d2i_AC_DIGEST(AC_DIGEST **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC_DIGEST *, AC_DIGEST_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->type,        d2i_ASN1_ENUMERATED);
  M_ASN1_D2I_get(ret->oid,         d2i_ASN1_OBJECT);
  M_ASN1_D2I_get(ret->algor,       d2i_X509_ALGOR);
  M_ASN1_D2I_get(ret->digest,      d2i_ASN1_BIT_STRING);
  M_ASN1_D2I_Finish(a, AC_DIGEST_free, AC_F_D2I_AC_DIGEST);
}

AC_DIGEST *AC_DIGEST_new(void)
{
  AC_DIGEST *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret, AC_DIGEST);
  M_ASN1_New(ret->type, M_ASN1_ENUMERATED_new);
  ret->oid = NULL;
  ret->algor = NULL;
  M_ASN1_New(ret->algor,  X509_ALGOR_new);
  M_ASN1_New(ret->digest, M_ASN1_BIT_STRING_new);
  return(ret);
  M_ASN1_New_Error(AC_F_AC_DIGEST_New);
}

void AC_DIGEST_free(AC_DIGEST *a)
{
  if (a==NULL) return;

  ASN1_ENUMERATED_free(a->type);
  ASN1_OBJECT_free(a->oid);
  X509_ALGOR_free(a->algor);
  ASN1_BIT_STRING_free(a->digest);
  OPENSSL_free(a);
}

int i2d_AC_IS(AC_IS *a, unsigned char **pp)
{
  M_ASN1_I2D_vars(a);
  M_ASN1_I2D_len(a->issuer,      i2d_GENERAL_NAMES);
  M_ASN1_I2D_len(a->serial,      i2d_ASN1_INTEGER);
  M_ASN1_I2D_len_IMP_opt(a->uid, i2d_ASN1_BIT_STRING);
  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put(a->issuer,      i2d_GENERAL_NAMES);
  M_ASN1_I2D_put(a->serial,      i2d_ASN1_INTEGER);
  M_ASN1_I2D_put_IMP_opt(a->uid, i2d_ASN1_BIT_STRING, V_ASN1_BIT_STRING);
  M_ASN1_I2D_finish();
}

AC_IS *d2i_AC_IS(AC_IS **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC_IS *, AC_IS_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->issuer,  d2i_GENERAL_NAMES);
  M_ASN1_D2I_get(ret->serial,  d2i_ASN1_INTEGER);
  M_ASN1_D2I_get_opt(ret->uid, d2i_ASN1_BIT_STRING, V_ASN1_BIT_STRING);
  M_ASN1_D2I_Finish(a, AC_IS_free, AC_F_D2I_AC_IS);
}

AC_IS *AC_IS_new(void)
{
  AC_IS *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret, AC_IS);
  M_ASN1_New(ret->issuer, GENERAL_NAMES_new);
  M_ASN1_New(ret->serial, M_ASN1_INTEGER_new);
  ret->uid = NULL;
  return(ret);
  M_ASN1_New_Error(AC_F_AC_IS_New);
}

void AC_IS_free(AC_IS *a)
{
  if (a==NULL) return;

  GENERAL_NAMES_free(a->issuer);
  M_ASN1_INTEGER_free(a->serial);
  M_ASN1_BIT_STRING_free(a->uid);
  OPENSSL_free(a);
}

int i2d_AC_FORM(AC_FORM *a, unsigned char **pp)
{
  M_ASN1_I2D_vars(a);

  M_ASN1_I2D_len(a->names,  i2d_GENERAL_NAMES);
  M_ASN1_I2D_len_IMP_opt(a->is,     i2d_AC_IS);
  M_ASN1_I2D_len_IMP_opt(a->digest, i2d_AC_DIGEST);
  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put(a->names,  i2d_GENERAL_NAMES);
  M_ASN1_I2D_put_IMP_opt(a->is,     i2d_AC_IS, 0);
  M_ASN1_I2D_put_IMP_opt(a->digest, i2d_AC_DIGEST, 1);
  M_ASN1_I2D_finish();
}

AC_FORM *d2i_AC_FORM(AC_FORM **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC_FORM *, AC_FORM_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->names,  d2i_GENERAL_NAMES);
  M_ASN1_D2I_get_IMP_opt(ret->is,     d2i_AC_IS, 0, V_ASN1_SEQUENCE);
  M_ASN1_D2I_get_IMP_opt(ret->digest, d2i_AC_DIGEST, 1, V_ASN1_SEQUENCE);
  M_ASN1_D2I_Finish(a, AC_FORM_free, ASN1_F_D2I_AC_FORM);
}

AC_FORM *AC_FORM_new(void)
{
  AC_FORM *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret, AC_FORM);
  ret->names = GENERAL_NAMES_new();
  ret->is = NULL;
  ret->digest = NULL;
  return(ret);
  M_ASN1_New_Error(AC_F_AC_FORM_New);
}

void AC_FORM_free(AC_FORM *a)
{
  if (a==NULL) return;

  GENERAL_NAMES_free(a->names);
  AC_IS_free(a->is);
  AC_DIGEST_free(a->digest);
  OPENSSL_free(a);

}

int i2d_AC_HOLDER(AC_HOLDER *a, unsigned char **pp)
{
  M_ASN1_I2D_vars(a);

  M_ASN1_I2D_len_IMP_opt(a->baseid, i2d_AC_IS);
  M_ASN1_I2D_len_IMP_opt(a->name, i2d_GENERAL_NAMES);
  M_ASN1_I2D_len_IMP_opt(a->digest, i2d_AC_DIGEST);
  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put_IMP_opt(a->baseid, i2d_AC_IS, 0);	       
  M_ASN1_I2D_put_IMP_opt(a->name, i2d_GENERAL_NAMES, 1);
  M_ASN1_I2D_put_IMP_opt(a->digest, i2d_AC_DIGEST, 2);
  M_ASN1_I2D_finish();
}

AC_HOLDER *d2i_AC_HOLDER(AC_HOLDER **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC_HOLDER *, AC_HOLDER_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get_IMP_opt(ret->baseid, d2i_AC_IS, 0, V_ASN1_SEQUENCE);
  M_ASN1_D2I_get_IMP_opt(ret->name, d2i_GENERAL_NAMES, 1, V_ASN1_SEQUENCE);
  M_ASN1_D2I_get_IMP_opt(ret->digest, d2i_AC_DIGEST, 2, V_ASN1_SEQUENCE);
  M_ASN1_D2I_Finish(a, AC_HOLDER_free, ASN1_F_D2I_AC_HOLDER);
}

AC_HOLDER *AC_HOLDER_new(void)
{
  AC_HOLDER *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret, AC_HOLDER);
  M_ASN1_New(ret->baseid, AC_IS_new);
  ret->name = NULL;
  ret->digest = NULL;
  return(ret);
  M_ASN1_New_Error(ASN1_F_AC_HOLDER_New);
}

void AC_HOLDER_free(AC_HOLDER *a)
{
  if (!a) return;

  AC_IS_free(a->baseid);
  GENERAL_NAMES_free(a->name);
  AC_DIGEST_free(a->digest);
  OPENSSL_free(a);
}

/* new AC_VAL functions by valerio */


AC_VAL *AC_VAL_new(void)
{
  AC_VAL *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret, AC_VAL);

  ret->notBefore = NULL;
  ret->notAfter = NULL;
  
  return(ret);
  M_ASN1_New_Error(ASN1_F_AC_VAL_New);
}

int i2d_AC_VAL(AC_VAL *a, unsigned char **pp) 
{
  M_ASN1_I2D_vars(a);

  M_ASN1_I2D_len(a->notBefore, i2d_ASN1_GENERALIZEDTIME);
  M_ASN1_I2D_len(a->notAfter,  i2d_ASN1_GENERALIZEDTIME);

  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put(a->notBefore, i2d_ASN1_GENERALIZEDTIME);
  M_ASN1_I2D_put(a->notAfter,  i2d_ASN1_GENERALIZEDTIME);

  M_ASN1_I2D_finish();
}

AC_VAL *d2i_AC_VAL(AC_VAL **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC_VAL *, AC_VAL_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();

  M_ASN1_D2I_get(ret->notBefore, d2i_ASN1_GENERALIZEDTIME);
  M_ASN1_D2I_get(ret->notAfter,  d2i_ASN1_GENERALIZEDTIME);

  M_ASN1_D2I_Finish(a, AC_VAL_free, AC_F_D2I_AC);
}

void AC_VAL_free(AC_VAL *a)
{

  if (a==NULL) return;

  M_ASN1_GENERALIZEDTIME_free(a->notBefore);
  M_ASN1_GENERALIZEDTIME_free(a->notAfter);

  OPENSSL_free(a);
}


/* end of new code */


int i2d_AC_INFO(AC_INFO *a, unsigned char **pp)
{
  M_ASN1_I2D_vars(a);

  M_ASN1_I2D_len(a->version,  i2d_ASN1_INTEGER);
  M_ASN1_I2D_len(a->holder,   i2d_AC_HOLDER);
  M_ASN1_I2D_len_IMP_opt(a->form, i2d_AC_FORM);
  M_ASN1_I2D_len(a->alg,      i2d_X509_ALGOR);
  M_ASN1_I2D_len(a->serial,   i2d_ASN1_INTEGER);
  M_ASN1_I2D_len(a->validity, i2d_AC_VAL);
  M_ASN1_I2D_len_SEQUENCE(a->attrib, i2d_AC_ATTR);
  M_ASN1_I2D_len_IMP_opt(a->id,       i2d_ASN1_BIT_STRING);
  M_ASN1_I2D_len_SEQUENCE_opt(a->exts, i2d_X509_EXTENSION);
  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put(a->version,  i2d_ASN1_INTEGER);
  M_ASN1_I2D_put(a->holder,   i2d_AC_HOLDER);
  M_ASN1_I2D_put_IMP_opt(a->form,   i2d_AC_FORM, 0);
  M_ASN1_I2D_put(a->alg,      i2d_X509_ALGOR);
  M_ASN1_I2D_put(a->serial,   i2d_ASN1_INTEGER);
  M_ASN1_I2D_put(a->validity, i2d_AC_VAL);
  M_ASN1_I2D_put_SEQUENCE(a->attrib, i2d_AC_ATTR);
  M_ASN1_I2D_put_IMP_opt(a->id,       i2d_ASN1_BIT_STRING, V_ASN1_BIT_STRING);
  M_ASN1_I2D_put_SEQUENCE_opt(a->exts, i2d_X509_EXTENSION);
  M_ASN1_I2D_finish();
}

AC_INFO *d2i_AC_INFO(AC_INFO **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC_INFO *, AC_INFO_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->version,    d2i_ASN1_INTEGER);
  M_ASN1_D2I_get(ret->holder,     d2i_AC_HOLDER);
  M_ASN1_D2I_get_IMP_opt(ret->form,     d2i_AC_FORM, 0, V_ASN1_SEQUENCE);
  M_ASN1_D2I_get(ret->alg,        d2i_X509_ALGOR);
  M_ASN1_D2I_get(ret->serial,     d2i_ASN1_INTEGER);
  M_ASN1_D2I_get(ret->validity, d2i_AC_VAL);
  M_ASN1_D2I_get_seq(ret->attrib, d2i_AC_ATTR, AC_ATTR_free);
  M_ASN1_D2I_get_opt(ret->id,     d2i_ASN1_BIT_STRING, V_ASN1_BIT_STRING);
  M_ASN1_D2I_get_seq_opt(ret->exts,   d2i_X509_EXTENSION, X509_EXTENSION_free);
  M_ASN1_D2I_Finish(a, AC_INFO_free, AC_F_D2I_AC);
}

AC_INFO *AC_INFO_new(void)
{
  AC_INFO *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret, AC_INFO);
  M_ASN1_New(ret->version,  ASN1_INTEGER_new);
  M_ASN1_New(ret->holder,   AC_HOLDER_new);
  M_ASN1_New(ret->form,     AC_FORM_new);
  M_ASN1_New(ret->alg,      X509_ALGOR_new);
  M_ASN1_New(ret->serial,   ASN1_INTEGER_new);
  M_ASN1_New(ret->validity, AC_VAL_new);
  M_ASN1_New(ret->attrib,   sk_AC_ATTR_new_null);
  ret->id = NULL;
  M_ASN1_New(ret->exts,     sk_X509_EXTENSION_new_null);
/*   ret->exts=NULL; */
  return(ret);
  M_ASN1_New_Error(AC_F_AC_INFO_NEW);
}

void AC_INFO_free(AC_INFO *a)
{
  if (a==NULL) return;
  ASN1_INTEGER_free(a->version);
  AC_HOLDER_free(a->holder);
  AC_FORM_free(a->form);
  X509_ALGOR_free(a->alg);
  ASN1_INTEGER_free(a->serial);
  AC_VAL_free(a->validity);
  sk_AC_ATTR_pop_free(a->attrib, AC_ATTR_free);
  ASN1_BIT_STRING_free(a->id);
  sk_X509_EXTENSION_pop_free(a->exts, X509_EXTENSION_free);
  OPENSSL_free(a);
}

int i2d_AC(AC *a, unsigned char **pp) 
{
  M_ASN1_I2D_vars(a);

  M_ASN1_I2D_len(a->acinfo,    i2d_AC_INFO);
  M_ASN1_I2D_len(a->sig_alg,   i2d_X509_ALGOR);
  M_ASN1_I2D_len(a->signature, i2d_ASN1_BIT_STRING);

  M_ASN1_I2D_seq_total();

  M_ASN1_I2D_put(a->acinfo,    i2d_AC_INFO);
  M_ASN1_I2D_put(a->sig_alg,   i2d_X509_ALGOR);
  M_ASN1_I2D_put(a->signature, i2d_ASN1_BIT_STRING);

  M_ASN1_I2D_finish();
}

AC *d2i_AC(AC **a, VOMS_MAYBECONST unsigned char **pp, long length)
{
  M_ASN1_D2I_vars(a, AC *, AC_new);

  M_ASN1_D2I_Init();
  M_ASN1_D2I_start_sequence();
  M_ASN1_D2I_get(ret->acinfo,    d2i_AC_INFO);
  M_ASN1_D2I_get(ret->sig_alg,   d2i_X509_ALGOR);
  M_ASN1_D2I_get(ret->signature, d2i_ASN1_BIT_STRING);
  M_ASN1_D2I_Finish(a, AC_free, AC_F_D2I_AC);
}

AC *AC_new(void)
{
  AC *ret = NULL;
  ASN1_CTX c;

  M_ASN1_New_Malloc(ret, AC);
  M_ASN1_New(ret->acinfo,    AC_INFO_new);
  M_ASN1_New(ret->sig_alg,   X509_ALGOR_new);
  M_ASN1_New(ret->signature, M_ASN1_BIT_STRING_new);
  return(ret);
  M_ASN1_New_Error(AC_F_AC_New);
}

void AC_free(AC *a)
{
  if (a==NULL) return;

  AC_INFO_free(a->acinfo);
  X509_ALGOR_free(a->sig_alg);
  M_ASN1_BIT_STRING_free(a->signature);
  OPENSSL_free(a);
}

/* Wrapping ASN1_dup with AC_dup for use in C++. 
 * Calling ASN1_dup with casting generates inconsistent behavior across C++ compilers 
 */
AC *AC_dup(AC *ac)
{
  return (AC *)ASN1_dup((int (*)())i2d_AC, (char * (*) ())d2i_AC, (char *)ac);
}

EVP_PKEY *EVP_PKEY_dup(EVP_PKEY *pkey)
{
  return (EVP_PKEY *)ASN1_dup((int (*)())i2d_PrivateKey, (char * (*) ())d2i_AutoPrivateKey, (char *)pkey);
}

int AC_verify(X509_ALGOR *algor1, ASN1_BIT_STRING *signature,char *data, EVP_PKEY *pkey)
{
  return ASN1_verify((int (*)())i2d_AC_INFO, algor1, signature, data, pkey);
}
