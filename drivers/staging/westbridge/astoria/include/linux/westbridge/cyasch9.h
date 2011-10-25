/*
## Cypress West Bridge API header file (cyasch9.h)
 ## ===========================
## Copyright (C) 2010  Cypress Semiconductor
 ##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
 ##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
 ##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor
## Boston, MA  02110-1301, USA.
 ## ===========================
*/

#ifndef _INCLUDED_CYASCH9_H_
#define _INCLUDED_CYASCH9_H_

/*
 * These chapter 9 USB 2.0 constants are provided for convenience
 */

#define CY_CH9_SETUP_TYPE_MASK				(0x60)			/* Used to mask off request type */
#define CY_CH9_SETUP_STANDARD_REQUEST			(0)			/* Standard Request */
#define CY_CH9_SETUP_CLASS_REQUEST			(0x20)			/* Class Request */
#define CY_CH9_SETUP_VENDOR_REQUEST			(0x40)			/* Vendor Request */
#define CY_CH9_SETUP_RESERVED_REQUEST 			(0x60)			/* Reserved or illegal request */

#define CY_CH9_SETUP_DEST_MASK				(0x1f)			/* Used to mask off the dest of the request */
#define CY_CH9_SETUP_DEST_DEVICE			(0)
#define CY_CH9_SETUP_DEST_INTERFACE			(1)
#define CY_CH9_SETUP_DEST_ENDPOINT			(2)
#define CY_CH9_SETUP_DEST_OTHER				(3)

#define CY_CH9_SC_GET_STATUS				(0x00)			/*  Setup command: Get Status */
#define CY_CH9_SC_CLEAR_FEATURE				(0x01)			/*  Setup command: Clear Feature */
#define CY_CH9_SC_RESERVED				(0x02)			/*  Setup command: Reserved */
#define CY_CH9_SC_SET_FEATURE				(0x03)			/*  Setup command: Set Feature */
#define CY_CH9_SC_SET_ADDRESS				(0x05)			/*  Setup command: Set Address */
#define CY_CH9_SC_GET_DESCRIPTOR			(0x06)			/*  Setup command: Get Descriptor */
#define CY_CH9_SC_SET_DESCRIPTOR			(0x07)			/*  Setup command: Set Descriptor */
#define CY_CH9_SC_GET_CONFIGURATION			(0x08)			/*  Setup command: Get Configuration */
#define CY_CH9_SC_SET_CONFIGURATION			(0x09)			/*  Setup command: Set Configuration */
#define CY_CH9_SC_GET_INTERFACE				(0x0a)			/*  Setup command: Get Interface */
#define CY_CH9_SC_SET_INTERFACE				(0x0b)			/*  Setup command: Set Interface */
#define CY_CH9_SC_SYNC_FRAME				(0x0c)			/*  Setup command: Sync Frame */
#define CY_CH9_SC_ANCHOR_LOAD				(0xa0)			/*  Setup command: Anchor load */
   
#define CY_CH9_GD_DEVICE				(0x01)			/*  Get descriptor: Device */
#define CY_CH9_GD_CONFIGURATION				(0x02)			/*  Get descriptor: Configuration */
#define CY_CH9_GD_STRING				(0x03)			/*  Get descriptor: String */
#define CY_CH9_GD_INTERFACE				(0x04)			/*  Get descriptor: Interface */
#define CY_CH9_GD_ENDPOINT				(0x05)			/*  Get descriptor: Endpoint */
#define CY_CH9_GD_DEVICE_QUALIFIER			(0x06)			/*  Get descriptor: Device Qualifier */
#define CY_CH9_GD_OTHER_SPEED_CONFIGURATION		(0x07)			/*  Get descriptor: Other Configuration */
#define CY_CH9_GD_INTERFACE_POWER			(0x08)			/*  Get descriptor: Interface Power */
#define CY_CH9_GD_HID					(0x21)			/*  Get descriptor: HID */
#define CY_CH9_GD_REPORT				(0x22)			/*  Get descriptor: Report */

#define CY_CH9_GS_DEVICE				(0x80)			/*  Get Status: Device */
#define CY_CH9_GS_INTERFACE				(0x81)			/*  Get Status: Interface */
#define CY_CH9_GS_ENDPOINT				(0x82)			/*  Get Status: End Point */

#define CY_CH9_FT_DEVICE				(0x00)			/*  Feature: Device */
#define CY_CH9_FT_ENDPOINT				(0x02)			/*  Feature: End Point */

/*
 * Language Codes
 */

#define CY_CH9_LANGID_AFRIKAANS				(0x0436)
#define CY_CH9_LANGID_ALBANIAN				(0x041c)
#define CY_CH9_LANGID_ARABIC_SAUDI_ARABIA		(0x0401)
#define CY_CH9_LANGID_ARABIC_IRAQ			(0x0801)
#define CY_CH9_LANGID_ARABIC_EGYPT			(0x0c01)
#define CY_CH9_LANGID_ARABIC_LIBYA			(0x1001)
#define CY_CH9_LANGID_ARABIC_ALGERIA			(0x1401)
#define CY_CH9_LANGID_ARABIC_MOROCCO			(0x1801)
#define CY_CH9_LANGID_ARABIC_TUNISIA			(0x1c01)
#define CY_CH9_LANGID_ARABIC_OMAN			(0x2001)
#define CY_CH9_LANGID_ARABIC_YEMEN			(0x2401)
#define CY_CH9_LANGID_ARABIC_SYRIA			(0x2801)
#define CY_CH9_LANGID_ARABIC_JORDAN			(0x2c01)
#define CY_CH9_LANGID_ARABIC_LEBANON			(0x3001)
#define CY_CH9_LANGID_ARABIC_KUWAIT			(0x3401)
#define CY_CH9_LANGID_ARABIC_UAE			(0x3801)
#define CY_CH9_LANGID_ARABIC_BAHRAIN			(0x3c01)
#define CY_CH9_LANGID_ARABIC_QATAR			(0x4001)
#define CY_CH9_LANGID_ARMENIAN				(0x042b)
#define CY_CH9_LANGID_ASSAMESE				(0x044d)
#define CY_CH9_LANGID_AZERI_LATIN			(0x042c)
#define CY_CH9_LANGID_AZERI_CYRILLIC			(0x082c)
#define CY_CH9_LANGID_BASQUE				(0x042d)
#define CY_CH9_LANGID_BELARUSSIAN			(0x0423)
#define CY_CH9_LANGID_BENGALI				(0x0445)
#define CY_CH9_LANGID_BULGARIAN				(0x0402)
#define CY_CH9_LANGID_BURMESE				(0x0455)
#define CY_CH9_LANGID_CATALAN				(0x0403)
#define CY_CH9_LANGID_CHINESE_TAIWAN			(0x0404)
#define CY_CH9_LANGID_CHINESE_PRC			(0x0804)
#define CY_CH9_LANGID_CHINESE_HONG_KONG_SAR_PRC		(0x0c04)
#define CY_CH9_LANGID_CHINESE_SINGAPORE			(0x1004)
#define CY_CH9_LANGID_CHINESE_MACAU_SAR			(0x1404)
#define CY_CH9_LANGID_CROATIAN				(0x041a)
#define CY_CH9_LANGID_CZECH				(0x0405)
#define CY_CH9_LANGID_DANISH				(0x0406)
#define CY_CH9_LANGID_DUTCH_NETHERLANDS			(0x0413)
#define CY_CH9_LANGID_DUTCH_BELGIUM			(0x0813)
#define CY_CH9_LANGID_ENGLISH_UNITED_STATES		(0x0409)
#define CY_CH9_LANGID_ENGLISH_UNITED_KINGDON		(0x0809)
#define CY_CH9_LANGID_ENGLISH_AUSTRALIAN		(0x0c09)
#define CY_CH9_LANGID_ENGLISH_CANADIAN			(0x1009)
#define CY_CH9_LANGID_ENGLISH_NEW Zealand		(0x1409)
#define CY_CH9_LANGID_ENGLISH_IRELAND			(0x1809)
#define CY_CH9_LANGID_ENGLISH_SOUTH Africa		(0x1c09)
#define CY_CH9_LANGID_ENGLISH_JAMAICA			(0x2009)
#define CY_CH9_LANGID_ENGLISH_CARIBBEAN			(0x2409)
#define CY_CH9_LANGID_ENGLISH_BELIZE			(0x2809)
#define CY_CH9_LANGID_ENGLISH_TRINIDAD			(0x2c09)
#define CY_CH9_LANGID_ENGLISH_ZIMBABWE			(0x3009)
#define CY_CH9_LANGID_ENGLISH_PHILIPPINES		(0x3409)
#define CY_CH9_LANGID_ESTONIAN				(0x0425)
#define CY_CH9_LANGID_FAEROESE				(0x0438)
#define CY_CH9_LANGID_FARSI				(0x0429)
#define CY_CH9_LANGID_FINNISH				(0x040b)
#define CY_CH9_LANGID_FRENCH_STANDARD			(0x040c)
#define CY_CH9_LANGID_FRENCH_BELGIAN			(0x080c)
#define CY_CH9_LANGID_FRENCH_CANADIAN			(0x0c0c)
#define CY_CH9_LANGID_FRENCH_SWITZERLAND		(0x100c)
#define CY_CH9_LANGID_FRENCH_LUXEMBOURG			(0x140c)
#define CY_CH9_LANGID_FRENCH_MONACO			(0x180c)
#define CY_CH9_LANGID_GEORGIAN				(0x0437)
#define CY_CH9_LANGID_GERMAN_STANDARD			(0x0407)
#define CY_CH9_LANGID_GERMAN_SWITZERLAND		(0x0807)
#define CY_CH9_LANGID_GERMAN_AUSTRIA			(0x0c07)
#define CY_CH9_LANGID_GERMAN_LUXEMBOURG			(0x1007)
#define CY_CH9_LANGID_GERMAN_LIECHTENSTEIN		(0x1407)
#define CY_CH9_LANGID_GREEK				(0x0408)
#define CY_CH9_LANGID_GUJARATI				(0x0447)
#define CY_CH9_LANGID_HEBREW				(0x040d)
#define CY_CH9_LANGID_HINDI				(0x0439)
#define CY_CH9_LANGID_HUNGARIAN				(0x040e)
#define CY_CH9_LANGID_ICELANDIC				(0x040f)
#define CY_CH9_LANGID_INDONESIAN			(0x0421)
#define CY_CH9_LANGID_ITALIAN_STANDARD			(0x0410)
#define CY_CH9_LANGID_ITALIAN_SWITZERLAND		(0x0810)
#define CY_CH9_LANGID_JAPANESE				(0x0411)
#define CY_CH9_LANGID_KANNADA				(0x044b)
#define CY_CH9_LANGID_KASHMIRI_INDIA			(0x0860)
#define CY_CH9_LANGID_KAZAKH				(0x043f)
#define CY_CH9_LANGID_KONKANI				(0x0457)
#define CY_CH9_LANGID_KOREAN				(0x0412)
#define CY_CH9_LANGID_KOREAN_JOHAB			(0x0812)
#define CY_CH9_LANGID_LATVIAN				(0x0426)
#define CY_CH9_LANGID_LITHUANIAN			(0x0427)
#define CY_CH9_LANGID_LITHUANIAN_CLASSIC		(0x0827)
#define CY_CH9_LANGID_MACEDONIAN			(0x042f)
#define CY_CH9_LANGID_MALAY_MALAYSIAN			(0x043e)
#define CY_CH9_LANGID_MALAY_BRUNEI_DARUSSALAM		(0x083e)
#define CY_CH9_LANGID_MALAYALAM				(0x044c)
#define CY_CH9_LANGID_MANIPURI				(0x0458)
#define CY_CH9_LANGID_MARATHI				(0x044e)
#define CY_CH9_LANGID_NEPALI_INDIA			(0x0861)
#define CY_CH9_LANGID_NORWEGIAN_BOKMAL			(0x0414)
#define CY_CH9_LANGID_NORWEGIAN_NYNORSK			(0x0814)
#define CY_CH9_LANGID_ORIYA				(0x0448)
#define CY_CH9_LANGID_POLISH				(0x0415)
#define CY_CH9_LANGID_PORTUGUESE_BRAZIL			(0x0416)
#define CY_CH9_LANGID_PORTUGUESE_STANDARD		(0x0816)
#define CY_CH9_LANGID_PUNJABI				(0x0446)
#define CY_CH9_LANGID_ROMANIAN				(0x0418)
#define CY_CH9_LANGID_RUSSIAN				(0x0419)
#define CY_CH9_LANGID_SANSKRIT				(0x044f)
#define CY_CH9_LANGID_SERBIAN_CYRILLIC			(0x0c1a)
#define CY_CH9_LANGID_SERBIAN_LATIN			(0x081a)
#define CY_CH9_LANGID_SINDHI				(0x0459)
#define CY_CH9_LANGID_SLOVAK				(0x041b)
#define CY_CH9_LANGID_SLOVENIAN				(0x0424)
#define CY_CH9_LANGID_SPANISH_TRADITIONAL_SORT		(0x040a)
#define CY_CH9_LANGID_SPANISH_MEXICAN			(0x080a)
#define CY_CH9_LANGID_SPANISH_MODERN_SORT		(0x0c0a)
#define CY_CH9_LANGID_SPANISH_GUATEMALA			(0x100a)
#define CY_CH9_LANGID_SPANISH_COSTA_RICA		(0x140a)
#define CY_CH9_LANGID_SPANISH_PANAMA			(0x180a)
#define CY_CH9_LANGID_SPANISH_DOMINICAN Republic	(0x1c0a)
#define CY_CH9_LANGID_SPANISH_VENEZUELA			(0x200a)
#define CY_CH9_LANGID_SPANISH_COLOMBIA			(0x240a)
#define CY_CH9_LANGID_SPANISH_PERU			(0x280a)
#define CY_CH9_LANGID_SPANISH_ARGENTINA			(0x2c0a)
#define CY_CH9_LANGID_SPANISH_ECUADOR			(0x300a)
#define CY_CH9_LANGID_SPANISH_CHILE			(0x340a)
#define CY_CH9_LANGID_SPANISH_URUGUAY			(0x380a)
#define CY_CH9_LANGID_SPANISH_PARAGUAY			(0x3c0a)
#define CY_CH9_LANGID_SPANISH_BOLIVIA			(0x400a)
#define CY_CH9_LANGID_SPANISH_EL_SALVADOR		(0x440a)
#define CY_CH9_LANGID_SPANISH_HONDURAS			(0x480a)
#define CY_CH9_LANGID_SPANISH_NICARAGUA			(0x4c0a)
#define CY_CH9_LANGID_SPANISH_PUERTO Rico		(0x500a)
#define CY_CH9_LANGID_SUTU				(0x0430)
#define CY_CH9_LANGID_SWAHILI_KENYA			(0x0441)
#define CY_CH9_LANGID_SWEDISH				(0x041d)
#define CY_CH9_LANGID_SWEDISH_FINLAND			(0x081d)
#define CY_CH9_LANGID_TAMIL				(0x0449)
#define CY_CH9_LANGID_TATAR_TATARSTAN			(0x0444)
#define CY_CH9_LANGID_TELUGU				(0x044a)
#define CY_CH9_LANGID_THAI				(0x041e)
#define CY_CH9_LANGID_TURKISH				(0x041f)
#define CY_CH9_LANGID_UKRAINIAN				(0x0422)
#define CY_CH9_LANGID_URDU_PAKISTAN			(0x0420)
#define CY_CH9_LANGID_URDU_INDIA			(0x0820)
#define CY_CH9_LANGID_UZBEK_LATIN			(0x0443)
#define CY_CH9_LANGID_UZBEK_CYRILLIC			(0x0843)
#define CY_CH9_LANGID_VIETNAMESE			(0x042a)

#pragma pack(push, 1)
typedef struct CyCh9DeviceDesc
{
    uint8_t bLength ;
    uint8_t bDescriptorType ;
    uint16_t bcdUSB ;
    uint8_t bDeviceClass ;
    uint8_t bDeviceSubClass ;
    uint8_t bDeviceProtocol ;
    uint8_t bMaxPacketSize0 ;
    uint16_t idVendor ;
    uint16_t idProduct ;
    uint16_t bcdDevice ;
    uint8_t iManufacturer ;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} CyCh9DeviceDesc ;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct CyCh9DeviceQualifierDesc
{
    uint8_t bLength ;
    uint8_t bDescriptorType ;
    uint16_t bcdUSB ;
    uint8_t bDeviceClass ;
    uint8_t bDeviceSubClass ;
    uint8_t bDeviceProtocol ;
    uint8_t bMaxPacketSize0 ;
    uint8_t bNumConfigurations;
    uint8_t reserved ;
} CyCh9DeviceQualifierDesc ;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct CyCh9ConfigurationDesc
{
    uint8_t bLength ;
    uint8_t bDescriptorType ;
    uint16_t wTotalLength ;
    uint8_t bNumInterfaces ;
    uint8_t bConfigurationValue ;
    uint8_t iConfiguration ;
    uint8_t bmAttributes ;
    uint8_t bMaxPower ;
} CyCh9ConfigurationDesc ;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct CyCh9InterfaceDesc
{
    uint8_t bLength ;
    uint8_t bDescriptorType ;
    uint8_t bInterfaceNumber ;
    uint8_t bAlternateSetting ;
    uint8_t bNumEndpoints ;
    uint8_t bInterfaceClass ;
    uint8_t bInterfaceSubClass ;
    uint8_t bInterfaceProtocol ;
    uint8_t iInterface ;
} CyCh9InterfaceDesc ;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct CyCh9EndpointDesc
{
    uint8_t bLength ;
    uint8_t bDescriptorType ;
    uint8_t bEndpointAddress ;
    uint8_t bmAttributes ;
    uint16_t wMaxPacketSize ;
    uint8_t bInterval ;
} CyCh9EndpointDesc ;
#pragma pack(pop)

/* Configuration attributes */
#define CY_CH9_CONFIG_ATTR_RESERVED_BIT7			(0x80)
#define CY_CH9_CONFIG_ATTR_SELF_POWERED				(0x40)
#define CY_CH9_CONFIG_ATTR_REMOTE_WAKEUP			(0x20)

#define CY_CH9_ENDPOINT_DIR_OUT					(0)
#define CY_CH9_ENDPOINT_DIR_IN					(0x80)
#define CY_CH9_MK_EP_ADDR(dir, num)				((dir) | ((num) & 0x0f))

#define CY_CH9_ENDPOINT_ATTR_CONTROL				(0)
#define CY_CH9_ENDPOINT_ATTR_ISOCHRONOUS			(1)
#define CY_CH9_ENDPOINT_ATTR_BULK				(2)
#define CY_CH9_ENDPOINT_ATTR_INTERRUPT				(3)

#define CY_CH9_ENDPOINT_ATTR_NO_SYNC				(0 << 2)
#define CY_CH9_ENDPOINT_ATTR_ASYNCHRONOUS			(1 << 2)
#define CY_CH9_ENDPOINT_ATTR_ADAPTIVE				(2 << 2)
#define CY_CH9_ENDPOINT_ATTR_SYNCHRONOUS			(3 << 2)

#define CY_CH9_ENDPOINT_ATTR_DATA				(0 << 4)
#define CY_CH9_ENDPOINT_ATTR_FEEDBACK				(1 << 4)
#define CY_CH9_ENDPOINT_ATTR_IMPLICIT_FEEDBACK			(2 << 4)
#define CY_CH9_ENDPOINT_ATTR_RESERVED				(3 << 4)

#define CY_CH9_ENDPOINT_PKTSIZE_NONE				(0 << 11)
#define CY_CH9_ENDPOINT_PKTSIZE_ONE				(1 << 11)
#define CY_CH9_ENDPOINT_PKTSIZE_TWO				(2 << 11)
#define CY_CH9_ENDPOINT_PKTSIZE_RESERVED			(3 << 11)
#endif
