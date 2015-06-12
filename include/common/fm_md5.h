/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_md5.h
 * Creation Date:  July 21, 2009
 * Description:    MD5 hash functions
 *
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * Modified by Intel Corporation to use Intel standard types and
 * formatting.
 *****************************************************************************/

#ifndef __FM_FM_MD5_H
#define __FM_FM_MD5_H

typedef struct _fm_MD5Context 
{
	fm_uint32 buf[4];
	fm_uint32 bits[2];
	fm_byte   in[64];
	
} fm_MD5Context;

void fmMD5Init(fm_MD5Context *context);
void fmMD5Update(fm_MD5Context *context, fm_byte const *buf, fm_uint len); 
void fmMD5Final(fm_MD5Context *context, fm_byte digest[16]);

#endif  /* __FM_FM_MD5_H */
