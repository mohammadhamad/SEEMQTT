/*
 * The author of this code is Angelos D. Keromytis (angelos@dsl.cis.upenn.edu)
 *
 * This code was written by Angelos D. Keromytis in Philadelphia, PA, USA,
 * in April-May 1998
 *
 * Copyright (C) 1998, 1999 by Angelos D. Keromytis.
 *	
 * Permission to use, copy, and modify this software without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software. 
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, THE AUTHORS MAKES NO
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */

#if HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#include "header.h"
#include "keynote.h"
#include "assertion.h"
#include "signature.h"

/*
 * Get some sort of key-hash for hash table indexing purposes.
 */
static int
keynote_keyhash(void *key, int alg)
{
    struct keynote_binary *bn;
    unsigned int res = 0, i;
#ifdef CRYPTO
    DSA *dsa;
    RSA *rsa;
#endif /* CRYPTO */

    if (key == (void *) NULL)
      return 0;

    switch (alg)
    {
#ifdef CRYPTO
    BIGNUM *dsap, *dsaq, *dsag, *dsapub_key;
    BIGNUM *rsan, *rsae;
	case KEYNOTE_ALGORITHM_DSA:
	    dsa = (DSA *) key;

        DSA_get0_pqg(dsa, &dsap, &dsaq, &dsag);
        DSA_get0_key(dsa, &dsapub_key, NULL);
	    
        res += BN_mod_word(dsap, HASHTABLESIZE);
	    res += BN_mod_word(dsaq, HASHTABLESIZE);
	    res += BN_mod_word(dsag, HASHTABLESIZE);
	    res += BN_mod_word(dsapub_key, HASHTABLESIZE);

//	    res += BN_mod_word(dsa->p, HASHTABLESIZE);
//	    res += BN_mod_word(dsa->q, HASHTABLESIZE);
//	    res += BN_mod_word(dsa->g, HASHTABLESIZE);
//	    res += BN_mod_word(dsa->pub_key, HASHTABLESIZE);
	    return res % HASHTABLESIZE;

        case KEYNOTE_ALGORITHM_RSA:
	        rsa = (RSA *) key;
            RSA_get0_key(rsa, &rsan, &rsae, NULL);

            res += BN_mod_word(rsan, HASHTABLESIZE);
            res += BN_mod_word(rsae, HASHTABLESIZE);

//            res += BN_mod_word(rsa->n, HASHTABLESIZE);
//            res += BN_mod_word(rsa->e, HASHTABLESIZE);
	    return res % HASHTABLESIZE;

	case KEYNOTE_ALGORITHM_X509: /* RSA-specific */
	    rsa = (RSA *) key;
            RSA_get0_key(rsa, &rsan, &rsae, NULL);

            res += BN_mod_word(rsan, HASHTABLESIZE);
            res += BN_mod_word(rsae, HASHTABLESIZE);

//            res += BN_mod_word(rsa->n, HASHTABLESIZE);
//            res += BN_mod_word(rsa->e, HASHTABLESIZE);
	    return res % HASHTABLESIZE;
#endif /* CRYPTO */

	case KEYNOTE_ALGORITHM_BINARY:
	    bn = (struct keynote_binary *) key;
	    for (i = 0; i < bn->bn_len; i++)
	      res = (res + ((unsigned char) bn->bn_key[i])) % HASHTABLESIZE;

	    return res;

	case KEYNOTE_ALGORITHM_NONE:
	    return keynote_stringhash(key, HASHTABLESIZE);

	default:
	    return 0;
    }
}

/*
 * Return RESULT_TRUE if key appears in the action authorizers.
 */
int
keynote_in_action_authorizers(void *key, int algorithm)
{
    struct keylist *kl, *kl2;
    void *s;
    int alg;

    if (algorithm == KEYNOTE_ALGORITHM_UNSPEC)
    {
	kl2 = keynote_keylist_find(keynote_current_assertion->as_keylist, key);
	if (kl2 == (struct keylist *) NULL)
	  return RESULT_FALSE;   /* Shouldn't ever happen */

	s = kl2->key_key;
	alg = kl2->key_alg;
    }
    else
    {
	s = key;
	alg = algorithm;
    }

    for (kl = keynote_current_session->ks_action_authorizers;
	 kl != (struct keylist *) NULL;
	 kl = kl->key_next)
      if ((kl->key_alg == alg) ||
	  ((kl->key_alg == KEYNOTE_ALGORITHM_RSA) &&
	   (alg == KEYNOTE_ALGORITHM_X509)) ||
	  ((kl->key_alg == KEYNOTE_ALGORITHM_X509) &&
	   (alg == KEYNOTE_ALGORITHM_RSA)))
	if (kn_keycompare(kl->key_key, s, alg) == RESULT_TRUE)
	  return RESULT_TRUE;

    return RESULT_FALSE;
}

/*
 * Add a key to the keylist. Return RESULT_TRUE on success, -1 (and set
 * keynote_errno) otherwise. We are not supposed to make a copy of the 
 * argument.
 */
int
keynote_keylist_add(struct keylist **keylist, char *key)
{
    struct keynote_deckey dc;
    struct keylist *kl;
    
    if (keylist == (struct keylist **) NULL)
    {
	keynote_errno = ERROR_MEMORY;
	return -1;
    }
    
    kl = (struct keylist *) calloc(1, sizeof(struct keylist));
    if (kl == (struct keylist *) NULL)
    {
	keynote_errno = ERROR_MEMORY;
	return -1;
    }

    if (kn_decode_key(&dc, key, KEYNOTE_PUBLIC_KEY) != 0)
    {
	free(kl);
	return -1;
    }

    kl->key_key = dc.dec_key;
    kl->key_alg = dc.dec_algorithm;
    kl->key_stringkey = key;
    kl->key_next = *keylist;
    *keylist = kl;
    return RESULT_TRUE;
}

/*
 * Remove an action authorizer.
 */
int
kn_remove_authorizer(int sessid, char *key)
{
    struct keynote_session *ks;
    struct keylist *kl, *kl2;

    keynote_errno = 0;
    if ((keynote_current_session == (struct keynote_session *) NULL) ||
	(keynote_current_session->ks_id != sessid))
    {
	keynote_current_session = keynote_find_session(sessid);
	if (keynote_current_session == (struct keynote_session *) NULL)
	{
	    keynote_errno = ERROR_NOTFOUND;
	    return -1;
	}
    }

    ks = keynote_current_session;

    /* If no action authorizers present */
    if ((kl = ks->ks_action_authorizers) == (struct keylist *) NULL)
    {
	keynote_errno = ERROR_NOTFOUND;
	return -1;
    }

    /* First in list */
    if (!strcmp(kl->key_stringkey, key))
    {
	ks->ks_action_authorizers = kl->key_next;
	kl->key_next = (struct keylist *) NULL;
	keynote_keylist_free(kl);
	return 0;
    }

    for (; kl->key_next != (struct keylist *) NULL; kl = kl->key_next)
      if (!strcmp(kl->key_stringkey, key))
      {
	  kl2 = kl->key_next;
	  kl->key_next = kl2->key_next;
	  kl2->key_next = (struct keylist *) NULL;
	  keynote_keylist_free(kl2);
	  return 0;
      }
    
    keynote_errno = ERROR_NOTFOUND;
    return -1;
}

/*
 * Add an action authorizer.
 */
int
kn_add_authorizer(int sessid, char *key)
{
    char *stringkey;

    keynote_errno = 0;
    if ((keynote_current_session == (struct keynote_session *) NULL) ||
	(keynote_current_session->ks_id != sessid))
    {
	keynote_current_session = keynote_find_session(sessid);
	if (keynote_current_session == (struct keynote_session *) NULL)
	{
	    keynote_errno = ERROR_NOTFOUND;
	    return -1;
	}
    }

    stringkey = strdup((char *) key);
    if (stringkey == (char *) NULL)
    {
	keynote_errno = ERROR_MEMORY;
	return -1;
    }

    if (keynote_keylist_add(&(keynote_current_session->ks_action_authorizers),
			    stringkey) == -1)
    {
	free(stringkey);
	return -1;
    }

    return 0;
}

/*
 * Find a keylist entry based on the key_stringkey entry.
 */
struct keylist *
keynote_keylist_find(struct keylist *kl, char *s)
{
    for (; kl != (struct keylist *) NULL; kl = kl->key_next)
      if (!strcmp(kl->key_stringkey, s))
	return kl;

    return kl;
}

/*
 * Free keylist list.
 */
void
keynote_keylist_free(struct keylist *kl)
{
    struct keylist *kl2;

    while (kl != (struct keylist *) NULL)
    {
	kl2 = kl->key_next;
	free(kl->key_stringkey);
	keynote_free_key(kl->key_key, kl->key_alg);
	free(kl);
	kl = kl2;
    }
}

/*
 * Free a key.
 */
void
kn_free_key(struct keynote_deckey *dc)
{
    if (dc)
      keynote_free_key(dc->dec_key, dc->dec_algorithm);
}

/*
 * Find the num-th assertion given the authorizer. Return NULL if not found.
 */
struct assertion *
keynote_find_assertion(void *authorizer, int num, int algorithm)
{
    struct assertion *as;
    unsigned int h;

    if (authorizer == (char *) NULL)
      return (struct assertion *) NULL;

    h = keynote_keyhash(authorizer, algorithm);
    for (as = keynote_current_session->ks_assertion_table[h];
	 as != (struct assertion *) NULL;
	 as = as->as_next)
      if ((as->as_authorizer != (void *) NULL) &&
	  ((as->as_signeralgorithm == algorithm) ||
	   ((as->as_signeralgorithm == KEYNOTE_ALGORITHM_RSA) &&
	    (algorithm == KEYNOTE_ALGORITHM_X509)) ||
	   ((as->as_signeralgorithm == KEYNOTE_ALGORITHM_X509) &&
	    (algorithm == KEYNOTE_ALGORITHM_RSA))))
	if (kn_keycompare(authorizer, as->as_authorizer, algorithm) ==
	    RESULT_TRUE)
	  if (num-- == 0)
	    return as;

    return (struct assertion *) NULL;
}

/*
 * Add an assertion to the hash table. Return RESULT_TRUE on success,
 * ERROR_MEMORY for memory failure, ERROR_SYNTAX if some problem with
 * the assertion is detected.
 */
int
keynote_add_htable(struct assertion *as, int which)
{
    char *hashname;
    u_int i;

    if (as == (struct assertion *) NULL)
    {
	keynote_errno = ERROR_MEMORY;
	return -1;
    }

    if (!which)
      hashname = as->as_authorizer_string_s;
    else
      hashname = as->as_authorizer;

    if (hashname == (char *) NULL)
    {
	keynote_errno = ERROR_SYNTAX;
	return -1;
    }

    i = keynote_keyhash(hashname, as->as_signeralgorithm);
    as->as_next = keynote_current_session->ks_assertion_table[i];
    keynote_current_session->ks_assertion_table[i] = as;
    return RESULT_TRUE;
}

/*
 * Parse and store an assertion in the internal hash table. 
 * Return the result of the evaluation, if doing early evaluation.
 * If an error was encountered, set keynote_errno.
 */
int
kn_add_assertion(int sessid, char *asrt, int len, int assertion_flags)
{
    struct assertion *as;

    keynote_errno = 0;
    if ((keynote_current_session == (struct keynote_session *) NULL) ||
	(keynote_current_session->ks_id != sessid))
    {
	keynote_current_session = keynote_find_session(sessid);
	if (keynote_current_session == (struct keynote_session *) NULL)
	{
	    keynote_errno = ERROR_NOTFOUND;
	    return -1;
	}
    }

    as = keynote_parse_assertion(asrt, len, assertion_flags);
    if ((as == (struct assertion *) NULL) || (keynote_errno != 0))
    {
	if (keynote_errno == 0)
	  keynote_errno = ERROR_SYNTAX;

	return -1;
    }

    as->as_id = keynote_current_session->ks_assertioncounter++;

    /* Check for wrap around...there has to be a better solution to this */
    if (keynote_current_session->ks_assertioncounter < 0)
    {
	keynote_free_assertion(as);
	keynote_errno = ERROR_SYNTAX;
	return -1;
    }
    
    if (keynote_add_htable(as, 0) != RESULT_TRUE)
    {
	keynote_free_assertion(as);
	return -1;
    }

    as->as_internalflags |= ASSERT_IFLAG_NEEDPROC;
    return as->as_id;
}

/*
 * Remove an assertion from the hash table.
 */
static int
keynote_remove_assertion(int sessid, int assertid, int deleteflag)
{
    struct assertion *ht, *ht2;
    int i;

    if ((keynote_current_session == (struct keynote_session *) NULL) ||
	(keynote_current_session->ks_id != sessid))
    {
	keynote_current_session = keynote_find_session(sessid);
	if (keynote_current_session == (struct keynote_session *) NULL)
	{
	    keynote_errno = ERROR_NOTFOUND;
	    return -1;
	}
    }

    for (i = 0; i < HASHTABLESIZE; i++)
    {
	ht = keynote_current_session->ks_assertion_table[i];
	if (ht == (struct assertion *) NULL)
	  continue;

	/* If first entry in bucket */
	if (ht->as_id == assertid)
	{
	    keynote_current_session->ks_assertion_table[i] = ht->as_next;
	    if (deleteflag)
	      keynote_free_assertion(ht);
	    return 0;
	}

	for (; ht->as_next != (struct assertion *) NULL; ht = ht->as_next)
	  if (ht->as_next->as_id == assertid)  /* Got it */
	  {
	      ht2 = ht->as_next;
	      ht->as_next = ht2->as_next;
	      if (deleteflag)
		keynote_free_assertion(ht2);
	      return 0;
	  }
    }

    keynote_errno = ERROR_NOTFOUND;
    return -1;
}

/*
 * API wrapper for deleting assertions.
 */
int
kn_remove_assertion(int sessid, int assertid)
{
    keynote_errno = 0;
    return keynote_remove_assertion(sessid, assertid, 1);
}

/*
 * Internally-used wrapper for removing but not deleting assertions.
 */
int
keynote_sremove_assertion(int sessid, int assertid)
{
    return keynote_remove_assertion(sessid, assertid, 0);
}

/* 
 * Free an assertion structure.
 */
void
keynote_free_assertion(struct assertion *as)
{
    if (as == (struct assertion *) NULL)
      return;

    if (as->as_buf != (char *) NULL)
      free(as->as_buf);

    if (as->as_signature != (char *) NULL)
      free(as->as_signature);

    if (as->as_env != (struct environment *) NULL)
      keynote_env_cleanup(&(as->as_env), 1);

    if (as->as_keylist != (struct keylist *) NULL)
      keynote_keylist_free(as->as_keylist);

    if (as->as_authorizer != (void *) NULL)
      keynote_free_key(as->as_authorizer, as->as_signeralgorithm);

    free(as);
}

u_int 
keynote_stringhash(char *name, u_int size)
{
    unsigned int hash_val = 0;
    unsigned int i;

    if ((size == 0) || (size == 1))
      return 0;

    for (; *name; name++) 
    {
        hash_val = (hash_val << 2) + *name;
        if ((i = hash_val & 0x3fff) != 0)
	  hash_val = ((hash_val ^ (i >> 12)) & 0x3fff);
    }

    return hash_val % size;
}
