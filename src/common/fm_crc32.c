/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_crc32.c
 * Creation Date:  March 19, 2007
 * Description:    Functions to manipulate bitfields
 *
 * Copyright (c) 2007 - 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Intel Corporation nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* CRC-32 least significant bit (LSB) first table.
 * The table has been generated using the following algorithm:
 *
 * fm_uint32 crc;
 * fm_uint32 poly = 0xedb88320U;
 * fm_int    i, j;
 *
 * for (i = 0 ; i < 256 ; i++)
 * {
 *     crc = i;
 *     for (j = 0; j < 8; j++)
 *     {
 *         crc = (crc >> 1) ^ (poly & (-(crc & 1)));
 *     }
 *     fmCrc32CTable[i] = crc;
 * }
 */
static const fm_uint32 fmCrc32Table[] =
{
    0x00000000U, 0x77073096U, 0xee0e612cU, 0x990951baU, 0x076dc419U,
    0x706af48fU, 0xe963a535U, 0x9e6495a3U, 0x0edb8832U, 0x79dcb8a4U,
    0xe0d5e91eU, 0x97d2d988U, 0x09b64c2bU, 0x7eb17cbdU, 0xe7b82d07U,
    0x90bf1d91U, 0x1db71064U, 0x6ab020f2U, 0xf3b97148U, 0x84be41deU,
    0x1adad47dU, 0x6ddde4ebU, 0xf4d4b551U, 0x83d385c7U, 0x136c9856U,
    0x646ba8c0U, 0xfd62f97aU, 0x8a65c9ecU, 0x14015c4fU, 0x63066cd9U,
    0xfa0f3d63U, 0x8d080df5U, 0x3b6e20c8U, 0x4c69105eU, 0xd56041e4U,
    0xa2677172U, 0x3c03e4d1U, 0x4b04d447U, 0xd20d85fdU, 0xa50ab56bU,
    0x35b5a8faU, 0x42b2986cU, 0xdbbbc9d6U, 0xacbcf940U, 0x32d86ce3U,
    0x45df5c75U, 0xdcd60dcfU, 0xabd13d59U, 0x26d930acU, 0x51de003aU,
    0xc8d75180U, 0xbfd06116U, 0x21b4f4b5U, 0x56b3c423U, 0xcfba9599U,
    0xb8bda50fU, 0x2802b89eU, 0x5f058808U, 0xc60cd9b2U, 0xb10be924U,
    0x2f6f7c87U, 0x58684c11U, 0xc1611dabU, 0xb6662d3dU, 0x76dc4190U,
    0x01db7106U, 0x98d220bcU, 0xefd5102aU, 0x71b18589U, 0x06b6b51fU,
    0x9fbfe4a5U, 0xe8b8d433U, 0x7807c9a2U, 0x0f00f934U, 0x9609a88eU,
    0xe10e9818U, 0x7f6a0dbbU, 0x086d3d2dU, 0x91646c97U, 0xe6635c01U,
    0x6b6b51f4U, 0x1c6c6162U, 0x856530d8U, 0xf262004eU, 0x6c0695edU,
    0x1b01a57bU, 0x8208f4c1U, 0xf50fc457U, 0x65b0d9c6U, 0x12b7e950U,
    0x8bbeb8eaU, 0xfcb9887cU, 0x62dd1ddfU, 0x15da2d49U, 0x8cd37cf3U,
    0xfbd44c65U, 0x4db26158U, 0x3ab551ceU, 0xa3bc0074U, 0xd4bb30e2U,
    0x4adfa541U, 0x3dd895d7U, 0xa4d1c46dU, 0xd3d6f4fbU, 0x4369e96aU,
    0x346ed9fcU, 0xad678846U, 0xda60b8d0U, 0x44042d73U, 0x33031de5U,
    0xaa0a4c5fU, 0xdd0d7cc9U, 0x5005713cU, 0x270241aaU, 0xbe0b1010U,
    0xc90c2086U, 0x5768b525U, 0x206f85b3U, 0xb966d409U, 0xce61e49fU,
    0x5edef90eU, 0x29d9c998U, 0xb0d09822U, 0xc7d7a8b4U, 0x59b33d17U,
    0x2eb40d81U, 0xb7bd5c3bU, 0xc0ba6cadU, 0xedb88320U, 0x9abfb3b6U,
    0x03b6e20cU, 0x74b1d29aU, 0xead54739U, 0x9dd277afU, 0x04db2615U,
    0x73dc1683U, 0xe3630b12U, 0x94643b84U, 0x0d6d6a3eU, 0x7a6a5aa8U,
    0xe40ecf0bU, 0x9309ff9dU, 0x0a00ae27U, 0x7d079eb1U, 0xf00f9344U,
    0x8708a3d2U, 0x1e01f268U, 0x6906c2feU, 0xf762575dU, 0x806567cbU,
    0x196c3671U, 0x6e6b06e7U, 0xfed41b76U, 0x89d32be0U, 0x10da7a5aU,
    0x67dd4accU, 0xf9b9df6fU, 0x8ebeeff9U, 0x17b7be43U, 0x60b08ed5U,
    0xd6d6a3e8U, 0xa1d1937eU, 0x38d8c2c4U, 0x4fdff252U, 0xd1bb67f1U,
    0xa6bc5767U, 0x3fb506ddU, 0x48b2364bU, 0xd80d2bdaU, 0xaf0a1b4cU,
    0x36034af6U, 0x41047a60U, 0xdf60efc3U, 0xa867df55U, 0x316e8eefU,
    0x4669be79U, 0xcb61b38cU, 0xbc66831aU, 0x256fd2a0U, 0x5268e236U,
    0xcc0c7795U, 0xbb0b4703U, 0x220216b9U, 0x5505262fU, 0xc5ba3bbeU,
    0xb2bd0b28U, 0x2bb45a92U, 0x5cb36a04U, 0xc2d7ffa7U, 0xb5d0cf31U,
    0x2cd99e8bU, 0x5bdeae1dU, 0x9b64c2b0U, 0xec63f226U, 0x756aa39cU,
    0x026d930aU, 0x9c0906a9U, 0xeb0e363fU, 0x72076785U, 0x05005713U,
    0x95bf4a82U, 0xe2b87a14U, 0x7bb12baeU, 0x0cb61b38U, 0x92d28e9bU,
    0xe5d5be0dU, 0x7cdcefb7U, 0x0bdbdf21U, 0x86d3d2d4U, 0xf1d4e242U,
    0x68ddb3f8U, 0x1fda836eU, 0x81be16cdU, 0xf6b9265bU, 0x6fb077e1U,
    0x18b74777U, 0x88085ae6U, 0xff0f6a70U, 0x66063bcaU, 0x11010b5cU,
    0x8f659effU, 0xf862ae69U, 0x616bffd3U, 0x166ccf45U, 0xa00ae278U,
    0xd70dd2eeU, 0x4e048354U, 0x3903b3c2U, 0xa7672661U, 0xd06016f7U,
    0x4969474dU, 0x3e6e77dbU, 0xaed16a4aU, 0xd9d65adcU, 0x40df0b66U,
    0x37d83bf0U, 0xa9bcae53U, 0xdebb9ec5U, 0x47b2cf7fU, 0x30b5ffe9U,
    0xbdbdf21cU, 0xcabac28aU, 0x53b39330U, 0x24b4a3a6U, 0xbad03605U,
    0xcdd70693U, 0x54de5729U, 0x23d967bfU, 0xb3667a2eU, 0xc4614ab8U,
    0x5d681b02U, 0x2a6f2b94U, 0xb40bbe37U, 0xc30c8ea1U, 0x5a05df1bU,
    0x2d02ef8dU
};

/* CRC-32C least significant bit (LSB) first table.
 * The table has been generated using the following algorithm:
 *
 * fm_uint32 crc;
 * fm_uint32 poly = 0x82f63b78U;
 * fm_int    i, j;
 *
 * for (i = 0 ; i < 256 ; i++)
 * {
 *     crc = i;
 *     for (j = 0; j < 8; j++)
 *     {
 *         crc = (crc >> 1) ^ (poly & (-(crc & 1)));
 *     }
 *     fmCrc32CTable[i] = crc;
 * }
 */
static const fm_uint32 fmCrc32CTable[] =
{
    0x00000000U, 0xf26b8303U, 0xe13b70f7U, 0x1350f3f4U, 0xc79a971fU,
    0x35f1141cU, 0x26a1e7e8U, 0xd4ca64ebU, 0x8ad958cfU, 0x78b2dbccU,
    0x6be22838U, 0x9989ab3bU, 0x4d43cfd0U, 0xbf284cd3U, 0xac78bf27U,
    0x5e133c24U, 0x105ec76fU, 0xe235446cU, 0xf165b798U, 0x030e349bU,
    0xd7c45070U, 0x25afd373U, 0x36ff2087U, 0xc494a384U, 0x9a879fa0U,
    0x68ec1ca3U, 0x7bbcef57U, 0x89d76c54U, 0x5d1d08bfU, 0xaf768bbcU,
    0xbc267848U, 0x4e4dfb4bU, 0x20bd8edeU, 0xd2d60dddU, 0xc186fe29U,
    0x33ed7d2aU, 0xe72719c1U, 0x154c9ac2U, 0x061c6936U, 0xf477ea35U,
    0xaa64d611U, 0x580f5512U, 0x4b5fa6e6U, 0xb93425e5U, 0x6dfe410eU,
    0x9f95c20dU, 0x8cc531f9U, 0x7eaeb2faU, 0x30e349b1U, 0xc288cab2U,
    0xd1d83946U, 0x23b3ba45U, 0xf779deaeU, 0x05125dadU, 0x1642ae59U,
    0xe4292d5aU, 0xba3a117eU, 0x4851927dU, 0x5b016189U, 0xa96ae28aU,
    0x7da08661U, 0x8fcb0562U, 0x9c9bf696U, 0x6ef07595U, 0x417b1dbcU,
    0xb3109ebfU, 0xa0406d4bU, 0x522bee48U, 0x86e18aa3U, 0x748a09a0U,
    0x67dafa54U, 0x95b17957U, 0xcba24573U, 0x39c9c670U, 0x2a993584U,
    0xd8f2b687U, 0x0c38d26cU, 0xfe53516fU, 0xed03a29bU, 0x1f682198U,
    0x5125dad3U, 0xa34e59d0U, 0xb01eaa24U, 0x42752927U, 0x96bf4dccU,
    0x64d4cecfU, 0x77843d3bU, 0x85efbe38U, 0xdbfc821cU, 0x2997011fU,
    0x3ac7f2ebU, 0xc8ac71e8U, 0x1c661503U, 0xee0d9600U, 0xfd5d65f4U,
    0x0f36e6f7U, 0x61c69362U, 0x93ad1061U, 0x80fde395U, 0x72966096U,
    0xa65c047dU, 0x5437877eU, 0x4767748aU, 0xb50cf789U, 0xeb1fcbadU,
    0x197448aeU, 0x0a24bb5aU, 0xf84f3859U, 0x2c855cb2U, 0xdeeedfb1U,
    0xcdbe2c45U, 0x3fd5af46U, 0x7198540dU, 0x83f3d70eU, 0x90a324faU,
    0x62c8a7f9U, 0xb602c312U, 0x44694011U, 0x5739b3e5U, 0xa55230e6U,
    0xfb410cc2U, 0x092a8fc1U, 0x1a7a7c35U, 0xe811ff36U, 0x3cdb9bddU,
    0xceb018deU, 0xdde0eb2aU, 0x2f8b6829U, 0x82f63b78U, 0x709db87bU,
    0x63cd4b8fU, 0x91a6c88cU, 0x456cac67U, 0xb7072f64U, 0xa457dc90U,
    0x563c5f93U, 0x082f63b7U, 0xfa44e0b4U, 0xe9141340U, 0x1b7f9043U,
    0xcfb5f4a8U, 0x3dde77abU, 0x2e8e845fU, 0xdce5075cU, 0x92a8fc17U,
    0x60c37f14U, 0x73938ce0U, 0x81f80fe3U, 0x55326b08U, 0xa759e80bU,
    0xb4091bffU, 0x466298fcU, 0x1871a4d8U, 0xea1a27dbU, 0xf94ad42fU,
    0x0b21572cU, 0xdfeb33c7U, 0x2d80b0c4U, 0x3ed04330U, 0xccbbc033U,
    0xa24bb5a6U, 0x502036a5U, 0x4370c551U, 0xb11b4652U, 0x65d122b9U,
    0x97baa1baU, 0x84ea524eU, 0x7681d14dU, 0x2892ed69U, 0xdaf96e6aU,
    0xc9a99d9eU, 0x3bc21e9dU, 0xef087a76U, 0x1d63f975U, 0x0e330a81U,
    0xfc588982U, 0xb21572c9U, 0x407ef1caU, 0x532e023eU, 0xa145813dU,
    0x758fe5d6U, 0x87e466d5U, 0x94b49521U, 0x66df1622U, 0x38cc2a06U,
    0xcaa7a905U, 0xd9f75af1U, 0x2b9cd9f2U, 0xff56bd19U, 0x0d3d3e1aU,
    0x1e6dcdeeU, 0xec064eedU, 0xc38d26c4U, 0x31e6a5c7U, 0x22b65633U,
    0xd0ddd530U, 0x0417b1dbU, 0xf67c32d8U, 0xe52cc12cU, 0x1747422fU,
    0x49547e0bU, 0xbb3ffd08U, 0xa86f0efcU, 0x5a048dffU, 0x8ecee914U,
    0x7ca56a17U, 0x6ff599e3U, 0x9d9e1ae0U, 0xd3d3e1abU, 0x21b862a8U,
    0x32e8915cU, 0xc083125fU, 0x144976b4U, 0xe622f5b7U, 0xf5720643U,
    0x07198540U, 0x590ab964U, 0xab613a67U, 0xb831c993U, 0x4a5a4a90U,
    0x9e902e7bU, 0x6cfbad78U, 0x7fab5e8cU, 0x8dc0dd8fU, 0xe330a81aU,
    0x115b2b19U, 0x020bd8edU, 0xf0605beeU, 0x24aa3f05U, 0xd6c1bc06U,
    0xc5914ff2U, 0x37faccf1U, 0x69e9f0d5U, 0x9b8273d6U, 0x88d28022U,
    0x7ab90321U, 0xae7367caU, 0x5c18e4c9U, 0x4f48173dU, 0xbd23943eU,
    0xf36e6f75U, 0x0105ec76U, 0x12551f82U, 0xe03e9c81U, 0x34f4f86aU,
    0xc69f7b69U, 0xd5cf889dU, 0x27a40b9eU, 0x79b737baU, 0x8bdcb4b9U,
    0x988c474dU, 0x6ae7c44eU, 0xbe2da0a5U, 0x4c4623a6U, 0x5f16d052U,
    0xad7d5351U
};

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmCrc32
 * \ingroup intUtil
 *
 * \desc            Returns the Ethernet 32-bit cyclic redundancy check
 *                  (CRC-32) of the given buffer.
 *
 * \param[in]       buf points to the buffer for which to compute the CRC-32.
 *
 * \param[in]       len is the number of bytes to include in the CRC-32.
 *
 * \return          The CRC-32 value.
 *
 *****************************************************************************/
fm_uint32 fmCrc32(fm_byte *buf, fm_int len)
{
    fm_int        i;
    fm_uint32     crc;

    crc = 0xffffffffU;

    for (i = 0 ; i < len ; i++)
    {
        crc = fmCrc32Table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }

    crc = ~crc;

    return crc;

}   /* end fmCrc32 */




/*****************************************************************************/
/** fmCrc32Math
 * \ingroup intUtil
 *
 * \desc            Returns the Ethernet 32-bit cyclic redundancy check
 *                  (CRC-32) of the given buffer with the CRC initialized
 *                  to zero.
 *
 * \param[in]       buf points to the buffer for which to compute the CRC-32.
 *
 * \param[in]       len is the number of bytes to include in the CRC-32.
 *
 * \return          The CRC-32 value.
 *
 *****************************************************************************/
fm_uint32 fmCrc32Math(fm_byte *buf, fm_int len)
{
    fm_int        i;
    fm_uint32     crc;

    crc = 0;

    for (i = 0 ; i < len ; i++)
    {
        crc = fmCrc32Table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }

    return crc;

}   /* end fmCrc32Math */




/*****************************************************************************/
/** fmCrc32C
 * \ingroup intUtil
 *
 * \desc            Returns the Castagnoli 32-bit cyclic redundancy check
 *                  (CRC-32C) of the given buffer.
 *
 * \param[in]       buf points to the buffer for which to compute the CRC-32C.
 *
 * \param[in]       len is the number of bytes to include in the CRC-32C.
 *
 * \return          The CRC-32C value.
 *
 *****************************************************************************/
fm_uint32 fmCrc32C(fm_byte *buf, fm_int len)
{
    fm_int    i;
    fm_uint32 crc;

    crc = 0xffffffffU;

    for (i = 0 ; i < len ; i++)
    {
        crc = fmCrc32CTable[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }

    crc = ~crc;

    return crc;

}   /* end fmCrc32C */

