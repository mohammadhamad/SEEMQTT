//#include <iostream>
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/md5.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/entropy_poll.h"

#include "KeynoteESP.h"

static const char hextab[] = {
     '0', '1', '2', '3', '4', '5', '6', '7',
     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};
/*
* Actual conversion to hex.
*/
void bin2hex(unsigned char *data, unsigned char *buffer, int len)
{
  int off = 0;
  while(len > 0)
  {
    buffer[off++] = hextab[*data >> 4];
    buffer[off++] = hextab[*data & 0xF];
    data++;
    len--;

  }
}


static int kn_myrand(void *rng_state, unsigned char *output, size_t len)
{
  size_t olen;
  return mbedtls_hardware_poll(rng_state, output, len, &olen);
}

/*
* Create a credential.
* Input: authorizer public key, Licensee public key, public key lenght, conditions,condition string lengh  and authorizer private key
* both public key Should follow the keynote ecoding
*/

void  kn_GenCredential (const unsigned char * cre, int cr_len , const unsigned char * auth_pr, int prk_len,  unsigned char ** sig_cre,  int * relen)
{

  unsigned char signature[128]={0};
  //int tmprlen = 888;
  char * result =  kn_rsa_sign_sha256(cre,cr_len,  auth_pr, prk_len, signature);
  if (result == NULL )
  {
    printf("Error on kn_rsa_sign_sha256\n");
    return ;
  }
  char * kn_sign = kn_encode_signture("",result) ; //  the lengh of kn_sig = KN_SIGN_SZ
  if (kn_sign == NULL )
  {
    printf("Error on kn_sign\n");
    return;
  }
  //*relen = KN_SIGN_SZ + cr_len ;
  char * sigpart = "Signature: ";
  *relen =  cr_len + strlen(kn_sign) + strlen(sigpart) ;
  unsigned char * outconme = (unsigned char *) malloc(*relen) ;
  if (outconme == NULL)
  {
    printf("Error on malloc\n");
    return;
  }
  memset(outconme, '0', *relen);
  //printf("*******relen = %d***\n", *relen );
  sprintf(outconme,"%s%s%s",cre,sigpart, kn_sign );
  //printf("%s\n",outconme );
  *sig_cre = outconme ;
}
/*
* Encode a binary string with hex encoding. Return 0 on success.
*/
int kn_encode_hex(unsigned char *buf, char **dest, int len)
{

  if (dest == (char **) NULL)
  {
    //.println(" Error: dest  is NULL \n");
    return -1;
  }

  *dest = (char *) calloc(2 * len + 1, sizeof(char));
  if (*dest == (char *) NULL)
  {
    return -1;
  }

  bin2hex((unsigned char *)buf, (unsigned char *) *dest, len);

  return 0;
}


/*
* Encode the signature
* the length of the result is KN_SIGN_SZ
*/
char * kn_encode_signture( char *algname ,  char *key)
{
  int  index = 0 ;
  char * result = (char *)malloc (KN_SIGN_SZ);
  memset(result, '\0',KN_SIGN_SZ);

  int length  = 50 ;
  int start = 12;
  int i, k;

  for (int j = 0 ; j< start; j++)
  result[index +j]= ' ';

  index += start;
  // add "\"";
  result[index++] = '\"';
  int alglen= strlen(algname);

  strncpy(result+index,algname, alglen);
  index += alglen;

  for (i = 0, k = strlen(algname) + 2; i < strlen(key); i++, k++)
  {
    if (k == length)
    {
      if (i == strlen(key))
      {
        char * t = "\"\n";
        strncpy (result+index,t, strlen(t));
        index +=strlen(t);
        return  result;
      }
      char * tt = "\\\n";
      strncpy(result+index,tt, strlen(tt));
      index +=strlen(tt);
      for (int j = 0 ; j< start; j++)
      result[index +j]= ' ';

      index += start;
      i--;
      k = 0;
    }
    else
    {
      result[index++] = key[i] ;
    }

  }

  char * l = "\"\n" ;
  strncpy (result+index , l , strlen(l));
  return result;
}

/*
* sign the credential
*/
char *  kn_rsa_sign_md5(const unsigned char * input, int len, const unsigned char * iot_pr_key, int keylen,   unsigned char * sign)
{
  mbedtls_pk_context  pr;
  mbedtls_pk_init(&pr);
  mbedtls_md5_context shscontext;
  int error_code_pr ;
  int hashlen =  16 ;
  unsigned char hashval[hashlen];

  #ifdef DDBG
  println("Input Text");
  Print( (uint8_t *)input, 0 ,P_SIZE);
  #endif

  error_code_pr =  mbedtls_pk_parse_key(&pr, iot_pr_key, keylen,NULL,NULL);
  if( error_code_pr != 0)
  {
    #ifdef DDBG
    Serial.println( "READING PRIVATE KEY NOT SUCCESSFULL");
    #endif
    mbedtls_pk_free(&pr);
    return NULL ;
  }

  #ifdef DDBG
  printf(" Length of the cre = %d \n", len);
  printf("************************\n");
  for (int i = 0 ; i< len ; i++)
    printf("%c", input[i] );
  printf("************************\n");
   #endif
  /* compute the MD5 hash for the input*/
  mbedtls_md5_init(&shscontext);
  mbedtls_md5_starts(&shscontext);
  mbedtls_md5_update(&shscontext,input, len-1);
  const unsigned char sigalg[]  = "sig-rsa-md5-hex:";
  int sigalglen = strlen((char *)sigalg);
  mbedtls_md5_update(&shscontext, sigalg, sigalglen);
  mbedtls_md5_finish(&shscontext, hashval);

  #ifdef DDBG
  printf(" MD5 lngth hash= %d \n", hashlen);
  printf(" sigalglen= %d (must be 16)\n", sigalglen);
  for (int i = 0 ; i< hashlen ; i++)
    printf("%x", hashval[i] );
   #endif
  int sign_ret;
  size_t olen= 0;
  sign_ret = mbedtls_pk_sign(&pr, MBEDTLS_MD_MD5, hashval, hashlen, sign, &olen, kn_myrand, NULL);
  if (sign_ret != 0)
  {
    mbedtls_pk_free(&pr);
    return NULL;
  }

    #ifdef DDBG
  printf(" figlen = %d\n", olen);
  for (int i = 0 ; i< olen ; i++)
    printf("%x", sign[i] );
    #endif
  char * finalbuf = (char *)NULL;
  int  i = kn_encode_hex(sign, (char **) &finalbuf, olen);

  if (i != 0)
    return NULL;

  /* Concatenate algorithm name and signature value */
  char * rebuf = (char *) calloc(sigalglen + strlen(finalbuf) + 1, sizeof(char));
  sprintf(rebuf, "%s%s", sigalg, finalbuf);

  return rebuf;
  mbedtls_pk_free(&pr);
}



/*
* sign the credential using sha256
*/
char *  kn_rsa_sign_sha256(const unsigned char * input, int len, const unsigned char * iot_pr_key, int keylen,   unsigned char * sign)
{

  mbedtls_pk_context  pr;
  mbedtls_pk_init(&pr);
  mbedtls_sha256_context shscontext;
  int error_code_pr ;
  int hashlen =  32 ;
  unsigned char hashval[hashlen];

  #ifdef DDBG
  println("Input Text");
  Print( (uint8_t *)input, 0 ,P_SIZE);
  #endif

  error_code_pr =  mbedtls_pk_parse_key(&pr, iot_pr_key, keylen,NULL,NULL);
  if( error_code_pr != 0)
  {
    #ifdef DDBG
    Serial.println( "READING PRIVATE KEY NOT SUCCESSFULL");
    #endif
    mbedtls_pk_free(&pr);
    return NULL ;
  }

  #ifdef DDBG
  printf(" Length of the cre = %d \n", len);
  printf("************************\n");
  for (int i = 0 ; i< len ; i++)
    printf("%c", input[i] );
  printf("************************\n");
   #endif
  /* compute the MD5 hash for the input*/
  mbedtls_sha256_init(&shscontext);
  mbedtls_sha256_starts(&shscontext,0);
  mbedtls_sha256_update(&shscontext,input, len-1);
  const unsigned char sigalg[]  = "sig-rsa-sha256-hex:";
  int sigalglen = strlen((char *)sigalg);
  mbedtls_sha256_update(&shscontext, sigalg, sigalglen);
  mbedtls_sha256_finish(&shscontext, hashval);

  #ifdef DDBG
  printf(" sha256 lngth hash= %d \n", hashlen);
  printf(" sigalglen= %d (must be 18)\n", sigalglen);
  for (int i = 0 ; i< hashlen ; i++)
    printf("%x", hashval[i] );
  #endif
  int sign_ret;
  size_t olen= 0;
  sign_ret = mbedtls_pk_sign(&pr, MBEDTLS_MD_SHA256, hashval, hashlen, sign, &olen, kn_myrand, NULL);
  if (sign_ret != 0)
  {
    mbedtls_pk_free(&pr);
    return NULL;
  }

    #ifdef DDBG
  printf(" figlen = %d\n", olen);
  for (int i = 0 ; i< olen ; i++)
    printf("%x", sign[i] );
    #endif
  char * finalbuf = (char *)NULL;
  int  i = kn_encode_hex(sign, (char **) &finalbuf, olen);

  if (i != 0)
    return NULL;

  /* Concatenate algorithm name and signature value */
  char * rebuf = (char *) calloc(sigalglen + strlen(finalbuf) + 1, sizeof(char));
  sprintf(rebuf, "%s%s", sigalg, finalbuf);

  return rebuf;
  mbedtls_pk_free(&pr);
}
