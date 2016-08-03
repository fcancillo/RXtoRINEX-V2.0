/** @file OSPtoRINEX.cpp
 * Contains the command line program to generate RINEX files from an OSP data file containing SiRF IV receiver messages. 
 *<p>Usage:
 *<p>OSPtoRINEX.exe {options} [OSPfilename]
 *<p>Options are:
 *	- -a or --aend : Append end-of-file comment lines to Rinex file. Default value FALSE
 *	- -b or --bias : Apply receiver clock bias to measurements and time. Default value TRUE
 *	- -c or --glo50bps : Use MID8 GLONASS 50bps data to generate GLONASS navigation file, instead of MID70. Default value FALSE
 *	- -d or --gps50bps : Use MID8 GPS 50bps data to generate nav file, instead of MID15. Default value FALSE
 *	- -h or --help : Show usage data and stops. Default value HELP=FALSE
 *	- -i MINSV or --minsv=MINSV : Minimun satellites in a fix to acquire observations. Default value MINSV = 4
 *	- -j ANTN or --antnum=ANTN : Receiver antenna number. Default value ANTN = Antenna#
 *	- -k ANTT or --antype=ANTT : Receiver antenna type. Default value ANTT = AntennaType
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *	- -m MRKNAM or --mrkname=MRKNAM : Marker name. Default value MRKNAM = MRKNAM
 *	- -n or --nav : Generate RINEX navigation file. Default value FALSE
 *	- -o OBSERVER or --observer=OBSERVER : Observer name. Default value OBSERVER = OBSERVER
 *	- -p PGM or --program=PGM : Program used to generate RINEX file. Default value OSPtoRINEX
 *	- -q RUNBY or --runby=RUNBY : Who runs the RINEX file generator. Default value RUNBY = RUNBY
 *	- -r RINEX or --rinex=RINEX : RINEX file name prefix. Default value RINEX = PNT1
 *	- -s SYSLST or --selsys=SYSLST : List of additional systems to GPS (R or S or R,S) to be included in the RINEX files. Default value an empty list
 *	- -u MRKNUM or --mrknum=MRKNUM : Marker number. Default value MRKNUM = MRKNUM
 *	- -v VER or --ver=VER : RINEX version to generate (V210, V300). Default value VER = V210
 *	- -y AGENCY or --agency=AGENCY : Agency name. Default value AGENCY = AGENCY
 *Default value for operator is: DATA.OSP 
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
 *V2.0	|3/2016	|Removed option to allow selection for GPS and SBAS of observables to include.
 *				|Removed option to generate or not observation file.
 *				|Added capability to generate GLONASS navigation files
 *				|Added capability to generate multiple navigation files in V2.10
 */

//from CommonClasses
#include "ArgParser.h"
#include "Logger.h"
#include "Utilities.h"
#include "GNSSdataFromOSP.h"
#include "RinexData.h"


using namespace std;

//@cond DUMMY
///Compilation date to identify program full version
const string COMPDATE = __DATE__;
///Program name
const string THISPRG = "OSPtoRINEX";
///The command line format
const string CMDLINE = THISPRG + ".exe {options} [OSPfilename]";
///Current program version
const string MYVER = " V2.0 ";
///A common message
const string FILENOK = "Cannot open or create file ";
///The receiver name
const string RECEIVER_NAME = "SiRF";
//The parser object to store options and operators passed in the comman line
ArgParser parser;
//Metavariables for options
int AGENCY, APPEND, ANTN, ANTT, APBIAS, MID8G, MID8R, HELP, LOGLEVEL, NAVI, MINSV, MRKNAM, MRKNUM, OBSERVER, PGM, RINEX, RUNBY, SELSYS, TOFO, VER;
//Metavariables for operators
int OSPF;
//functions in this file
int generateRINEX(FILE*, Logger*);
void prinfNavFile(RinexData &, RinexData::RINEXversion, char, Logger*);
//@endcond 
/**main
 * gets the command line arguments, sets parameters accordingly and triggers the data acquisition to generate RINEX files.
 * Input data are contained  in a OSP binary file containing receiver messages (see SiRF IV ICD for details).
 * The output is a RINEX observation data file, and optionally a RINEX navigation data file.
 * A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange
 * Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists
 * also for Version 3.00.
 *
 *@param argc the number of arguments passed from the command line
 *@param argv the array of arguments passed from the command line
 *@return  the exit status according to the following values and meaning::
 *		- (0) no errors have been detected
 *		- (1) an error has been detected in arguments
 *		- (2) error when opening the input file
 *		- (3) error when creating output files or no epoch data exist
 */
int main(int argc, char* argv[]) {
	/**The main process sequence follows:*/
	/// 1- Defines and sets the error logger object
	Logger log("LogFile.txt", string(), string(argv[0]) + MYVER + COMPDATE + string(" START"));
	/// 2- Setups the valid options in the command line. They will be used by the argument/option parser
	AGENCY = parser.addOption("-y", "--agency", "AGENCY", "Agency name", "AGENCY");
	VER = parser.addOption("-v", "--ver", "VER", "RINEX version to generate (V210, V302)", "V210");
	MRKNUM = parser.addOption("-u", "--mrknum", "MRKNUM", "Marker number", "MRKNUM");
	SELSYS = parser.addOption("-s", "--selsys", "SELSYS", "Systems from input in addition to GPS (R,S or R or S)", "");
	RINEX = parser.addOption("-r", "--rinex", "RINEX", "RINEX file name prefix", "PNT1");
	RUNBY = parser.addOption("-q", "--runby", "RUNBY", "Who runs the RINEX file generation", "RUNBY");
	PGM = parser.addOption("-p", "--program", "PGM", "Program used to generate RINEX file", (char *) (THISPRG+MYVER).c_str());
	OBSERVER = parser.addOption("-o", "--observer", "OBSERVER", "Observer name", "OBSERVER");
	NAVI = parser.addOption("-n", "--nRINEX", "NAVI", "Generate RINEX navigation file", false);
	MRKNAM = parser.addOption("-m", "--mrkname", "MRKNAM", "Marker name", "MRKNAM");
	LOGLEVEL = parser.addOption("-l", "--llevel", "LOGLEVEL", "Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)", "INFO");
	ANTT = parser.addOption("-k", "--antype", "ANTT", "Receiver antenna type", "AntennaType");
	ANTN = parser.addOption("-j", "--antnum", "ANTN", "Receiver antenna number", "Antenna#");
	MINSV = parser.addOption("-i", "--minsv", "MINSV", "Minimun satellites in a fix to acquire observations", "4");
	HELP = parser.addOption("-h", "--help", "HELP", "Show usage data and stops", false);
	MID8G = parser.addOption("-d", "--gps50bps", "MID8G", "Use MID8 GPS 50bps data to generate nav file", false);
	MID8R = parser.addOption("-c", "--glo50bps", "MID8R", "Use MID8 GLONASS 50bps data to generate nav file", false);
	APBIAS = parser.addOption("-b", "--bias", "APBIAS", "Apply receiver clock bias to measurements (and time)", true);
	APPEND = parser.addOption("-a", "--aend", "APPEND", "Append end-of-file comment lines to Rinex file", false);
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
	if (parser.getBoolOpt(HELP)) {
		//help info has been requested
		parser.usage("Generates RINEX files from an OSP data file containing SiRF IV receiver messages", CMDLINE);
		return 0;
	}
	/// 5- Sets logging level stated in option
	log.setLevel(parser.getStrOpt(LOGLEVEL));
	/// 6- Opens the OSP binary file
	FILE* inFile;
	string fileName = parser.getOperator (OSPF);
	if ((inFile = fopen(fileName.c_str(), "rb")) == NULL) {
		log.severe(FILENOK + fileName);
		return 2;
	}
	/// 7- Calls generateRINEX to generate RINEX files extracting data from messages in the binary OSP file
	int n = generateRINEX(inFile, &log);
	log.info("End of RINEX generation. Epochs read: " + to_string((long long) n));
	fclose(inFile);
	return n>0? 0:3;
}
/**generateRINEX iterates over the input OSP file processing GNSS receiver messages to extract RINEX data and print them.
 *
 *@param inFile is the FILE containing the binary OSP messages
 *@param plog point to the Logger
 *@return the number of epochs read in the inFile
 */
int generateRINEX(FILE* inFile, Logger* plog) {
	/**The generateRINEX process sequence follows:*/
	int epochCount;		//to count the number of epochs processed
	string outFileName;	//the output file name for RINEX files
	FILE* obsFile;		//the file where RINEX observation data will be printed
	vector<string> selSys;	//the selected systems
	vector<string> selObs;	//the empty selected observations
	vector<string> observables = getTokens("C1C,L1C,D1C,S1C", ',');	//the defined observables in OSP
	bool glonassSel = false;		//if GLONASS data (observation or navigation) are requested or not
	bool prtNav = parser.getBoolOpt(NAVI);	//if navigation file will be printed or not
	/// 1- Setups the RinexData object members with data given in command line options
	string aStr = parser.getStrOpt(SELSYS);	//the selected systems
	if (aStr.empty()) aStr = "G";
	else aStr = "G," + aStr;
	selSys = getTokens(aStr, ',');
	aStr = parser.getStrOpt(VER);
	RinexData::RINEXversion rinexVer = RinexData::V210;		//default version is 2.10
	if (aStr.compare("V302") == 0) rinexVer = RinexData::V302;
	RinexData rinex(rinexVer, plog);
	try {
		rinex.setHdLnData(RinexData::RUNBY, parser.getStrOpt(PGM), parser.getStrOpt(RUNBY));
		rinex.setHdLnData(RinexData::MRKNAME, parser.getStrOpt(MRKNAM));
		rinex.setHdLnData(RinexData::MRKNUMBER, parser.getStrOpt(MRKNUM));
		rinex.setHdLnData(RinexData::ANTTYPE, parser.getStrOpt(ANTN), parser.getStrOpt(ANTT));
		rinex.setHdLnData(RinexData::ANTHEN, (double) 0.0, (double) 0.0, (double) 0.0);
		rinex.setHdLnData(RinexData::AGENCY, parser.getStrOpt(OBSERVER), parser.getStrOpt(AGENCY));
		rinex.setHdLnData(RinexData::TOFO, string("GPS"));
		rinex.setHdLnData(RinexData::WVLEN, (int) 1, (int) 0);
		for (vector<string>::iterator it = selSys.begin(); it != selSys.end(); it++) {
			rinex.setHdLnData(RinexData::TOBS, it->at(0), observables);
			if (it->at(0) == 'R') glonassSel = true;
		}
		if (!rinex.setFilter(selSys, selObs)) plog->warning("Error in selected systems. Erroneous data ignored");
	} catch (string error) {
			plog->severe(error);
	}
	/// 2- Setups the GNSSdataFromOSP object used to extract message data from the OSP file
	GNSSdataFromOSP gnssAcq(RECEIVER_NAME, stoi(parser.getStrOpt(MINSV)), parser.getBoolOpt(APBIAS), inFile, plog);
	/// 3- Starts data acquisition extracting RINEX header data located in the binary file
	if(!gnssAcq.acqHeaderData(rinex)) {
		plog->warning("All, or some header data not acquired");
	};
	if (glonassSel) gnssAcq.acqGLOparams();
	/// 4- For the observation RINEX file, generate the filename in standard format, create it, print header,
	outFileName = rinex.getObsFileName(parser.getStrOpt(RINEX));
	if ((obsFile = fopen(outFileName.c_str(), "w")) == NULL) {
		plog->severe(FILENOK + outFileName);
		return 0;
	}
	try {
		rinex.printObsHeader(obsFile);
	/// and iterate over the binary OSP file extracting epoch by epoch data and printing them
		epochCount = 0;
		rewind(inFile);
		while (gnssAcq.acqEpochData(rinex, parser.getBoolOpt(MID8G), parser.getBoolOpt(MID8R))) {
			rinex.printObsEpoch(obsFile);
			epochCount++;
		}
		if (parser.getBoolOpt(APPEND)) rinex.printObsEOF(obsFile);
	} catch (string error) {
		plog->severe(error);
	}
	fclose(obsFile);
	/// 5- If navigation RINEX file requested, generate the filename in standard format, create it, print header,
	if (prtNav) {
		if (rinexVer == RinexData::V302) {
			prinfNavFile(rinex, rinexVer, 'M', plog);
		}
		else {
			for (vector<string>::iterator it = selSys.begin(); it != selSys.end(); it++) {
				prinfNavFile(rinex, rinexVer, it->at(0), plog);
			}
		}
	}
	return epochCount;
}

/**prinfNavFile prints a RINEX navigation file from the navigation data stored stored in the given RinexData object.
 *File format will be according the given version, and for the given satellite system if version to be generated is 2.10.
 *
 *@param rinex is the RinexData object containing navigation data for the file to be printed
 *@param ver is the RINEX version of the file to be generated
 *@param sysId is the identification of the satellite system data to be printed. Only relevant for version 2.10 files.
 *@param plog a pointer to the Logger object where logging messages will be printed
 */

void prinfNavFile(RinexData &rinex, RinexData::RINEXversion ver, char sysId, Logger* plog) {
	FILE* navFile;		//the file where RINEX navigation data will be printed
	string outFileName;	//the output file name for RINEX files
	char fnameSfx;
	switch (ver) {
	case RinexData::V210:
		switch (sysId) {
		case 'G': fnameSfx = 'N'; break;
		case 'R': fnameSfx = 'G'; break;
		case 'S': fnameSfx = 'H'; break;
		default:
			plog->warning("Cannot print RINEX V2.10 navigation file for system " + string(1, sysId));
			return;
		}
		outFileName = rinex.getNavFileName(parser.getStrOpt(RINEX), fnameSfx);
		break;
	case RinexData::V302:
		outFileName = rinex.getNavFileName(parser.getStrOpt(RINEX));
		break;
	}
	if ((navFile = fopen(outFileName.c_str(), "w")) == NULL) {
		plog->warning(FILENOK + outFileName);
		return;
	}
	try {
		rinex.setFilter(vector<string>(1,string(1,sysId)), vector<string>());
		rinex.printNavHeader(navFile);
		rinex.printNavEpoch(navFile);
	} catch (string error) {
		plog->severe(error);
	}
	fclose(navFile);
}