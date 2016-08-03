/** @file OSPtoTXT.cpp
 * Contains the command line program to dump contents of an OSP binary data file to the standard output.
 *<p>
 *Usage:
 *<p>OSPtoTXT.exe {options} [OSPfileName]
 *<p>Options are:
 *	- -h or --help : Show usage data. Default value HELP=FALSE
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *Default values for operators are: DATA.OSP 
 *<p>
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
 *V1.1	|2/2016	|Minor changes to improve logging
 */

//from CommonClasses
#include "ArgParser.h"
#include "Logger.h"
#include "OSPMessage.h"
#include "Utilities.h"

using namespace std;

//@cond DUMMY
///The command line format
const string CMDLINE = "OSPtoTXT.exe {options} [OSPfileName]";
///The current version of this program
const string MYVER = " V1.1";
///The parser object to store options and operators passed in the command line
ArgParser parser;
//Metavariables for options
int HELP, LOGLEVEL;	//the metavariables for the command line options 
//Metavariables for operators
int OSPF;		//metavariables for the command line operands
//@endcond 
//functions in this file
int extractMsgs(FILE* , Logger*);

/**main
 * gets the command line arguments, set parameters accordingly and performs the data acquisition for printing them.
 * Input data are contained  in a OSP binary file containing length and payload of receiver messages
 * (see SiRF IV ICD for details).
 * The output contains printed descriptive relevant data from each OSP message:
 *  - Message identification (MID, in decimal) and payload length for all messages
 *  - Payload parameter values for relevant messages used to generate RINEX or RTK files (MIDs 2, 6, 7, 8, 11, 12, 15, 28, 50, 56, 64, 68, 75)
 *  - Payload bytes in hexadecimal, for MID 255
 * Output data are sent to the standard output (stdout file), which could be redirected.
 *
 *@param argc the number of arguments passed from the command line
 *@param argv the array of arguments passed from the command line
 *@return  the exit status according to the following values and meaning::
 *		- (0) no errors have been detected
 *		- (1) an error has been detected in arguments
 *		- (2) error when opening the input file
 */
int main(int argc, char* argv[]) {
	/**The main process sequence follows:*/
	/// 1- Defines and sets the error logger object
	Logger log("LogFile.txt", string(), string(argv[0]) + MYVER + string(" START"));
	/// 2- Setups the valid options in the command line. They will be used by the argument/option parser
	LOGLEVEL = parser.addOption("-l", "--llevel", "LOGLEVEL", "Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)", "INFO");
	HELP = parser.addOption("-h", "--help", "HELP", "Show usage data", false);
	/// 3- Setups the default values for operators in the command line
	OSPF = parser.addOperator("DATA.OSP");
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
		parser.usage("Dumps contents of a OSP binary data files to the standard out", CMDLINE);
		return 0;
	}
	/// 5- Sets logging level stated in option
	log.setLevel(parser.getStrOpt(LOGLEVEL));
	/// 6- Opens the OSP binary file
	FILE* inFile;
	string fileName = parser.getOperator (OSPF);
	if ((inFile = fopen(fileName.c_str(), "rb")) == NULL) {
		log.severe("Cannot open file " + fileName);
		return 2;
	}
	/// 7- Call extractMsgs to extract messages from the binary OSP file and print contents
	int n = extractMsgs(inFile, &log);
	fclose(inFile);
	log.info("End of data extraction. Messages read: " + to_string((long long) n));
	return 0;
}

/**extractMsgs
 * extracts OSP messages contained in a OSP binary file and prints relevant data to stdout.
 * The OSP binary file contain OSP messages (see SiRF IV ICD for details) 
 *
 * @param inFile the pointer to the OSP binary FILE to read
 * @param plog the pointer to the Logger object
 * @return the number of messages read
 */
int extractMsgs(FILE* inFile, Logger* plog) {
	OSPMessage message;
	int mid;
	int nMessages = 0;
	///For each input message, the following data are printed:
	while (message.fill(inFile)) {
		nMessages++;
		mid = message.get();
		/// - for all messages, MID and payload length
		printf("MID:%3d;Ln:%3d;", mid, message.payloadLen());
		switch (mid) {
		case 2:		/// - MID 2, solution data: X, Y, Z, vX, vY, vZ, week, TOW and satellites used
			printf("X:%8d;",message.getInt());
			printf("Y:%8d;",message.getInt());
			printf("Z:%8d;",message.getInt());
			printf("vX:%4hd;",message.getShort());
			printf("vY:%4hd;",message.getShort());
			printf("vZ:%4hd;",message.getShort());
			message.get(); //skip Mode 1
			message.get(); //skip HDOP2
			message.get(); //skip Mode 2
			printf("wk:%4hu;",message.getUShort());
			printf("TOW:%6u;",message.getUInt());
			printf("SVs:%2d",message.get());
			break;
		case 6:		/// - MID 6: SiRF and customer versions
			unsigned int lsirf, lcust;
			lsirf = message.get();
			lcust = message.get();
			printf("SiRF ver:");
			while (lsirf-- > 0) printf("%c", message.get());
			printf(";Cust ver:");
			while (lcust-- > 0) printf("%c", message.get());
			break;
		case 7:		/// - MID 7, Clock Status Data: week, TOW, satellites used, drift, bias, and EsT
			printf("ewk:%3hu;", message.getUShort());
			printf("TOW:%6u;", message.getUInt());
			printf("SVs:%2d;", message.get());
			printf("drft:%8u;", message.getUInt());
			printf("bias:%8u;", message.getUInt());
			printf("EsT:%8u", message.getUInt());
			break;
		case 8:		/// - MID 8, 50 BPS Data: 10 words subframe in hexadecimal
			printf("ch:%2d;", message.get());
			printf("SV:%2d;", message.get());
			unsigned int word[10];
			for(int i=0; i<10; i++) word[i] = message.getUInt();
			printf("TOW:%6u;sfr:%2u;pg:%2u;\n\t",
					(word[1]>>13) & 0x1FFFF,
					(word[1]>>8) & 0x07,
					(word[2]>>24) & 0x3F );
			for(int i=0; i<10; i++) printf("%08X;", word[i]);
			break;
		case 11:	/// - MID 11, Command Acknowledgment
			printf("ack:%3d", message.get());
			break;
		case 12:	/// - MID 12, Command Negative Acknowledgment
			printf("nack:%3d", message.get());
			break;
		case 15:	/// - MID 15, Ephemeris Data with compact subframes 1, 2 & 3, in response to poll
			printf("SV:%2d\n", message.get());
			for(int i=0; i<3; i++) {
				printf("\t");
				for (int j=0; j<15; j++) printf(" %04hX", message.getUShort());
				printf("\n");
			}
			break;
		case 28:	/// - MID 28, Navigation Library Measurement Data
			printf("Ch:%2d;", message.get());
			printf("Ttg:%8u;", message.getUInt());
			printf("SV:%2d;", message.get());
			printf("Tsw:%14.3f;", message.getDouble());
			printf("Psr:%14.3f;", message.getDouble());
			printf("Cfr:%14.3f;", message.getFloat());
			printf("Cph:%14.3f;", message.getDouble());
			printf("Trk:%3hu;", message.getUShort());
			printf("Syn:%02X\n", message.get());
			printf("\tCN0:");
			for (int i=0; i<10; i++) printf("%3d;", message.get());
			printf("\n\tDri:%5hu", message.getUShort());
			break;
		case 50:	/// - MID 50, SBAS Parameters
			printf("SBASsv:%3d;", message.get());
			printf("Md:%3d;", message.get());
			printf("Tout:%3d;", message.get());
			printf("Flg:%02X", message.get());
			break;
		case 56:	/// - MID 56: Extended Ephemeris Data-reserved use
			printf("SID:%3d;", message.get());
           	break;
		case 64:	//Navigation Library Messages
			printf("SID:%3d", message.get());
			break;
		case 67:	/// - MID 67, Multi-constellation Navigation Data (SiRFV)
			printf("SID:%3d", message.get());
			break;
		case 68:	/// - MID 68, Measurement Engine. Wraps the content of another OSP message and outputs it to SiRFLive
			printf("Wraps:%3d", message.get());
			break;
		case 70:	/// -MID 70 GLONASS almanac/ephemeris response to MID 212
			printf("SID:%3d", message.get());
			break;
		case 75:	/// - MID 75, ACK/NACK/ERROR Notification 
			printf("SID:%2d;", message.get());
			printf(" echo to MID%2d ", message.get());
			printf("SID%2d:", message.get());
			printf("%02x", message.get());
			break;
		case 255:	/// - MID 255, ASCII Development Data Output
			for(unsigned int i=0; i<message.payloadLen()-1; i++)  printf("%c", message.get());
			break;
		default:
			break;
		}
		printf("\n");
	}
	return nMessages;
}

