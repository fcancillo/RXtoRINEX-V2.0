/** @file OSPmessage.cpp
 * Contains the implementation of the OSPmessage class.
 */

#include "OSPMessage.h"
//from CommonClasses
#include "Utilities.h"

/**Constructs and empty OSPMessage object.
 */
OSPMessage::OSPMessage(void) {
	cursor = 0;
	payloadLength = 0;
}

/**Destructs OSPmessage objects.
 */
OSPMessage::~OSPMessage(void) {
}

/**fill fills a OSPMessage object buffer with data extracted from next message in the OSP binary file.
 * First the length of next payload message is read, and then the payload bytes are read into the buffer.
 * The buffer cursor for further extractions from the buffer is set to 0.
 * <p>For a message to be correctly read:
 * - payload length shall be correctly read and its value shall be less than the maximum payload size (as defined in the OSP ICD)
 * - payload bytes shall be correctly read
 *
 * @param file the pointer to the OSP binary FILE containing messages
 * @return true when a message was correctly read, false otherwise (read error or end of file found)
 */
bool OSPMessage::fill(FILE* file) {
	unsigned char buffer[2];

	cursor = 0;
	//read message length from the input stream
	if (fread(buffer, 1, 2, file) < 2) return false;
	payloadLength = (buffer[0] << 8) | buffer[1];	//numbers in msg are big endians
	//read payload bytes
	if (payloadLength > MAXPAYLOADSIZE) return false;
	if (fread(payload, 1, payloadLength, file) < payloadLength) return false;
	return true;
}

/**skipBytes skips the number of bytes stated in the argument from the payload buffer.
 * It increments the payload cursor to allow next data extraction of values after bytes skipped. 
 *
 * @param n the number of bytes to skip, or increment to apply to the buffer cursor
 * @return true if the cursor remains inside current payload (cursor < payloadLength)
 */
bool OSPMessage::skipBytes(int n) {
	cursor += n;
	if (cursor >= payloadLength) return false;
	return true;
}

/**payloadLen provides the length of the current payload
 *
 * @return the payload length of the message in buffer
 */
unsigned int OSPMessage::payloadLen() {
	return payloadLength;
}

/**get gets the byte value in the payload at current cursor position.
 * The cursor is incremented by one after getting the byte.
 *
 * @return the byte value at cursor
 * @throws the integer value 1 when it is intended to get data after the end of payload 
 */
int OSPMessage::get() {
	if (cursor >= payloadLength) throw 1;
	unsigned int value = payload[cursor];
	cursor += 1;
	return value;
}

/**getInt gets four bytes starting at cursor which are interpreted as a 32 bits integer.
 * Integers are stored in the payload with the most significant byte first.
 * The cursor is incremented by four
 *
 * @return the integer value of the four bytes starting at cursor 
 * @throws the integer value 2 when it is intended to get data after the end of payload 
 */
int OSPMessage::getInt() {
	if (cursor+3 >= payloadLength) throw 2;
	unsigned int value = 
		payload[cursor] << 24 |
		payload[cursor+1] << 16 |
		payload[cursor+2] << 8 |
		payload[cursor+3];
	cursor += 4;
	return value;
}

/**getUInt method gets four bytes starting at cursor which are interpreted as a 32 bits unsigned integer.
 * Integers are stored in the payload with the most significant byte first.
 * The cursor is incremented by four
 *
 * @return the unsigned integer value of the four bytes starting at cursor 
 * @throws the integer value 3 when it is intended to get data after the end of payload 
 */
unsigned int OSPMessage::getUInt() {
	if (cursor+3 >= payloadLength) throw 3;
	unsigned int value = 
		payload[cursor] << 24 |
		payload[cursor+1] << 16 |
		payload[cursor+2] << 8 |
		payload[cursor+3];
	cursor += 4;
	return value;
}

/**getShort gets two bytes starting at cursor which are interpreted as a 16 bits integer.
 * Integers are stored in the payload with the most significant byte first.
 * The cursor is incremented by four
 *
 * @return the short integer value of the two bytes starting at cursor 
 * @throws the integer value 4 when it is intended to get data after the end of payload 
 */
short OSPMessage::getShort() {
	if (cursor+1 >= payloadLength) throw 4;
	unsigned short value = payload[cursor] << 8 | payload[cursor+1];
	cursor += 2;
	return value;
}

/**getUShort gets from payload two bytes starting at cursor which are interpreted as a 16 bits unsigned integer.
 * Integers are stored in the payload with the most significant byte first.
 * The cursor is incremented by four
 *
 * @return the unsigned short integer value of the two bytes starting at cursor 
 * @throws the integer value 5 when it is intended to get data after the end of payload 
 */
unsigned short OSPMessage::getUShort() {
	if (cursor+1 >= payloadLength) throw 5;
	unsigned short value = payload[cursor] << 8 | payload[cursor+1];
	cursor += 2;
	return value;
}

/**getFloat gets four bytes starting at cursor which are interpreted as a 32 bits floating number.
 * Floating numbers are stored in the payload with bytes in reverse order.
 * The cursor is incremented by four
 *
 * @return the float value of the four bytes starting at cursor
 * @throws the integer value 6 when it is intended to get data after the end of payload 
 */
float OSPMessage::getFloat() {
	unsigned char buffer[4];
	float* p2value = (float*) buffer;
	if (cursor+3 >= payloadLength) throw 6;
	buffer[0] = payload[cursor + 3];
	buffer[1] = payload[cursor + 2];
	buffer[2] = payload[cursor + 1];
	buffer[3] = payload[cursor + 0];
	cursor += 4;
	return *p2value;
}

/**getDouble gets eight bytes starting at cursor which are interpreted as a 64 bits floating number.
 * Double numbers are stored in the payload with bytes in the following order: 3, 2, 1, 0, 4, 5, 6, 7.
 * The cursor is incremented by eight
 *
 * @return the float value of the eight bytes starting at cursor 
 * @throws the integer value 7 when it is intended to get data after the end of payload 
 */
double OSPMessage::getDouble() {
	unsigned char buffer[8];
	double* p2value = (double*) buffer;
	if (cursor+7 >= payloadLength) throw 7;
	buffer[0] = payload[cursor + 3];
	buffer[1] = payload[cursor + 2];
	buffer[2] = payload[cursor + 1];
	buffer[3] = payload[cursor + 0];
	buffer[4] = payload[cursor + 7];
	buffer[5] = payload[cursor + 6];
	buffer[6] = payload[cursor + 5];
	buffer[7] = payload[cursor + 4];
	cursor += 8;
	return *p2value;
}

/**getInt3 gets three bytes starting at cursor which are interpreted as a 24 bits signed integer.
 * Integers are stored in the payload with the most significant byte first.
 * The cursor is incremented by three
 *
 * @return the integer value of the four bytes starting at cursor 
 * @throws the integer value 8 when it is intended to get data after the end of payload 
 */
int OSPMessage::getInt3() {
	if (cursor+2 >= payloadLength) throw 8;
	unsigned int uvalue = 
		payload[cursor] << 16 |
		payload[cursor+1] << 8 |
		payload[cursor+2];
	cursor += 3;
	return getTwosComplement(uvalue, 24);
}
