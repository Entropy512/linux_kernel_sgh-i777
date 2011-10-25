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


#include "Common_Def.h"

#include <linux/types.h>

/*===========================================================================

===========================================================================*/


void I2C_WriteByte(byte deviceID, byte offset, byte value);
byte I2C_ReadByte(byte deviceID, byte offset);

byte ReadByteTPI (byte Offset); 
void WriteByteTPI (byte Offset, byte Data);
void WriteIndexedRegister (byte PageNum, byte Offset, byte Data);
void ReadModifyWriteIndexedRegister (byte PageNum, byte Offset, byte Mask, byte Data);
void ReadModifyWriteIndexedRegister (byte PageNum, byte Offset, byte Mask, byte Data);
void ReadModifyWriteTPI(byte Offset, byte Mask, byte Data);
void WriteByteCBUS(byte Offset, byte Data);
void ReadModifyWriteCBUS(byte Offset, byte Mask, byte Value) ;
byte ReadIndexedRegister (byte PageNum, byte Offset) ;
byte ReadByteCBUS (byte Offset) ;




