/** @file OSPmessage.h
 * Contains the OSPMessage class definition used to acquire data from OSP messages taking into account their type.
 *
 *Copyright 2015 Francisco Cancillo
 *<p>
 *This file is part of the RXtoRINEX tool.
 *<p>
 *RXtoRINEX is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 *as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *RXtoRINEX is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *<p>
 *A copy of the GNU General Public License can be found at <http://www.gnu.org/licenses/>.
 *<p>Ver.	|Date	|Reason for change
 *<p>---------------------------------
 *<p>V1.0	|2/2015	|First release
 */
#ifndef OSPMESSAGE_H
#define OSPMESSAGE_H

#include <stdio.h>

///The maximum size in bytes of any message payload
#define MAXPAYLOADSIZE 2048

/**OSPMessage class provides resources to perform data acquisition from OSP message payload.
 *Note that a payload is a part of the OSP message described in the SiRF ICD.
 *<p>The class allows a cursor based, buffered acquisition process from the OSP file and include methods to get
 *values for basic data types at the current position of the cursor from the byte stream in the payload.
 *<p>Methods are defined to:
 * - fill the buffer with a OSP message read from OSP binary file
 * - get the value of the specific types a message could contain (byte, integer (short or not,
 *		unsigned or not), float or double). Bit and byte ordering in the source are taken into account to perform the translation.
 * - skip unused data from the buffer advancing the cursor
 */
class OSPMessage {
	unsigned char payload[MAXPAYLOADSIZE];	//buffer for the OSP message payload
	unsigned int payloadLength;		//the payload length in bytes of current message
	unsigned int cursor;	//payload index to the first byte to be extracted by any method defined below
							//it is incremented after any extraction
public:
	OSPMessage(void);
	~OSPMessage(void);
	bool fill(FILE*);	//fill the buffer whith a OSP message read from OSP binary file
	int get();			//get from payload the byte value at cursor. Increment it by one
	int getInt();		//get from payload the 32 bits integer at cursor. Increment it by four
	unsigned int getUInt(); //get from payload the 32 bits unsigned integer at cursor. Increment it by four
	short int getShort();	//get from payload the 16 bits integer at cursor. Increment it by two
	short unsigned int getUShort();	//get from payload the 16 bits unsigned integer at cursor. Increment it by two
	float getFloat();	//get from payload the 32 bits floating point at cursor. Increment it by four
	double getDouble();	//get from payload the 64 bits floating point at cursor. Increment it by eigth
	int getInt3();		//get from payload the 24 bits integer at cursor. Increment it by three
	bool skipBytes(int n);	//skip n bytes advancing cursor by n
	unsigned int payloadLen(); //provides the payload length
};
#endif