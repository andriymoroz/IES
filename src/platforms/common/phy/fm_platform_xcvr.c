/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_event.c
 * Creation Date:   November 1, 2012
 * Description:     Platform Event Notification functions.
 *
 * Copyright (c) 2006 - 2012, Intel Corporation
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
#include <platforms/common/event/fm_platform_event.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
#define PRINT_BIT_NAME(cond, name)              \
        if (cond)                               \
        {                                       \
            FM_LOG_PRINT(name);                 \
        }

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


 /*****************************************************************************/
/* PrintBytesName
 * \ingroup intPlatformXcvr
 *
 * \desc            Print name and any printable characters in buf.
 *
 * \param[in]       name is the name to print.
 *
 * \param[in]       addr is the address to print.
 *
 * \param[in]       buf is the buffer containing any printable characters.
 *
 * \param[in]       len is the length of the buffer.
 *
 * \return          NONE.
 *
 *****************************************************************************/
static void PrintBytesName(fm_text name, fm_int addr, fm_byte buf[], fm_int len)
{
    fm_int i;

    if (name)
    {
        FM_LOG_PRINT("%20s[%02x]: ", name, addr);
    }

    for (i = 0 ; i < len ; i++)
    {
        if (isprint(buf[i]))
        {
            FM_LOG_PRINT("%c", buf[i]);
        }
    }
    FM_LOG_PRINT("\n");

}   /* end PrintBytesName */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


 /*****************************************************************************/
/* fmPlatformXcvrTypeGetName
 * \ingroup intPlatformXcvr
 *
 * \desc            Returns the string representation of the transceiver type
 *
 * \param[in]       type is the transceiver type.
 *
 * \return          String representation of the value.
 *
 *****************************************************************************/
fm_text fmPlatformXcvrTypeGetName(fm_platformXcvrType type)
{
    switch (type)
    {
        case FM_PLATFORM_XCVR_TYPE_UNKNOWN:
            return "XCVR_TYPE_UNKNOWN";
        case FM_PLATFORM_XCVR_TYPE_OPTICAL:
            return "XCVR_TYPE_OPTICAL";
        case FM_PLATFORM_XCVR_TYPE_DAC:
            return "XCVR_TYPE_DAC";
        case FM_PLATFORM_XCVR_TYPE_NOT_PRESENT:
            return "XCVR_TYPE_NOT_PRESENT";
    }

    return "XCVR_TYPE_UNHANDLED";

} /* fmPlatformXcvrTypeGetName */




/*****************************************************************************/
/** fmPlatformXcvrEepromIsBaseCsumValid
 * \ingroup intPlatformXcvr
 *
 * \desc            Returns whether base eeprom checksum is valid.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          TRUE is base eeprom check is valid.
 *
 *****************************************************************************/
fm_uint fmPlatformXcvrEepromIsBaseCsumValid(fm_byte *eeprom)
{
    fm_byte   cs;
    fm_int    k;

    for (k = 0, cs = 0 ; k <= 62 ; k++)
    {
        cs += eeprom[k];
    }

    return (cs == eeprom[63]);

} /* fmPlatformXcvrEepromIsBaseCsumValid */




/*****************************************************************************/
/** fmPlatformXcvrEepromIsExtCsumValid
 * \ingroup intPlatformXcvr
 *
 * \desc            Returns whether ext eeprom checksum is valid.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          TRUE is ext eeprom check is valid.
 *
 *****************************************************************************/
fm_uint fmPlatformXcvrEepromIsExtCsumValid(fm_byte *eeprom)
{
    fm_byte   cs;
    fm_int    k;

    for (k = 64, cs = 0 ; k <= 94 ; k++)
    {
        cs += eeprom[k];
    }

    return (cs == eeprom[95]);

} /* fmPlatformXcvrEepromIsExtCsumValid */




/*****************************************************************************/
/** fmPlatformXcvrEepromGetLen
 * \ingroup intPlatformXcvr
 *
 * \desc            Returns the transceiver cable length in the transceiver eeprom.
 *                  This assumes the eeprom checksum has been verified.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          Transceiver cable length, if applicable.
 *
 *****************************************************************************/
fm_uint fmPlatformXcvrEepromGetLen(fm_byte *eeprom)
{
    return eeprom[18];

} /* fmPlatformXcvrEepromGetLen */





/*****************************************************************************/
/** fmPlatformXcvrEepromGetType
 * \ingroup intPlatformXcvr
 *
 * \desc            Returns the transceiver type given the transceiver eeprom.
 *                  This assumes the eeprom checksum has been verified.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          fm_platformXcvrType
 *
 *****************************************************************************/
fm_platformXcvrType fmPlatformXcvrEepromGetType(fm_byte *eeprom)
{
    if (eeprom[0] == 0xC || eeprom[0] == 0xD  || eeprom[0] == 0x11)
    {
        /* QSFP */
        /* Only bit3 at index 0 indicate non-optical */
        if ( (eeprom[3+0] & ~(1 << 3)) ||
             (eeprom[3+3] & (1 << 0)) )
        {
            return FM_PLATFORM_XCVR_TYPE_OPTICAL;
        }
        else
        {
            return FM_PLATFORM_XCVR_TYPE_DAC;
        }
    }
    if (eeprom[0] == 0x3)
    {
        /* 1000BASE-SX and 10GBASE-SR */
        if ( (eeprom[3+0] & (1 << 4)) ||
             (eeprom[3+3] & (1 << 0)) )
        {
            return FM_PLATFORM_XCVR_TYPE_OPTICAL;
        }
        else
        {
            return FM_PLATFORM_XCVR_TYPE_DAC;
        }
    }
    else
    {
        return FM_PLATFORM_XCVR_TYPE_UNKNOWN;
    }

} /* fmPlatformXcvrEepromGetType */



/*****************************************************************************/
/** fmPlatformXcvrIs1000BaseT
 * \ingroup intPlatformXcvr
 *
 * \desc            Returns TRUE if the module is 1000BaseT.
 *                  This assumes the eeprom checksum has been verified.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          TRUE if 1000BaseT module.
 *
 *****************************************************************************/
fm_bool fmPlatformXcvrIs1000BaseT(fm_byte *eeprom)
{
    return (eeprom[3+3] & (1 << 3)) ? TRUE : FALSE;

} /* fmPlatformXcvrIs1000BaseT */


/*****************************************************************************/
/** fmPlatformXcvrIs10G1G
 * \ingroup intPlatformXcvr
 *
 * \desc            Returns TRUE if the module support 10G and 1G speed.
 *                  This assumes the eeprom checksum has been verified.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          TRUE if dual rate module.
 *
 *****************************************************************************/
fm_bool fmPlatformXcvrIs10G1G(fm_byte *eeprom)
{
    /* 1000BASE-SX and 10GBASE-SR or 1000BASE-LX and 10GBASE-LR*/
    return ( ((eeprom[3+0] & (1 << 4)) && eeprom[3+3] & (1 << 0)) ||
             ((eeprom[3+0] & (1 << 5)) && eeprom[3+3] & (1 << 1)) );

} /* fmPlatformXcvrIs10G1G */



/*****************************************************************************/
/** fmPlatformXcvrEepromDumpSpecCompliance
 * \ingroup intPlatformXcvr
 *
 * \desc            Dumps transceiver spec compliance.
 *                  This assumes the eeprom checksum has been verified.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fmPlatformXcvrEepromDumpSpecCompliance(fm_byte *eeprom)
{

    FM_LOG_PRINT("%20s[%02x]:", "Spec Comp", 3);

    PRINT_BIT_NAME((eeprom[3+0] & (1 << 7)), " Extended");
    PRINT_BIT_NAME((eeprom[3+0] & (1 << 6)), " 10GBASE-LRM");
    PRINT_BIT_NAME((eeprom[3+0] & (1 << 5)), " 10GBASE-LR");
    PRINT_BIT_NAME((eeprom[3+0] & (1 << 4)), " 10GBASE-SR");
    PRINT_BIT_NAME((eeprom[3+0] & (1 << 3)), " 40GBASE-CR4");
    PRINT_BIT_NAME((eeprom[3+0] & (1 << 2)), " 40GBASE-SR4");
    PRINT_BIT_NAME((eeprom[3+0] & (1 << 1)), " 40GBASE-LR4");
    PRINT_BIT_NAME((eeprom[3+0] & (1 << 0)), " 40G-XLPPI");

    PRINT_BIT_NAME((eeprom[3+1] & (1 << 3)), " 40G-OTN");
    PRINT_BIT_NAME((eeprom[3+1] & (1 << 2)), " OC48-LongReach");
    PRINT_BIT_NAME((eeprom[3+1] & (1 << 1)), " OC48-IntReach");
    PRINT_BIT_NAME((eeprom[3+1] & (1 << 0)), " OC48-ShortReach");

    PRINT_BIT_NAME((eeprom[3+2] & (1 << 5)), " SAS-6.0G");
    PRINT_BIT_NAME((eeprom[3+2] & (1 << 4)), " SAS-3.0G");

    PRINT_BIT_NAME((eeprom[3+3] & (1 << 3)), " 1000BASE-T");
    PRINT_BIT_NAME((eeprom[3+3] & (1 << 2)), " 1000BASE-CX");
    PRINT_BIT_NAME((eeprom[3+3] & (1 << 1)), " 1000BASE-LX");
    PRINT_BIT_NAME((eeprom[3+3] & (1 << 0)), " 1000BASE-SX");

    PRINT_BIT_NAME((eeprom[3+4] & (1 << 7)), " VLD");
    PRINT_BIT_NAME((eeprom[3+4] & (1 << 6)), " SD");
    PRINT_BIT_NAME((eeprom[3+4] & (1 << 5)), " ID");
    PRINT_BIT_NAME((eeprom[3+4] & (1 << 4)), " LD");
    PRINT_BIT_NAME((eeprom[3+4] & (1 << 3)), " M");
    PRINT_BIT_NAME((eeprom[3+4] & (1 << 1)), " LC");
    PRINT_BIT_NAME((eeprom[3+4] & (1 << 0)), " EL");

    PRINT_BIT_NAME((eeprom[3+5] & (1 << 7)), " AL");
    PRINT_BIT_NAME((eeprom[3+5] & (1 << 6)), " SN");
    PRINT_BIT_NAME((eeprom[3+5] & (1 << 5)), " SL");
    PRINT_BIT_NAME((eeprom[3+5] & (1 << 4)), " LL");

    PRINT_BIT_NAME((eeprom[3+6] & (1 << 7)), " TwinAxial(TW)");
    PRINT_BIT_NAME((eeprom[3+6] & (1 << 6)), " Shielded(TP)");
    PRINT_BIT_NAME((eeprom[3+6] & (1 << 5)), " MiniCoax(MI)");
    PRINT_BIT_NAME((eeprom[3+6] & (1 << 4)), " VideoCoax(TV)");
    PRINT_BIT_NAME((eeprom[3+6] & (1 << 3)), " MMode62.5m(M6)");
    PRINT_BIT_NAME((eeprom[3+6] & (1 << 2)), " MMode50m(M5)");
    PRINT_BIT_NAME((eeprom[3+6] & (1 << 1)), " MMode50um(OM3)");
    PRINT_BIT_NAME((eeprom[3+6] & (1 << 0)), " SingleMode(SM)");

    FM_LOG_PRINT("\n");

    if (eeprom[3+0] & (1 << 7))
    {
        /* Extended specification compliance defined in SFF-8024 */
        FM_LOG_PRINT("%20s[%02x]: %d", "Ext Spec Comp", 64, eeprom[64]);
        switch (eeprom[64])
        {
            case 0:
                FM_LOG_PRINT("Unspecified");
                break;
            case 1:
                FM_LOG_PRINT("100G AOC - Active Optical Cable");
                break;
            case 2:
                FM_LOG_PRINT("100GBASE-SR4");
                break;
        }
        FM_LOG_PRINT("\n");
    }

}   /* end fmPlatformXcvrEepromDumpSpecCompliance */





/*****************************************************************************/
/** fmPlatformXcvrEepromDumpBaseExt
 * \ingroup intPlatformXcvr
 *
 * \desc            Dumps transceiver BASE and EEPROM eeprom.
 *                  This assumes the eeprom checksum has been verified.
 *
 * \param[out]      eeprom points to storage where the transceiver eeprom
 *                  is stored.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fmPlatformXcvrEepromDumpBaseExt(fm_byte *eeprom)
{
    fm_int    j;
    fm_byte   cs;
    fm_int    addr;

    addr = 0;
    FM_LOG_PRINT("%20s[%02x]: ", "Identifier", addr);
    switch (eeprom[addr])
    {
        case 0x3:
            FM_LOG_PRINT("SFP(0x3)\n");
            break;
        case 0xC:
            FM_LOG_PRINT("QSFP(0xC)\n");
            break;
        case 0xD:
            FM_LOG_PRINT("QSFP+(0xD)\n");
            break;
        case 0x11:
            FM_LOG_PRINT("QSFP28(0x11)\n");
            break;
        default:
            FM_LOG_PRINT("0x%02x\n", eeprom[addr]);
            break;
    }

    fmPlatformXcvrEepromDumpSpecCompliance(eeprom);
    addr = 18;
    FM_LOG_PRINT("%20s[%02x]: %dm\n", "Length", addr, eeprom[addr]);
    addr = 20;
    PrintBytesName("VendorName", addr, &eeprom[addr], 16);
    addr = 37;
    FM_LOG_PRINT("%20s[%02x]: 0x%02x%02x%02x ", "VendorOUI", addr,
                 eeprom[addr], eeprom[addr+1], eeprom[addr+2]);
    PrintBytesName(NULL, addr, &eeprom[addr], 3);
    addr = 40;
    PrintBytesName("VendorPN", addr, &eeprom[addr], 16);
    addr = 56;
    PrintBytesName("VendorRev", addr, &eeprom[addr], 2);
    for (j = 0, cs = 0 ; j <= 62 ; j++)
    {
        cs += eeprom[j];
    }
    addr = 63;
    FM_LOG_PRINT("%20s[%02x]: %02x Calculated: %02x\n", "CC_Base", addr, eeprom[addr], cs);

    addr = 68;
    PrintBytesName("VendorSN", addr, &eeprom[addr], 16);
    addr = 84;
    PrintBytesName("DateCode", addr, &eeprom[addr], 8);
    for (j = 64, cs = 0 ; j <= 94 ; j++) 
    {
        cs += eeprom[j];
    }
    addr = 95;
    FM_LOG_PRINT("%20s[%02x]: %02x Calculated: %02x\n", "CC_EXT", addr, eeprom[addr], cs);

}   /* end fmPlatformXcvrEepromDumpBaseExt */




/*****************************************************************************/
/** fmPlatformSfppXcvrEepromDumpPage1
 * \ingroup intPlatformXcvr
 *
 * \desc            Dumps transceiver SFP+ page 1 eeprom.
 *
 * \param[out]      eeprom points to storage where the transceiver page 1 eeprom
 *                  is stored.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fmPlatformXcvrSfppEepromDumpPage1(fm_byte *eeprom)
{
    fm_int    addr;
    fm_int16  val16;
    fm_uint16 uval16;

    if (!(((eeprom[96] == 0xFF) && (eeprom[97] == 0xFF)) || 
          ((eeprom[96] == 0x00) && (eeprom[97] == 0x00)) ))
    {
        addr = 96;
        val16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: %.1fC\n", "Temperature", addr, val16/256.0);
        addr = 98;
        uval16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: %.2fV\n", "Vcc", addr, uval16/10000.0);
        addr = 100;
        uval16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: %.2fmA\n", "Tx Bias Current", addr, uval16/500.0);
        addr = 102;
        uval16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: %.1fuW\n", "Tx Output Power", addr, uval16/10.0);
        addr = 104;
        uval16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: %.1fuW\n", "Rx Power", addr, uval16/10.0);
    }

}   /* end fmPlatformSfppXcvrEepromDumpPage1 */





/*****************************************************************************/
/** fmPlatforQsfpXcvrEepromDumpPage0
 * \ingroup intPlatformXcvr
 *
 * \desc            Dumps transceiver QSFP page 0 eeprom.
 *
 * \param[out]      eeprom points to storage where the transceiver page 0 eeprom
 *                  is stored.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fmPlatformXcvrQsfpEepromDumpPage0(fm_byte *eeprom)
{
    fm_int    addr;
    fm_int16  val16;
    fm_uint16 uval16;

    if (!(((eeprom[22] == 0xFF) && (eeprom[26] == 0xFF)) || 
          ((eeprom[22] == 0x00) && (eeprom[26] == 0x00)) ))
    {
        addr = 3;
        val16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: ", "LOS", addr);
        PRINT_BIT_NAME((eeprom[addr] & (1 << 0)), " RX1");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 1)), " RX2");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 2)), " RX3");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 3)), " RX4");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 4)), " TX1");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 5)), " TX2");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 6)), " TX3");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 7)), " TX4");
        FM_LOG_PRINT("\n");

        addr = 4;
        val16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: ", "FAULT", addr);
        PRINT_BIT_NAME((eeprom[addr] & (1 << 0)), " TX1");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 1)), " TX2");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 2)), " TX3");
        PRINT_BIT_NAME((eeprom[addr] & (1 << 3)), " TX4");
        FM_LOG_PRINT("\n");

        addr = 22;
        val16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: %.1fC\n", "Temperature", addr, val16/256.0);
        addr = 26;
        uval16 =  (eeprom[addr] << 8) | eeprom[addr+1];
        FM_LOG_PRINT("%20s[%02x]: %.2fV\n", "Vcc", addr, uval16/10000.0);

        addr = 34;
        FM_LOG_PRINT("%20s[%02x]: %.1f %.1f %.1f %.1f uW\n", "Rx Power", addr,
            ((eeprom[addr+0] << 8) | eeprom[addr+1])/10.0,
            ((eeprom[addr+2] << 8) | eeprom[addr+3])/10.0,
            ((eeprom[addr+4] << 8) | eeprom[addr+4])/10.0,
            ((eeprom[addr+6] << 8) | eeprom[addr+5])/10.0);

        addr = 42;
        FM_LOG_PRINT("%20s[%02x]: %.2f %.2f %.2f %.2f mA\n", "Tx Bias Current", addr,
            ((eeprom[addr+0] << 8) | eeprom[addr+1])/500.0,
            ((eeprom[addr+2] << 8) | eeprom[addr+3])/500.0,
            ((eeprom[addr+4] << 8) | eeprom[addr+4])/500.0,
            ((eeprom[addr+6] << 8) | eeprom[addr+5])/500.0);

    }

}   /* end fmPlatformQsfpXcvrEepromDumpPage0 */

