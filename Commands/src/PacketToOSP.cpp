/** @file PacketToOSP.cpp
 * Contains the command line program to extract from a binary file containing SiRF receiver message packets their payload data, and store them into an OSP binary file.
 *<p>
 *Usage:
 *<p>PacketToOSP.exe {options} [PacketsFilename]
 *<p>Options are:
 *	- -f BFILE or --binfile=BFILE : OSP binary output file. Default value BFILE = DATA.OSP
 *	- -h or --help : Show usage data. Default value HELP=FALSE
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *Default value for operator is: RXMESSAGES.PKT
 *
 *Copyright 2016 Francisco Cancillo
 *<p>
 *This file is part of the RXtoRINEX tool.
 *<p>
 *RXtoRINEX is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 *as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *RXtoRINEX is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *A copy of the GNU General Public License can be found at <http://www.gnu.org/licenses/>.
 *
 *Ver.	|Date	|Reason for change
 *------+-------+------------------
 *V1.0	|2/2016	|First release
 */

//from CommonClasses
#include "ArgParser.h"
#include "Logger.h"
#include "Utilities.h"

#include <stdio.h>

using namespace std;

//@cond DUMMY
#define START1 160	///<0xA0	OSP messages from/to receiver are preceded by the synchro sequence of two bytes with values START1, START2
#define START2 162	///<0xA2
#define END1 176	//0XB0	OSP messages from/to receiver are followed by the end sequence of two bytes with values END1, END2
#define END2 179	//0XB3

///Functions defined here
int filterPkts(Logger* plog);
bool synchOSPmsg(FILE* inFile);
int readOSPmsg(FILE* inFile);

///The maximum size in bytes of any message payload
#define MAXPAYLOADSIZE 2048
unsigned char payloadBuf[MAXPAYLOADSIZE];	//buffer for the OSP message payload
unsigned char payloadLnBuf[2];				//buffer for the OSP message payload length
unsigned int payloadLength;					//the payload length in bytes of current message

///The command line format
const string CMDLINE = "PacketToOSP.exe {options} [PacketsFilename]";
const string MYVER = " V1.0";
///The parser object to store options and operators passed in the comman line
ArgParser parser;
//Metavariables for options
int BFILE, HELP, LOGLEVEL;
//Metavariables for operators
int PKTF;
//@endcond 
//functions in this file

/**main
 * gets the command line arguments, set parameters accordingly and converts data.
 *<p>
 * From the input data file containing receiver message packets, the command extracts message packets, verifies them,
 * and writes the payload data of the correct ones to the OSP binary file. See SiRF IV ICD for details on receiver messages.
 *<p>
 * The binary OSP output files containt messages where head, check and tail have been removed, that is, the data for each
 * message consists of the two bytes of the payload length and the payload bytes.
 *
 *@param argc the number of arguments passed from the command line
 *@param argv the array of arguments passed from the command line
 *@return  the exit status according to the following values and meaning::
 *		- (0) no errors have been detected
 *		- (1) an error has been detected in arguments
 *		- (2) error when opening the input file
 *		- (3) error has occurred when creating the binary output OSP file
 *		- (4) error has occurred when reading packet data
 *		- (5) error has occurred when writing data message data
 */
int main(int argc, char** argv) {
	/**The main process sequence follows:*/
	/// 1- Defines and sets the error logger object
	Logger log("LogFile.txt", string(), string(argv[0]) + MYVER + string(" START"));
	/// 2- Setups the valid options in the command line. They will be used by the argument/option parser
	LOGLEVEL = parser.addOption("-l", "--llevel", "LOGLEVEL", "Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)", "INFO");
	HELP = parser.addOption("-h", "--help", "HELP", "Show usage data", false);
	BFILE = parser.addOption("-f", "--binfile", "BFILE", "OSP binary output file", "DATA.OSP");
	/// 3- Setups the default values for operators in the command line
	PKTF = parser.addOperator("RXMESSAGES.PKT");
	/// 4- Parses arguments in the command line extracting options and operators
	try {
		parser.parseArgs(argc, argv);
	}  catch (string error) {
		parser.usage("Argument error: " + error, CMDLINE);
		log.severe(error);
		return 1;
	}
	log.info(parser.showOptValues());
	log.info(parser.showOpeValues());
	if (parser.getBoolOpt(HELP)) {	//help info has been requested
		parser.usage("Captures OSP message data from a SiRF IV receiver and stores them in a OSP binary file", CMDLINE);
		return 0;
	}
	/// 5- Sets logging level stated in option
	log.setLevel(parser.getStrOpt(LOGLEVEL));
	/// 6- Filter input binary receiver packets generating output OSP messages
	return filterPkts(&log);
}
/**filterPkts
 * read receiver message packets from the input file, verify them and extract payload data binary which are written into the binary OSP file.
 * 
 *@param plog a pointer to a logger object
 *@return 0 if no error occurred when reading or writting, the related error code otherwise 
 */
int filterPkts(Logger* plog) {
	/// 6.1- Opens the message s binary input fiILE* inFile;
	FILE* inFile;
	int anInt;
	string logMsg;
	int nMsgWrite = 0;
	int nPkt = 0;
	string fileName = parser.getOperator(PKTF);
	if ((inFile = fopen(fileName.c_str(), "rb")) == NULL) {
		plog->severe("Cannot open file " + fileName);
		return 2;
	}
	/// 6.2- Creates the output binary file
	FILE *outFile;
	fileName = parser.getStrOpt(BFILE);
	if ((outFile = fopen(fileName.c_str(), "wb")) == NULL) {
		plog->severe("Cannot create the binary output file " + string(fileName));
		return 3;
	}
	/// 6.3- Reads packets from the input stream until end of file happen 
	while (synchOSPmsg(inFile)) {
		nPkt++;
		anInt = readOSPmsg(inFile);
		logMsg = "Packet " + to_string((long long) nPkt) + " OSP <" + to_string((long long) payloadBuf[0]) + "," + to_string((long long) payloadLength) + "> ";
		switch (anInt) {
		case 0:	//packet is correct. Update counters and write message to OSP file
			nMsgWrite++;
			if ((fwrite(payloadLnBuf, 1, 2, outFile) != 2) ||
				(fwrite(payloadBuf, 1, payloadLength, outFile) != payloadLength)) {
				plog->severe(logMsg + "Write error in message " + to_string((long long) nMsgWrite));
				return 5;
			}
			plog->finest(logMsg + "to msg " + to_string((long long) nMsgWrite));
			break;
		case 1:
			plog->warning(logMsg + "Error in checksum");
			break;
		case 2:
			plog->warning(logMsg + "Error reading payload");
			break;
		case 3:
			plog->warning(logMsg + "Error length too big");
			break;
		case 4:
			plog->warning(logMsg + "Error reading payload length");
			break;
		default:
			break;
		}
	}
	plog->info("Packets read:" + to_string((long long) nPkt) + " Messages written:" + to_string((long long) nMsgWrite));
	return 0;
}

/**synchroOSPmsg skips bytes from input until start of OSP message is reached.
 * Note that start of OSP message is preceded by the sequence of the two bytes START1 (A0) START2 (A2)
 *
 *@param inFile the input binary FILE containing OSP packets 
 *@return true if the sequence START1 START2 has been detected, false when EOF has been reached
 */
bool synchOSPmsg(FILE* inFile) {
	unsigned int inData = 0;
	//A state machine automata is used to skip bytes from input until START1 START2 appears
	//States: 1=is waiting to START1; 2=is waiting for STAR2; 3=START1+START2 detected
	int state = 1;
	while (state!=3) {
		if (fread(&inData, 1, 1, inFile) == 1) {
			switch (state) {
			case 1:
				switch (inData) {
				case START1:	state = 2; break;
				case START2:	break;
				default:		break;
				}
			case 2:
				switch (inData) {
				case START1:	break;
				case START2:	state = 3; break;
				default:		state = 1; break;
				}
			}
		}
		else return false;
	}
	return true;
}

/**readOSPmsg reads a OSP message from the input file and put ist payload data into a buffer.
 *
 *@param inFile the input binary FILE containing OSP packets 
 *@return the exit status according to the following values and meaning:
 *		- (0) when a correct formatted OSP message has been received;
 *		- (1) if the message has incorrect checksum;
 *		- (2) if error occurred when reading payload or not enought bytes were received
 *		- (3) if the payload length read is out of margin (>MAXBUFFERSIZE)
 *		- (4) if unable to read the two bytes of the OSP payload length 
 */
int readOSPmsg(FILE* inFile) {
	payloadLength = 0;
	if (fread(payloadLnBuf, 1, 2, inFile) != 2) {
		return 4;
	}
	payloadLength = (payloadLnBuf[0] << 8) | payloadLnBuf[1];	//numbers in msg are big endians
	if (!((payloadLength > 0) && (payloadLength < MAXPAYLOADSIZE-2))) {
		return 3;
	}
	//read payload data plus checkum (2 bytes)
	if (fread(payloadBuf, 1, payloadLength + 2, inFile) != (payloadLength + 2)) {
		return 2;
	}
	//compute checksum of payload contents
	unsigned int computedCheck = payloadBuf[0];
	for (unsigned int i=1; i<payloadLength; i++) {
		computedCheck += payloadBuf[i];
		computedCheck &= 0x7FFF;
	}
	//get checksum received after message payload
	unsigned int messageCheck = (payloadBuf[payloadLength] << 8) | payloadBuf[payloadLength+1];
	if (computedCheck != messageCheck) {
		return 1;	//checksum does not match!
	}
	return 0;
}

