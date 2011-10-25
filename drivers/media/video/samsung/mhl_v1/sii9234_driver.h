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





/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/


/*===========================================================================
                   FUNCTION DEFINITIONS
===========================================================================*/


void sii9234_interrupt_event(void);
bool sii9234_init(void);

void sii9234_unmaks_interrupt(void);

//MHL IOCTL INTERFACE
#define MHL_READ_RCP_DATA 0x1

