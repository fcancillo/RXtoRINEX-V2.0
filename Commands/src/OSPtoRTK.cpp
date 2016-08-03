/** @file OSPtoRTK.cpp
 * Contains the command line program to generate a RTK file with positioning data extracted from
 * an OSP data file containing SiRF IV receiver messages.
 *<p>Usage:
 *<p>OSPtoRTK {options} [OSPfileName]
 *<p>Options are:
 *	- -h or --help : Show usage data. Default value HELP=FALSE
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *	- -m MINSV or --minsv=MINSV : Minimum satellites in a fix to acquire solution data. Default value MINSV = 4
 * Default values for operators are: DATA.OSP 
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
 *V1.1	|2/2016	|Minor improvements for logging messages
 */

//from CommonClasses
#include "ArgParser.h"
#include "Logger.h"
#include "RTKobservation.h"
#include "GNSSdataFromOSP.h"
#include "OSPMessage.h"

using namespace std;

//@cond DUMMY
///The command line format
const string CMDLINE = "OSPtoRTK {options} [OSPfileName]";
const string MYVER = " V1.1";
///The receiver name
const string RECEIVER_NAME = "SiRF";
///The parser object to store options and operators passed in the comman line
ArgParser parser;
//Metavariables for options
int HELP, LOGLEVEL, MINSV;
//Metavariables for operators
int OSPF;
//@endcond 
//functions in this module
void generateRTKobs(FILE*, FILE*, string, string, Logger*);

/**main
 * gets the command line arguments, set parameters accordingly and triggers the data acquisition to generate the RTK file.
 * Input data are contained  in a OSP binary file containing receiver messages (see SiRF IV ICD for details).
 * The output is a RTK file with data formatted as per RTKLIB (http://www.rtklib.com/) for this kind of files.
 *
 * @param argc	the number of arguments passed from the command line
 * @param argv	array with argument values passed
 * @return the exit status according to the following values and meaning:
 *		- (0) no errors have been detected
 *		- (1) an error has been detected in arguments
 *		- (2) error when opening the input file
 *		- (3) error when creating output files, or no epoch data exist
 */
int main(int argc, char** argv) {
	/**The main process sequence follows:*/
	/// 1- Defines and sets the error logger object
	Logger log("LogFile.txt", string(), string(argv[0]) + MYVER + string(" START"));
	/// 2- Setups the valid options in the command line. They will be used by the argument/option parser
	MINSV = parser.addOption("-m", "--minsv", "MINSV", "Minimun satellites in a fix to acquire observations", "4");
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
	log.info("Start execution with " + parser.showOptValues());
	log.info(parser.showOpeValues());
	if (parser.getBoolOpt(HELP)) {	//help info has been requested
		parser.usage("Generates a RTK file with positioning data extracted from a OSP data file", CMDLINE);
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
	/// 7- Creates the output RTK file
	string rtkFileName = fileName + ".pos";
	FILE* rtkFile;
	if ((rtkFile = fopen(rtkFileName.c_str(), "w")) == NULL) {
		log.severe("Cannot create file " + rtkFileName);
		return 3;
	}
	/// 8- Generates RTK file calling generateRTKobs to extract data from messages in the binary OSP file and print them
	generateRTKobs(inFile, rtkFile, fileName, string(argv[0]) + MYVER, &log);
    fclose(inFile);
    fclose(rtkFile);
	return 0;
}

/**generateRTKobs
 * iterates over the input OSP file processing GNSS receiver messages to extract RTK positioning data, and prints them.
 * Data are extracted first for the RTK file header and them epoch by epoch for each solution (one per line).
 *
 * @param inFile the  pointer to the input OSP binary FILE
 * @param rtkFile the  pointer to the output RTK FILE
 * @param inFileName the  name of the input OSP binary FILE
 * @param prgName the program name
 * @param plog the pointer to the logger
 */
void generateRTKobs(FILE* inFile, FILE* rtkFile, string inFileName, string prgName, Logger* plog) {
	/**The generateRTKobs process sequence follows:*/
	int nEpochs = 0;		//to count the number of epochs processed
	/// 1- Setups the GNSSdataFromOSP object used to extract data from the binary file
	GNSSdataFromOSP gnssAcq(RECEIVER_NAME, stoi(parser.getStrOpt(MINSV)), true, inFile, plog);
	/// 2- Setups the RTKobservation object where extracted RTK data from the binary file will be placed 
	RTKobservation rtko(prgName, inFileName);
	/// 3- Acquire RTK header data located in the binary input file
	if(!gnssAcq.acqHeaderData(rtko)) {
		plog->warning("All, or some header data not acquired");
	}
	/// 4- Prints RTK file header
	rtko.printHeader(rtkFile);
	rewind(inFile);
	/// 6- Iterates over the binary OSP file extracting epoch by epoch solution data and printing them
	while (gnssAcq.acqEpochData(rtko)) {
		rtko.printSolution(rtkFile);
		nEpochs++;
	}
	plog->info("End of data extraction. Epochs read: " + to_string((long long) nEpochs));
}

