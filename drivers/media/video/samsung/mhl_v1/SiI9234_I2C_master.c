/***************************************************************************

* 

*   SiI9244 ? MHL Transmitter Driver

*

* Copyright (C) (2011, Silicon Image Inc)

*

* This program is free software; you can redistribute it and/or modify

* it under the terms of the GNU General Public License as published by

* the Free Software Foundation version 2.

*

* This program is distributed ¡°as is¡± WITHOUT ANY WARRANTY of any

* kind, whether express or implied; without even the implied warranty

* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

* GNU General Public License for more details.

*

*****************************************************************************/

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <plat/pm.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>

#include <linux/syscalls.h> 
#include <linux/fcntl.h> 
#include <asm/uaccess.h> 
#include <linux/types.h>


#include "Common_Def.h"
#include "SiI9234_I2C_slave_add.h"


/*===========================================================================

===========================================================================*/

#define READ   1
#define WRITE  0

#define LAST_BYTE      1
#define NOT_LAST_BYTE  0


#define TPI_INDEXED_PAGE_REG		0xBC
#define TPI_INDEXED_OFFSET_REG		0xBD
#define TPI_INDEXED_VALUE_REG		0xBE


/*===========================================================================

===========================================================================*/
//------------------------------------------------------------------------------
// Function: I2C_WriteByte
// Description:
//------------------------------------------------------------------------------
void I2C_WriteByte(byte deviceID, byte offset, byte value)
{

	struct i2c_client* client_ptr = get_sii9234_client(deviceID);
	if(!client_ptr)
	{
		printk("[MHL]I2C_WriteByte error %x\n",deviceID); 
		return;	
	}
	
	if(deviceID == 0x72)
		sii9234_i2c_write(client_ptr,offset,value);
	else if(deviceID == 0x7A)
		sii9234_i2c_write(client_ptr,offset,value);
	else if(deviceID == 0x92)
		sii9234_i2c_write(client_ptr,offset,value);
	else if(deviceID == 0xC8)
		sii9234_i2c_write(client_ptr,offset,value);


}


byte I2C_ReadByte(byte deviceID, byte offset)
{
    	byte number = 0;
	struct i2c_client* client_ptr = get_sii9234_client(deviceID);
	if(!client_ptr)
	{
		printk("[MHL]I2C_ReadByte error %x\n",deviceID); 
		return 0;	
	}

  
  	if(deviceID == 0x72)
		number = sii9234_i2c_read(client_ptr,offset);
	else if(deviceID == 0x7A)
		number = sii9234_i2c_read(client_ptr,offset);
	else if(deviceID == 0x92)
		number = sii9234_i2c_read(client_ptr,offset);
	else if(deviceID == 0xC8)
		number = sii9234_i2c_read(client_ptr,offset);
	
    return (number);

}

byte ReadByteTPI (byte Offset) 
{
	return I2C_ReadByte(SA_TX_Page0_Primary, Offset);
}

void WriteByteTPI (byte Offset, byte Data) 
{
	I2C_WriteByte(SA_TX_Page0_Primary, Offset, Data);
}



void ReadModifyWriteTPI(byte Offset, byte Mask, byte Data) 
{

	byte Temp;

	Temp = ReadByteTPI(Offset);		// Read the current value of the register.
	Temp &= ~Mask;					// Clear the bits that are set in Mask.
	Temp |= (Data & Mask);			// OR in new value. Apply Mask to Value for safety.
	WriteByteTPI(Offset, Temp);		// Write new value back to register.
}

byte ReadByteCBUS (byte Offset) 
{
	return I2C_ReadByte(SA_TX_CBUS_Primary, Offset);
}

void WriteByteCBUS(byte Offset, byte Data) 
{
	I2C_WriteByte(SA_TX_CBUS_Primary, Offset, Data);
}

void ReadModifyWriteCBUS(byte Offset, byte Mask, byte Value) 
{
  byte Temp;

  Temp = ReadByteCBUS(Offset);
  Temp &= ~Mask;
  Temp |= (Value & Mask);
  WriteByteCBUS(Offset, Temp);
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadIndexedRegister ()
//
// PURPOSE		:	Read the value from an indexed register.
//
//					Write:
//						1. 0xBC => Indexed page num
//						2. 0xBD => Indexed register offset
//
//					Read:
//						3. 0xBE => Returns the indexed register value
//
// INPUT PARAMS	:	PageNum	-	indexed page number
//					Offset	-	offset of the register within the indexed page.
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	The value read from the indexed register.
//
//////////////////////////////////////////////////////////////////////////////

byte ReadIndexedRegister (byte PageNum, byte Offset) 
{
	WriteByteTPI(TPI_INDEXED_PAGE_REG, PageNum);		// Indexed page
	WriteByteTPI(TPI_INDEXED_OFFSET_REG, Offset);		// Indexed register
	return ReadByteTPI(TPI_INDEXED_VALUE_REG);			// Return read value
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	WriteIndexedRegister ()
//
// PURPOSE		:	Write a value to an indexed register
//
//					Write:
//						1. 0xBC => Indexed page num
//						2. 0xBD => Indexed register offset
//						3. 0xBE => Set the indexed register value
//
// INPUT PARAMS	:	PageNum	-	indexed page number
//					Offset	-	offset of the register within the indexed page.
//					Data	-	the value to be written.
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED :	None
//
// RETURNS		:	None
//
//////////////////////////////////////////////////////////////////////////////

void WriteIndexedRegister (byte PageNum, byte Offset, byte Data) 
{
	WriteByteTPI(TPI_INDEXED_PAGE_REG, PageNum);		// Indexed page
	WriteByteTPI(TPI_INDEXED_OFFSET_REG, Offset);		// Indexed register
	WriteByteTPI(TPI_INDEXED_VALUE_REG, Data);			// Write value
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadModifyWriteIndexedRegister ()
//
// PURPOSE		:	Set or clear individual bits in a TPI register.
//
// INPUT PARAMS	:	PageNum	-	indexed page number
//					Offset	-	the offset of the indexed register to be modified.
//					Mask	-	"1" for each indexed register bit that needs to be
//								modified
//					Data	-	The desired value for the register bits in their
//								proper positions
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	void
//
//////////////////////////////////////////////////////////////////////////////

void ReadModifyWriteIndexedRegister (byte PageNum, byte Offset, byte Mask, byte Data) 
{

	byte Temp;

	Temp = ReadIndexedRegister (PageNum, Offset);	// Read the current value of the register.
	Temp &= ~Mask;									// Clear the bits that are set in Mask.
	Temp |= (Data & Mask);							// OR in new value. Apply Mask to Value for safety.
	WriteByteTPI(TPI_INDEXED_VALUE_REG, Temp);		// Write new value back to register.
}

