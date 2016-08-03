/** @file RINEXtoRINEX.cpp
 * Contains the command line program to convert an existing input RINEX observation or navigation file to a new RINEX file having data and version stated in the options. 
 *<p>Usage:
 *<p>RINEXtoRINEX.exe {options} InputRINEXfilename
 *<p>Options are:
 *	- -f FROMT or --fromtime=FROMT : Select epochs from the given date and time (comma separated yyyy,mm,dd,hh,mm,sec. Default value: 1st epoch in the input file
 *	- -k or --skipe : Skip epochs with erroneus data. Default value false
 *	- -h or --help : Show usage data and stops. Default value HELP=FALSE
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *	- -o OBSLST or --selobs=OBSLST : List of selected system-observables (ver.3.02 notation) from input (a comma separated list, like GC1C,GL1C). Default value is all selected.
 *	- -p OBS2LST or --selobs2=OBS2LST : List of selected system-observables (ver.2.10 notation) from input (comma separated list, like GC1,GL1,GL2). Default value is all selected.
 *	- -r RINEX or --rinex=RINEX : Output RINEX file name prefix. Default value RINEX = RTOR
 *	- -s SATLST or --selsat=SATLST : List of selected system-satellites from input (comma separated list, like G01,G02). Default value is all selected.
 *	- -t TOT or --totime=TOT : Select epochs before the given date and time (comma separated yyyy,mm,dd,hh,mm,sec. Default value: last epoch in the input file
 *	- -u RUNBY or --runby=RUNBY : Who runs the RINEX file generation. Default value: Not specified
 *	- -v VER or --ver=VER : RINEX version to generate (V210, V302). Default value VER = TBD (same as input)
 *<p>
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
 *<p>
 *Ver.	|Date	|Reason for change
 *------+-------+------------------
 *V1.0	|2/2016	|First release
 */
//from CommonClasses
#include "ArgParser.h"
#include "Logger.h"
#include "Utilities.h"
#include "RinexData.h"

using namespace std;

//@cond DUMMY
///The command line format
const string CMDLINE = "RINEXtoRINEX.exe {options} InputRINEXfilename";
///The program current version
const string MYVER = " V1.1";
///The parser object to store options and operators passed in the comman line
ArgParser parser;
//Metavariables for options
int AEND, BIAS, FROMT, GPS, HELP, LOGLEVEL, MINSV, OUTRINEX, RUNBY, SELOBS3, SELOBS2, SELSAT, SKIPE, TOT, VER;
//Metavariables for operators
int INRINEX;
//@endcond 

/**main
 * gets the command line arguments, set parameters accordingly and triggers the data acquisition to generate RINEX files.
 * Input data are contained  in a RINEX observation file containing header and observations data in the given format version.
 * The output is a RINEX observation data file in the version reuested.
 * A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange
 * Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists
 * also for Version 3.02.
 *
 *@param argc the number of arguments passed from the command line
 *@param argv the array of arguments passed from the command line
 *@return  the exit status according to the following values and meaning::
 *		- (0) no errors have been detected
 *		- (1) an error has been detected in arguments
 *		- (2) error when opening the input file
 *		- (3) error when reading, setting or printing header data
 *		- (4) error in data filtering parameters
 *		- (5) there were format errors in epoch data or no epoch data exist
 *		- (6) error when creating output file
 */
int main(int argc, char* argv[]) {
	/**The main process sequence follows:*/
	/// 1 - Defines and sets the error logger object
	Logger log("LogFile.txt", string(), string(argv[0]) + MYVER + string(" START"));
	/// 2 - Setups the valid options in the command line. They will be used by the argument/option parser
	VER = parser.addOption("-v", "--ver", "VER", "RINEX version to generate (V210, V302)", "TBD");
	RUNBY = parser.addOption("-u", "--runby", "RUNBY", "Who runs the RINEX file generation", "Run by");
	TOT = parser.addOption("-t", "--totime=TOT", "TOT", "Select epochs before the given date and time (comma separated yyyy,mm,dd,hh,mm,sec", "");
	SELSAT = parser.addOption("-s", "--selsat", "SELSAT", "Select system-satellite from input (comma separated list of sys{-prn}, like G,R or G01,G02)", "");
	OUTRINEX = parser.addOption("-r", "--rinex", "RINEX", "RINEX file name prefix", "RTOR");
	SELOBS2 = parser.addOption("-p", "--selobs2", "SELOBS2", "Select system-observable (ver.2.10 notation) from input (comma separated list, like C1,L1,L2)", "");
	SELOBS3 = parser.addOption("-o", "--selobs", "SELOBS3", "Select system-observable (ver.3.02 notation) from input (comma separated list, like GC1C,GL1C)", "");
	LOGLEVEL = parser.addOption("-l", "--llevel", "LOGLEVEL", "Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)", "INFO");
	HELP = parser.addOption("-h", "--help", "HELP", "Show usage data and stops", false);
	SKIPE = parser.addOption("-k", "--skipe", "SKIPE", "Skip epochs with erroneus data", false);
	FROMT = parser.addOption("-f", "--fromtime=FROMT", "FROMT", "Select epochs from the given date and time (comma separated yyyy,mm,dd,hh,mm,sec", "");
	/// 3- Setups the default values for operators in the command line
	INRINEX = parser.addOperator("RINEX.DAT");
	/// 4 - Parses arguments in the command line extracting options and operators
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
		parser.usage("Parses and read the given observation RINEX file generating a new file with the requested characteristics", CMDLINE);
		return 0;
	}
	/// 5 - Sets logging level stated in option. Default level is INFO
	log.setLevel(parser.getStrOpt(LOGLEVEL));
	/// 6 - Set 1st and last epoch time tags (if selected from / to epochs time) 
	bool fromTime = false, toTime = false;
	double fromTimeTag, toTimeTag;
	int week, year, month, day, hour, minute;
	double tow, second;
	string aStr = parser.getStrOpt(FROMT);
	if (!aStr.empty()) {
		if (sscanf(aStr.c_str(), "%d,%d,%d,%d,%d,%lf", &year, &month, &day, &hour, &minute, &second) != 6) {
			log.severe("Cannot state 'from time' for the time interval");
			return 1;
		}
		setWeekTow (year, month, day, hour, minute, second, week, tow);
		fromTimeTag = getSecsGPSEphe(week, tow);
		fromTime = true;
	}
	aStr = parser.getStrOpt(TOT);
	if (!aStr.empty()) {
		if (sscanf(aStr.c_str(), "%d,%d,%d,%d,%d,%lf", &year, &month, &day, &hour, &minute, &second) != 6) {
			log.severe("Cannot state 'to time' for the time interval");
			return 1;
		}
		setWeekTow (year, month, day, hour, minute, second, week, tow);
		toTimeTag = getSecsGPSEphe(week, tow);
		toTime = true;
	}
	/// 7 - Opens the RINEX input file
	FILE* inFile;
	string fileName = parser.getOperator (INRINEX);
	if ((inFile = fopen(fileName.c_str(), "r")) == NULL) {
		log.severe("Cannot open file " + fileName);
		return 2;
	}
	/// 8 - Create a RINEX object, and extract header data from the RINEX input file
	RinexData::RINEXversion rinexVer = RinexData::V210;		//default version is 2.10
	aStr = parser.getStrOpt(VER);
	if (aStr.compare("TBD") == 0) rinexVer = RinexData::VTBD;
	else if (aStr.compare("V302") == 0) rinexVer = RinexData::V302;
	RinexData rinex(rinexVer, &log);
	double aDouble;
	char fileType = ' ';
	char sysId = ' ';
	try {
		rinex.readRinexHeader(inFile);
		if (!rinex.getHdLnData(RinexData::INFILEVER, aDouble, fileType, sysId)) {
			log.severe("This RINEX input file version cannot be processed");
			return 3;
		}
		rinex.setHdLnData(RinexData::RUNBY, "RINEXtoRINEX", parser.getStrOpt(RUNBY));
	}  catch (string error) {
		log.severe(error);
		return 3;
	}
	/// 9 - Set filtering parameters for systems, satellites and/or observables, if any
	vector<string> obsV2Tokens = getTokens(parser.getStrOpt(SELOBS2), ',');
	vector<string> obsTokens = getTokens(parser.getStrOpt(SELOBS3), ',');
	string observable;
	//convert obsV2Tokens to V3 and append them to obsTokens
	for (vector<string>::iterator it = obsV2Tokens.begin(); it != obsV2Tokens.end(); it++) {
		observable = rinex.obsV2toV3((*it).substr(1));
		if (observable.empty()) log.warning("Filtering data: ignored unknown V2 observable " + observable);
		else obsTokens.push_back((*it).substr(0,1) + observable);
	}
	if (!rinex.setFilter(getTokens(parser.getStrOpt(SELSAT), ','), obsTokens))
		log.warning("Error in some data filtering parameters. Erroneous data ignored");
	bool skipe;
	string outFileName;
	FILE* outFile;
	int goodCount = 0;
	int badCount = 0;
	int skipCount = 0;
	int anInt;
	switch (fileType) {
	case 'O':
		try {
		/// 10.0 - If observation file, generate a RINEX observation filename for the new ouput file and open it
			//Set the time of the 1st observation as current epoch time
			if (rinex.getHdLnData(RinexData::TOFO, anInt, aDouble, outFileName)) rinex.setEpochTime(anInt, aDouble);
			else log.warning("Time of first observation not set. File name will not be standard");
			outFileName = rinex.getObsFileName(parser.getStrOpt(OUTRINEX));
			if ((outFile = fopen(outFileName.c_str(), "w")) == NULL) {
				log.severe("Cannot create file " + outFileName);
				return 6;
			}
		/// 10.1 -  prints new RINEX header ...
			rinex.printObsHeader(outFile);
		/// 10.2 - ... and iterate over input file extracting epoch by epoch data and printing them
			rinex.clearHeaderData();
			skipe = parser.getBoolOpt(SKIPE);
			while ((anInt = rinex.readObsEpoch(inFile)) != 0) {
				if (fromTime) {
					rinex.getEpochTime(week, tow, aDouble, minute);
					if (fromTimeTag > getSecsGPSEphe(week, tow)) {
						log.finer("Epoch before interval");
						continue;
					}
				}
				if (toTime) {
					rinex.getEpochTime(week, tow, aDouble, minute);
					if (toTimeTag <= getSecsGPSEphe(week, tow)) {
						log.finer("Epoch after interval");
						continue;
					}
				}
				switch (anInt) {
				case 1:
					rinex.printObsEpoch(outFile);
					goodCount++;
					break;
				case 2:
					rinex.printObsEpoch(outFile);
					goodCount++;
					rinex.clearHeaderData();
					break;
				case 3:
					if (!skipe) {
						rinex.printObsEpoch(outFile);
						skipCount++;
					}
				case 4:
				case 8:
					badCount++;
					break;
				case 5:
				case 6:
				case 7:
					if (!skipe) {
						rinex.printObsEpoch(outFile);
						skipCount++;
					}
					rinex.clearHeaderData();
				}
			}
		} catch (string error) {
			log.severe(error + string(". Incomplete RINEX obs. file"));
			fclose(outFile);
			return 5;
		}
		fclose(outFile);
		break;
	case 'N':
	case 'G':
	case 'E':
	case 'R':
		try {
		/// 11.0 - If navigation file, generate a RINEX navigation filename for the new ouput file and open it
			outFileName = rinex.getNavFileName(parser.getStrOpt (OUTRINEX));
			if ((outFile = fopen(outFileName.c_str(), "w")) == NULL) {
				log.severe("Cannot create file " + outFileName);
				return 6;
			}
		/// 11.1 - If navigation file, prints new RINEX header ...
			rinex.printNavHeader(outFile);
		/// 11.2 - ... and iterate over input file extracting epoch by epoch data and printing them
			while (((anInt = rinex.readNavEpoch(inFile)) != 0) && (anInt != 9)) {
				switch (anInt) {
				case 1: //Epoch navigation data are well formatted. They have been stored. They  belong to the current epoch
					rinex.printNavEpoch(outFile);	//print current epoch
					goodCount++;
					break;
				case 2: //Epoch navigation data are well formatted. They have been stored. They  DO NOT belong to the current epoch (cannot happen here: only 1 sat read)
				case 3: //Error in satellite system or prn. No epoch data stored.
				case 4: //Error in epoch date or time format. No epoch data stored.
				case 5: //Error in epoch data. No epoch data stored.
					badCount++;
				}

			}

		} catch (string error) {
			log.severe(error + string(". Incomplete RINEX nav. file"));
			fclose(outFile);
			return 5;
		}
		fclose(outFile);
		break;
	default:
		break;
	}
	fclose(inFile);
	log.info("End of RINEX generation. Epochs: good=" + to_string((long long) goodCount)
			+ " bad=" + to_string((long long) badCount)
			+ " skiped=" +  to_string((long long) skipCount));
	return goodCount>0? 0:5;
}