/** @file RXtoOSP.cpp
 * Contains the command line program to capture OSP message data from a SiRF IV receiver and stores them in a OSP binary file.
 *<p>
 *Usage:
 *<p>OSPDataLogger.exe {options}
 *<p>Options are:
 *	- -a PAT or --patience=PAT : Maximum number of bytes to read when waiting for a packet start (0xA0 0xA3). Default value PAT = 2500
 *	- -b BAUD or --baud=BAUD : Set serial port baud rate. Default value BAUD = 57600
 *	- -d DURATION or --duration=DURATION : Duration of acquisition period, in minutes. Default value DURATION = 5
 *	- -e or --ephemeris : Capture GPS ephemeris data (MID15). Default value EPHEM=TRUE
 *	- -f BFILE or --binfile=BFILE : OSP binary output file. Default value BFILE = 20150126_205513.OSP
 *	- -g or --GPS50bps : Capture GPS 50bps nav message (MID8). Default value G50BPS=FALSE
 *	- -h or --help : Show usage data. Default value HELP=FALSE
 *	- -i OBSINT or --interval=OBSINT : Observation interval (in seconds) for epoch data. Default value OBSINT = 5
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *	- -p COMPORT or --port=COMPORT : Serial port name where receiver is connected. Default value COMPORT = COM35
 *	- -s MID or --stop=MID : Stop epoch data acquisition when this MID (Message ID) arrives. Default value MID = 7
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
 *A copy of the GNU General Public License can be found at <http://www.gnu.org/licenses/>.
 *
 *Ver.	|Date	|Reason for change
 *------+-------+------------------
 *V1.0	|2/2015	|First release
 *V2.0	|2/2016	|Improve logging
				|Add commands for SiRFV
 */

//from CommonClasses
#include "ArgParser.h"
#include "Logger.h"
#include "Utilities.h"
//from SerialTxRx
#include "SerialTxRx.h"
//standard
#include <stdio.h>

using namespace std;

//@cond DUMMY
///The command line format
const string CMDLINE = "OSPDataLogger.exe {options}";
const string MYVER = " V2.0";
///The parser object to store options and operators passed in the comman line
ArgParser parser;
//Metavariables for options
int BAUD, DURATION, BFILE, G50BPS, HELP, EPHEM, OBSINT, LOGLEVEL, COMPORT, MID, PAT;

struct MSGwrite {
	int msgId;
	string payload;
	int base;
	string comment;

	MSGwrite(int id, string pld, int bs, string exp) {
		msgId = id;
		payload = pld;
		base = bs;
		comment = exp;
	}
};
vector <MSGwrite> lstWmsg;
//@endcond 
//functions in this file
int acquireBin(SerialTxRx, FILE*, int, int, int, Logger*);

/**main
 * gets the command line arguments, set parameters accordingly and triggers the data acquisition from the receiver.
 * The SiRFIV GPS receiver shall be connected to a serial port and ready to receive and send messages.
 * To be ready for this command, the receiver and computer shall be synchronized at the same baud rate, and the
 * receiver sending / accepting OSP messages.
 *<p>
 * This command sends messages to the receiver to state the data flow with the messages and rates needed to generate
 *  OSP files that could be used for the further extraction of data used in RINEX and RTK files.
 *<p>
 * From the input receiver data stream, the command extracts the messages requested, verifies then, and writes the
 * correct ones to the OSP binary file. See SiRF IV ICD for details on receiver messages.
 *<p>
 * The binary OSP output files containt messages where head, check and tail have been removed, that is, the data for each
 * message consists of the two bytes of the payload length and the payload bytes.
 *<p>
 * The SynchroRX command line provided in this project can be used to check and set the receiver state: baud rate,
 * accept/send OSP or NMEA messages, etc.
 * Other utilities provided by receiver manufacturers exist that could perform this synchro task.
 *
 *@param argc the number of arguments passed from the command line
 *@param argv the array of arguments passed from the command line
 *@return  the exit status according to the following values and meaning:
 *		- (0) no errors have been detected
 *		- (1) an error has been detected in arguments
 *		- (2) error when opening and setting the communication port
 *		- (3) the receiver is not sending OSP messages
 *		- (4) error has occurred when setting receiver
 *		- (5) error has occurred when creating the binary output OSP file
 *		- (6) error has occurred when writing data read from receiver
 */

int main(int argc, char** argv) {
	/**The main process sequence follows:*/
	/// 1- Defines and sets the error logger object
	Logger log("LogFile.txt", string(), string(argv[0]) + MYVER + string(" START"));
	/// 2- Setups the valid options in the command line. They will be used by the argument/option parser
	// Gets local time for naming the acquisition file (yymmdd_hhmmss.OSP)
	time_t rawtime;
	struct tm * timeinfo;
	char fileName[80];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (fileName, sizeof fileName,"%Y%m%d_%H%M%S.OSP", timeinfo);
	MID = parser.addOption("-s", "--stop", "MID", "Stop epoch data acquisition when this MID (Message ID) arrives", "7");
	COMPORT = parser.addOption("-p", "--port", "COMPORT", "Serial port name where receiver is connected", "COM35");
	LOGLEVEL = parser.addOption("-l", "--llevel", "LOGLEVEL", "Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)", "INFO");
	OBSINT = parser.addOption("-i", "--interval", "OBSINT", "Observation interval (in seconds) for epoch data", "5");
	HELP = parser.addOption("-h", "--help", "HELP", "Show usage data", false);
	G50BPS = parser.addOption("-g", "--G50bps", "G50BPS", "Request 50bps nav messages (MID8)", false);
	BFILE = parser.addOption("-f", "--binfile", "BFILE", "OSP binary output file", fileName);
	EPHEM = parser.addOption("-e", "--ephemeris", "EPHEM", "Request ephemeris data (MID15, MID70)", true);
	DURATION = parser.addOption("-d", "--duration", "DURATION", "Duration of acquisition period, in minutes", "5");
	BAUD = parser.addOption("-b", "--baud", "BAUD", "Set serial port baud rate", "57600");
	PAT = parser.addOption("-a", "--patience", "PAT", "Maximum number of bytes to read when waiting for a packet start", "2500");
	/// 3- Parses arguments in the command line extracting options and operators
	try {
		parser.parseArgs(argc, argv);
	}  catch (string error) {
		parser.usage("Argument error: " + error, CMDLINE);
		log.severe(error);
		return 1;
	}
	log.info(parser.showOptValues());
	if (parser.getBoolOpt(HELP)) {	//help info has been requested
		parser.usage("captures OSP message data from a SiRF IV receiver and stores them in a OSP binary file", CMDLINE);
		return 0;
	}
	/// 4- Sets logging level stated in option
	log.setLevel(parser.getStrOpt(LOGLEVEL));
	/// 5- Computes observation interval and number of epochs to read from data given in options
	int obsIntl = stoi(parser.getStrOpt(OBSINT));
	int nEpochs = stoi(parser.getStrOpt(DURATION)) * 60 / obsIntl;
	int patience = stoi(parser.getStrOpt(PAT));
	/// 6- Defines and sets up the SerialTxRx object to be used for communication with the receiver
	SerialTxRx port;
	try {
		port.openPort(parser.getStrOpt(COMPORT));
		port.setPortParams(stoi(parser.getStrOpt(BAUD)));
	} catch (string error) {
		log.severe(error);
		return 2;
	}
	/// 7- Verifies that receiver mode is OSP
	switch (port.readOSPmsg(patience)) {
	case 0:	//OSP message is OK
		break;
	case 1:	//Error in OSP message. May be recoverable
	case 2:
	case 3:
	case 4:
	case 5:
		log.warning("The receiver is sending erroneous OSP messages");
		break;
	case 6:
	default:
		log.severe("Error: the receiver is not sending OSP messages");
		return 3;
	}
	/// 8- Sends OSP commands to the communication port to perform receiver setup
	//build vector with sequence of messages to send
	lstWmsg.push_back(MSGwrite(166, "02 00 " + to_string((long long) obsIntl) + " 00 00 00 00", 10, "Enable all massesges at the interval stated"));
	lstWmsg.push_back(MSGwrite(166, "04 00 00 00 00 00 00", 16, "Disable debug msgs"));
	lstWmsg.push_back(MSGwrite(166, "00 1D 00 00 00 00 00", 16, "Disable navigation debug message 29")); 
	lstWmsg.push_back(MSGwrite(166, "00 1E 00 00 00 00 00", 16, "Disable navigation debug message 30"));
	lstWmsg.push_back(MSGwrite(166, "00 1F 00 00 00 00 00", 16, "Disable navigation debug message 31"));
	lstWmsg.push_back(MSGwrite(166, "00 04 00 00 00 00 00", 16, "Disable message 4 navigation"));
	if (!parser.getBoolOpt(G50BPS))
		lstWmsg.push_back(MSGwrite(166, "00 08 00 00 00 00 00", 16, "Disable message 8 50 BPD data"));
	lstWmsg.push_back(MSGwrite(166, "00 40 00 00 00 00 00", 16, "Disable message 64 aux measurements data"));
	lstWmsg.push_back(MSGwrite(166, "00 32 00 00 00 00 00", 16, "Disable message 50 SBAS stat"));
	lstWmsg.push_back(MSGwrite(166, "00 29 00 00 00 00 00", 16, "Disable message 41 Geodetic nav"));
	//lstWmsg.push_back(MSGwrite(133, "01 00 00 00 00 00", 16, "Set DGPS source to SBAS"));
	//Commands to request specific messages with data needed for RINEX files
	lstWmsg.push_back(MSGwrite(132, "00", 16, "Poll Software Version. Answer in MID6"));
	lstWmsg.push_back(MSGwrite(152, "00", 16, "Poll Navigation parameters. Answer in MID19"));
	if (parser.getBoolOpt(EPHEM)) {
		lstWmsg.push_back(MSGwrite(147, "00 00", 16, "Poll ephemeris. Answer in MID15"));
		lstWmsg.push_back(MSGwrite(147, "00 00", 16, "Poll ephemeris. Answer in MID15"));
		lstWmsg.push_back(MSGwrite(147, "00 00", 16, "Poll ephemeris. Answer in MID15"));
		lstWmsg.push_back(MSGwrite(212, "0C", 16, "In SiRFV: GLONASS Broadcast Ephemeris Request SID12. Answer in MID70 SID12"));
		lstWmsg.push_back(MSGwrite(212, "0C", 16, "In SiRFV: GLONASS Broadcast Ephemeris Request SID12. Answer in MID70 SID12"));
		lstWmsg.push_back(MSGwrite(212, "0C", 16, "In SiRFV: GLONASS Broadcast Ephemeris Request SID12. Answer in MID70 SID12"));
		//lstWmsg.push_back(MSGwrite(232, "02 FF FF", 16, "Poll ephemeris status SID2. Answer in MID56 SID3"));
	}
	for (vector<MSGwrite>::iterator it = lstWmsg.begin() ; it != lstWmsg.end(); ++it) {
		try {
			log.info("W OSP<" + to_string((long long) it->msgId) + "> b" + to_string((long long) it->base) +" pld:"+ it->payload + ". " + it->comment);
			port.writeOSPcmd(it->msgId, it->payload, it->base);
		} catch (string error) {	//an error has occurred when setting receiver
			log.severe(error);
		}
	}
	/// 8- Creates the output binary file
	FILE *outFile = fopen(parser.getStrOpt(BFILE).c_str(), "wb");
	if (outFile == NULL) {
		log.severe("Cannot create the binary output file " + string(fileName));
		return 5;
	}
	/// 9- Calls acquireBin to acquire and record data form receiver
	int n = acquireBin(port, outFile, nEpochs * 20, nEpochs, patience, &log);
	fclose(outFile);
	port.closePort();
	return n;
}
/**acquireBin
 * acquires binary OSP messages from the receiver and record them in the binary OSP file.
 * Data are read from the receiver and written to the OSP file until:
 * - the maximum number of messages is reached, or
 * - the maximum number of epoch is reached, or
 * - an unrecoverable error happens reading data from receiver
 * - a write error happens
 * 
 *@param  port the SerialTxRx object used to communicate with the receiver
 *@param  outFile the binary output file to record the messages received from receiver
 *@param maxMsgs the maximum number of messages to be recorded
 *@param maxEpochs the maximum number of epochs to be recorded
 *@param patience the maximum number of erroneous contiguous bytes to read before returning a read error
 *@param plog the pinter to the Logger
 *@return a read status according to the following values and meaning:
 *		- (0) no errors have been detected
 *		- (6) error has occurred when writing data read from receiver
 *		- (7) error reading data from receiver: patience exahusted or EOF
 */
int acquireBin(SerialTxRx port, FILE *outFile, int maxMsgs, int maxEpochs, int patience, Logger* plog) {
	/**The acquireBin process sequence follows:*/
	string txtToLog;
	int lastMsgMID = stoi(parser.getStrOpt(MID));
	/// 1- Sets counters
	int nMsgs = 0;
	int nErrors = 0;
	int nEpochs = 0;
	int readResult = 0;
	/// 2- Reads messages from the input stream until counts exhausted or unrecoverable error happen 
	while ((nMsgs < maxMsgs) && (nEpochs < maxEpochs)) {
		readResult = port.readOSPmsg(patience);
		/// - Log message read using format OSP<MID,length> Result
		txtToLog = "R OSP<"
			+ to_string((long long) ((int) port.payBuff[0])) 
			+ ":" + to_string((long long) ((int) port.payloadLen)) + "> ";
		switch (readResult) {
		case 0:	//message is correct
			txtToLog += "OK";
			/// - Update counters and write message to OSP file
			nMsgs++;
			if (port.payBuff[0] == lastMsgMID) nEpochs++;
			if ((fwrite(port.paylenBuff, 1, 2, outFile) != 2) ||
				(fwrite(port.payBuff, 1, port.payloadLen, outFile) != port.payloadLen)) {
				plog->severe(txtToLog + ". Write error");
				plog->info("nMsgs:" + to_string((long long) nMsgs) + " nEpochs:" + to_string((long long) nEpochs));
				return 6;
			}
			plog->finest(txtToLog);
			break;
		case 1:
			plog->warning(txtToLog + "Error in checksum");
			nErrors++;
			break;
		case 2:
			plog->warning(txtToLog + "Error reading payload or shorter than expected");
			nErrors++;
			break;
		case 3:
			plog->warning(txtToLog + "Error. Length out of margin");
			nErrors++;
			break;
		case 4:
			plog->warning(txtToLog + "Error reading payload length");
			nErrors++;
			break;
		case 5:
			plog->warning(txtToLog + "Error reading payload");
			nErrors++;
			break;
		case 6:
			plog->warning("Error reading. Patience exahusted or EOF");
			plog->info("nMsgs:" + to_string((long long) nMsgs) + " nEpochs:" + to_string((long long) nEpochs));
			return 7;
		default:
			plog->severe(txtToLog + "");
			nErrors++;
			break;
		}
	}
	plog->info("Acq End; nMsgs:" + to_string((long long) nMsgs) + " nEpochs:" + to_string((long long) nEpochs));
	return 0;
}
