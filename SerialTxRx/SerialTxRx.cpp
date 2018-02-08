/** @file SerialTxRx.cpp
 * Contains the implementation of the SerialTxRx class used to send/receive OSP message data from SiRF IV
 * receivers through a serial port using Windows resources.
 */

#include "SerialTxRx.h"

#include <vector>
#include <sstream>

CBRrate::CBRrate(int r, DWORD CBRr) {
	baudR = r;
	DCBbaud = CBRr;
}

/**Constructs an empty SerialTxRx object, without setting comms port parameters.
  */
SerialTxRx::SerialTxRx(void) {
	payloadLen = 0;
	addCBRrate (110, CBR_110);
	addCBRrate (300, CBR_300);
	addCBRrate (600, CBR_600);
	addCBRrate (1200, CBR_1200);
	addCBRrate (2400, CBR_2400);
	addCBRrate (4800, CBR_4800);
	addCBRrate (9600, CBR_9600);
	addCBRrate (14400, CBR_14400);
	addCBRrate (19200, CBR_19200);
	addCBRrate (38400, CBR_38400);
	addCBRrate (57600, CBR_57600);
	addCBRrate (115200, CBR_115200);
	addCBRrate (128000, CBR_128000);
	addCBRrate (256000, CBR_256000);
}

/**Destructs SerialTxRx objects.
 */
SerialTxRx::~SerialTxRx(void) {
	CBRrateLst.clear();
}

/**addCBRrate adds a Windous CBR identifier and it related baud rate to the list of rates the port can use
 *
 * @param rate the baud rate
 * @param CBRrt the Windows rate identifier
 */
void SerialTxRx::addCBRrate (int rate, DWORD CBRrt) {
	CBRrateLst.emplace_front(CBRrate(rate, CBRrt));
}

/**getCBRrate gets the Windous CBR identifier associated to a given baud rate
 *
 * @param rate the baud rate
 * @return the Windows rate identifier
 * @trow error string message if the given baud rate is not defined for this port 
 */
DWORD SerialTxRx::getCBRrate (int rate) {
	string error = MSG_UnkBaudR;
	forward_list<CBRrate>::iterator iterator;
	for (iterator = CBRrateLst.begin(); iterator != CBRrateLst.end(); iterator++) {
		if (iterator->baudR == rate) return iterator->DCBbaud;
	}
	throw error;
}

/**getBaudRate gets the baud rate associated to a give the Windous CBR identifier
 *
 * @param r the the Windows rate identifier
 * @return the baud rate
 * @trow error string message if the given rate identifier is not defined for this port 
 */
int SerialTxRx::getBaudRate (DWORD r) {
	string error = MSG_UnkBaudR;
	forward_list<CBRrate>::iterator iterator;
	for (iterator = CBRrateLst.begin(); iterator != CBRrateLst.end(); iterator++) {
		if (iterator->DCBbaud == r) return iterator->baudR;
	}
	throw error;
}

/**openPort opens the serial port portName to allow reception of messages or sending commands to a SiRF receiver.
 *
 * @param portName the name of the port to be opened
 * @throw error message when the port cannot be open, explaining the reason 
 */
void SerialTxRx::openPort(string portName) {
	string error;
	if(portName.length() > 5) {
		error =  portName + MSG_PortNameTooLong;
		throw error;
	}
	portName = "\\\\.\\" + portName;
	wchar_t wcompletePortName[200];
	int e = MultiByteToWideChar(CP_ACP, 0, portName.c_str(), -1, wcompletePortName, 200);
	hSerial = CreateFile(wcompletePortName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL);
	if (hSerial == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			error = portName + MSG_PortNotExist;
		} else {
			error = MSG_OpenError + portName;
		}
		throw error;
	}
}

/**setPortParams sets port baud rate.
 * Other relevant port parameters are set to the following values:
 *	- ByteSize = 8
 *	- StopBits = ONESTOPBIT;
 *	- Parity = NOPARITY;
 *	- fBinary = TRUE;
 *	- fDtrControl = DTR_CONTROL_DISABLE;
 *	- fRtsControl = RTS_CONTROL_DISABLE;
 *
 * @param baudRate the value of the baud rate to be set
 * @throw error string with the message explaining it
 */
void SerialTxRx::setPortParams(int baudRate) {
	COMMCONFIG dcbSerialParams;
	COMMTIMEOUTS timeouts;
	DWORD CBRbaudRate;
	string error;
	//give time to drain any bytes could exists in output buffers
	Sleep(100);
	//get the current state in dcb
	if (!GetCommState(hSerial, &dcbSerialParams.dcb)) {
		error = MSG_InitState;
		throw error;
	}
	try {
		CBRbaudRate = getCBRrate(baudRate);
	} catch (string error) {
		throw error;
	}
	//update dcb data as requested
	dcbSerialParams.dcb.DCBlength = sizeof(dcbSerialParams.dcb);
	dcbSerialParams.dcb.BaudRate = CBRbaudRate;
	dcbSerialParams.dcb.ByteSize = 8;
	dcbSerialParams.dcb.StopBits = ONESTOPBIT;
	dcbSerialParams.dcb.Parity = NOPARITY;
	dcbSerialParams.dcb.fBinary = TRUE;
	dcbSerialParams.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcbSerialParams.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcbSerialParams.dcb.fOutxCtsFlow = FALSE;
	dcbSerialParams.dcb.fOutxDsrFlow = FALSE;
	dcbSerialParams.dcb.fDsrSensitivity= FALSE;
	dcbSerialParams.dcb.fAbortOnError = TRUE;
	//set new state
	if (!SetCommState(hSerial, &dcbSerialParams.dcb)) {
		error = MSG_SetState;
		throw error;
	}
	GetCommTimeouts(hSerial,&timeouts);
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier= 10;
	if(!SetCommTimeouts(hSerial, &timeouts)) {
		error = MSG_SetTineout;
		throw error;
	}
	//get any garbage could exist: data got before change or transient bytes after change 
	DWORD nBytesRead;
	ReadFile(hSerial, payBuff, MAXBUFFERSIZE, &nBytesRead, NULL);
}

/**getPortParams gets current port parameters: baud rate, byte size and parity.
 *
 * @param baudRate a variable to place the value of the baud rate
 * @param size a variable to place the value of the transmission character size
 * @param parity a variable to place a true value if parity is used, or false if not used
 * @throw error string with the message explaining it
 */
void SerialTxRx::getPortParams(int& baudRate, int& size, bool& parity) {
	COMMCONFIG dcbSerialParams;
	string error;
	if (!GetCommState(hSerial, &dcbSerialParams.dcb)) {
		error = MSG_InitState;
		throw error;
	}
	try {
		baudRate = getBaudRate(dcbSerialParams.dcb.BaudRate);
	} catch (string error) {
		throw error;
	}
	size = dcbSerialParams.dcb.ByteSize;
	if (dcbSerialParams.dcb.Parity == NOPARITY) parity = false;
	else parity = true;
}

/**closePort closes the currently open serial port.
 */
void SerialTxRx::closePort() {
	CloseHandle(hSerial);
}

/**synchroOSPmsg skips bytes from input until start of OSP message is reached.
 * Note that start of OSP message is preceded by the sequence of the two bytes START1 (A0) START2 (A2)
 *
 * @param patience is the maximum number of bytes to skip or unsuccessful reads from the serial port before returning a false value
 * @return true if the sequence START1 START2 has been detected, false otherwise
 */
bool SerialTxRx::synchOSPmsg(int patience) {
	DWORD nBytesRead;
	unsigned int inData = 0;
	//A state machine automata is used to skip bytes from input until START1 START2 appears
	//States: 1=is waiting to START1; 2=is waiting for STAR2; 3=START1+START2 detected
	int state = 1;
	while (state!=3 && patience>0) {
		if (ReadFile(hSerial, &inData, 1, &nBytesRead, NULL) && (nBytesRead == 1)) {
			switch (state) {
			case 1:
				switch (inData) {
				case START1:	state = 2; break;
				case START2:	break;
				default:		patience--; break;
				}
			case 2:
				switch (inData) {
				case START1:	break;
				case START2:	state = 3; break;
				default:		state = 1; patience--; break;
				}
			}
		} else {
			patience--;
		}
	}
	DBGRPT("synchOSPmsg:state=%d;patience=%d\n", state, patience)
	return state == 3;
}

/**readOSPmsg reads a OSP message from the serial port.
 *
 * @param patience is the maximum number of bytes to skip or unsuccessful reads from the serial port before returning error
 * @return the exit status according to the following values and meaning:
 *		- (0) when a correct formatted OSP message has been received;
 *		- (1) if the message has incorrect checksum;
 *		- (2) if error occurred when reading payload or not enought bytes were received
 *		- (3) if the payload length read is out of margin (>MAXBUFFERSIZE)
 *		- (4) if unable to read the two bytes of the OSP payload length 
 *		- (5) if read error occurred reading OSP payload bytes
 *		- (6) if OSP start bytes have not been received before "exhaust patience"
 */
int SerialTxRx::readOSPmsg(int patience) {
	DWORD nBytesRead = 0;
	unsigned int computedCheck = 0;
	unsigned int messageCheck = 0;
	//skip bytes until beginning of a message
	if (!synchOSPmsg(patience)) return 6;
	//read payload length field (2 bytes)
	DBGRPT("readOSPmsg:")
	if (!ReadFile(hSerial, paylenBuff, 2, &nBytesRead, NULL)) {
		DBGRPT("error 5\n")
		return 5;
	}
	if (nBytesRead != 2) {
		DBGRPT("error 4\n")
		return 4;
	}
	payloadLen = (paylenBuff[0] << 8) | paylenBuff[1];	//numbers in msg are big endians
	DBGRPT("pllen=%d;", payloadLen)
	if (!((payloadLen > 0) && (payloadLen < MAXBUFFERSIZE-1-2))) {
		DBGRPT("error 3\n")
		return 3;
	}
	//read payload data plus checkum (2 bytes)
	if (!(ReadFile(hSerial, payBuff, payloadLen+2, &nBytesRead, NULL) && (nBytesRead == payloadLen+2))) {
		DBGRPT("error 2\n")
		return 2;
	}
	DBGRPT("pl=")
	#if defined (_DEBUG)
	for(int i=0; i<(int) nBytesRead; i++) DBGRPT ("%02X ", (unsigned int) payBuff[i])
	#endif
	DBGRPT(" (%d);", nBytesRead)
	//compute checksum of payload contents
	computedCheck = payBuff[0];
	for (unsigned int i=1; i<payloadLen; i++) {
		computedCheck += payBuff[i];
		computedCheck &= 0x7FFF;
	}
	//get checksum received after message payload
	messageCheck = (payBuff[payloadLen] << 8) | payBuff[payloadLen+1];
	if (computedCheck != messageCheck) {
		DBGRPT("error 1\n")
		return 1;	//checksum does not match!
	}
	DBGRPT("End OK\n")
	return 0;
}

/**writeOSPcmd builds a OSP message command and sends it to receiver through the serial port.
 *
 * @param mid the message identification of the OSP command to be generated
 * @param cmdArgs the rest of message payload, as a whitespace-separated sequence of bytes
 * @param base the base used to write the parameters (16, 10, ...)
 * @throw error string when message cannot be send
 */
void SerialTxRx::writeOSPcmd(int mid, string cmdArgs, int base) {
	//extract command arguments in cmrArgs: bytes in hexadecimal separated by blanks
	stringstream ss(cmdArgs);	// Insert the string with payload data into a stream
	vector<string> tokens;		//create a vector for command arguments
	string strBuf;
	DBGRPT("writeOSPmsg:");
	while (ss >> strBuf)		//extract arguments: each one is a token in the string
		tokens.push_back(strBuf);
	unsigned int payloadLen = 1 + tokens.size();
	if ((2 + 2 + 1 + payloadLen + 2 + 2) > MAXBUFFERSIZE) {
		string error = "Error OSP cmd too long = " + to_string((long long) payloadLen);
		DBGRPT(error.c_str());
		throw error;
	}
	//start filling the message buffer (pyload buffer used here for that) with command data
	unsigned int bufferIndex = 0;
	payBuff[bufferIndex++] = START1;
	payBuff[bufferIndex++] = START2;
	payBuff[bufferIndex++] = (unsigned char) (payloadLen >> 8);
	payBuff[bufferIndex++] = (unsigned char) (payloadLen & 0xFF);
	payBuff[bufferIndex++] = (unsigned char) mid;
	unsigned long ul;
	for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); it++) {
		ul = stoul(*it, nullptr, base);
		payBuff[bufferIndex++] = (unsigned char) ( ul & 0xFF);
	}
	//compute checksum and put its value in the buffer
	unsigned int computedCheck = payBuff[4];
	for (unsigned int i=5; i<bufferIndex; i++) {
		computedCheck += payBuff[i];
		computedCheck &= 0x7FFF;
	}
	payBuff[bufferIndex++] = (unsigned char) (computedCheck >> 8);
	payBuff[bufferIndex++] = (unsigned char) (computedCheck & 0xFF);
	//append end sequence
	payBuff[bufferIndex++] = END1;
	payBuff[bufferIndex++] = END2;
	//write the message to the output stream
	DWORD nBytesWritten = 0;
	WriteFile(hSerial, payBuff, bufferIndex, &nBytesWritten, NULL);
	DBGRPT("pllen=%d;msg=",payloadLen);
	#if defined (_DEBUG)
	for(unsigned int i=0; i<bufferIndex; i++) DBGRPT("%02X ", (unsigned int) payBuff[i]);
	#endif
	DBGRPT(" (%d)\n", (int) nBytesWritten)
	if (bufferIndex != nBytesWritten) {
		string error = "Error sending OSP cmd " + to_string((long long) payBuff[0]);
		DBGRPT(error.c_str());
		throw error;
	}
}

/**synchroNMEAmsg skips bytes until start of NMEA message is reached.
 *The NMEA message is preceded by the sequence <LineFeed><$> in the input stream of ASCII chars.
 *
 * @param patience the maximum number of skipped bytes or unsuccessful reads before returning 
 * @return true if the sequence <LineFeed>$ has been detected in the ASCII input sequence, false otherwise
 */
bool SerialTxRx::synchNMEAmsg(int patience) {
	DWORD nBytesRead;
	unsigned int inData = 0;
	//A state machine automata is used to skip bytes from input until <LineFeed><Dolar> appears
	//States: 1=is waiting to <LineFeed>; 2=is waiting for <$>; 3=<LineFeed><$> detected
	int state = 1;
	while (state!=3 && patience>0) {
		if (ReadFile(hSerial, &inData, 1, &nBytesRead, NULL) && (nBytesRead == 1)) {
			switch (state) {
			case 1:
				switch (inData) {
				case LF:	state = 2; break;
				case DOLAR:	break;
				default:	patience--; break;
				}
			case 2:
				switch (inData) {
				case LF:	break;
				case DOLAR:	state = 3; break;
				default:	state = 1; patience--; break;
				}
			}
		} else {
			patience--;
		}
	}
	DBGRPT("synchNMEA:state=%d;patience=%d\n", state, patience);
	return state == 3;
}

/**readNMEAmsg reads a NMEA message from the serial port.
 *
 * @param patience the maximum number of skipped chars or unsuccessful reads before returning 
 * @return the exit status according to the following values and meaning:
 *		- (0) when a correct message has been received;
 *		- (1) if the received message has incorrect checksum;
 *		- (2) if message received has less than five chars. Minimum NMEA message is $XXX*SS
 *		- (3) if the input stream does not contains NMEA messages or an input error occurred or EOF
 *		- (4) if NMEA start bytes have not been received before "exhaust patience"
 */
int SerialTxRx::readNMEAmsg(int patience) {
	DWORD nBytesRead = 0;
	unsigned int computedCheck = 0;
	unsigned int messageCheck = 0;
	int returnValue = 3;
	payloadLen = 0;
	//skip bytes until beginning of a message found (<LF><DOLLAR>)
	if (!synchNMEAmsg(patience)) return 4;
	//read NMEA message bytes (up to CR) and put them into payBuff
	DBGRPT("readNMEAmsg:")
	while (ReadFile(hSerial, (payBuff+payloadLen), 1, &nBytesRead, NULL) && (nBytesRead == 1)) {
		if (*(payBuff+payloadLen) == CR) {	//is the last char in a NMEA message
			*(payBuff+payloadLen) = 0;	//convert chars received to a C-string
			if (payloadLen < 5) {		//minimum NMEA message is $XXX*SS<CR>
				returnValue = 2;
				break;
			}
			payloadLen -= 3;	//last three bytes are the checksum: *SS
			*(payBuff+payloadLen) = 0;	//mark end of message
			returnValue = 0;
			break;
		}
		if (payloadLen < MAXBUFFERSIZE-1) payloadLen++;
		else break;
	}
	DBGRPT("pllen=%d", payloadLen)
	if (returnValue == 0) {	//a NMEA message has been receive
		//compute and verfy its checksum
		DBGRPT(";msg=%s", payBuff)
		computedCheck = payBuff[0];
		for (unsigned int i=1; i<payloadLen; i++) computedCheck ^= payBuff[i];
		//get checksum received after message payload
		sscanf((char *) (payBuff+payloadLen+1), "%x", &messageCheck);
		if (computedCheck != messageCheck) returnValue = 1;	//checksum does not match
	}
	DBGRPT(";return=%d\n", returnValue)
	return returnValue;
}

/**writeNMEAcmd builds a NMEA message command and sends it to receiver through to the serial port.
 *
 * @param mid the identification of the command to be generated in the form $PSRF<mid>, ...
 * @param cmdArgs the message parameters: a comma separated list of arguments
 * @throw  error string message explaining it
 */
void SerialTxRx::writeNMEAcmd(int mid, string cmdArgs) {
	DWORD nBytesWritten = 0;
	char checksumBuff[10];
	//init buffer with command header data and append arguments
	sprintf((char*) payBuff, "$PSRF%3d,", mid);
	strncat((char*) payBuff, cmdArgs.c_str(), MAXBUFFERSIZE);
	//compute checksum and append its value
	int plLen = strlen((char*) payBuff);
	unsigned int computedCheck = payBuff[1];
	for (int i=2; i<plLen; i++) computedCheck ^= (unsigned int) payBuff[i];
	sprintf(checksumBuff, "*%02X\r\n", computedCheck);
	strncat((char*) payBuff, checksumBuff, MAXBUFFERSIZE);
	//send command
	plLen = strlen((char*) payBuff);
	WriteFile(hSerial, payBuff, plLen, &nBytesWritten, NULL);
	DBGRPT("writeNMEAmsg:(%d)=%s",nBytesWritten, payBuff)
	if (nBytesWritten != plLen) {
		throw "Error sending NMEA $PSRF"
				+ to_string((long long) mid)
				+ "," + cmdArgs;
	}
}
