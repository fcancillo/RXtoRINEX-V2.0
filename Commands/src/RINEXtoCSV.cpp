/** @file RINEXtoCSV.cpp
 * Contains the command line program to generate a CSV or TXT file from data contained in a given observation or navigation RINEX file. 
 *<p>Usage:
 *<p>RINEXtoCSV.exe {options} InputRINEXfilename
 *<p>Options are:
 *	- -f FROMT or --fromtime=FROMT : Select epochs from the given date and time (comma separated yyyy,mm,dd,hh,mm,sec. Default value: 1st epoch in the input file
 *	- -h or --help : Show usage data and stops. Default value HELP=FALSE
 *	- -l LOGLEVEL or --llevel=LOGLEVEL : Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST). Default value LOGLEVEL = INFO
 *	- -o OBSLST or --selobs=OBSLST : List of selected system-observables (ver.3.01 notation) from input (a comma separated list, like GC1C,GL1C). Default value is all selected.
 *	- -p OBS2LST or --selobs2=OBS2LST : List of selected system-observables (ver.2.10 notation) from input (comma separated list, like GC1,GL1,GL2). Default value is all selected.
 *	- -s SATLST or --selsat=SATLST : List of selected system-satellites from input (comma separated list, like G01,G02). Default value is all selected.
 *	- -t TOT or --totime=TOT : Select epochs before the given date and time (comma separated yyyy,mm,dd,hh,mm,sec. Default value: last epoch in the input file
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
const string CMDLINE = "RINEXtoCSV.exe {options} InputRINEXfilename";
///The program current version
const string MYVER = " V1.0";
///The parser object to store options and operators passed in the command line
ArgParser parser;
//Metavariables for options
int AEND, BIAS, FROMT, GPS, HELP, LOGLEVEL, MINSV, SELOBS3, SELOBS2, SELSAT, TOT;
//Metavariables for operators
int INRINEX;
//@endcond 
///A data tipe to define a time interval
struct TimeIntervalParams {
	bool fromTime, toTime;
	double fromTimeTag, toTimeTag;
};
//functions in this file
int generateHeaderCSV(FILE*, RinexData &, Logger*);
int generateObsCSV(FILE*, FILE*, RinexData &, TimeIntervalParams &, Logger*);
int generateGPSNavCSV(FILE*, FILE*, RinexData &, TimeIntervalParams &, Logger*);
int generateGalNavCSV(FILE*, FILE*, RinexData &, TimeIntervalParams &, Logger*);
int generateGloNavCSV(FILE*, FILE*, RinexData &, TimeIntervalParams &, Logger*);
int generateSBASNavCSV(FILE*, FILE*, RinexData &, TimeIntervalParams &, Logger*);

/**main
 * gets the command line arguments, set parameters accordingly and triggers the data acquisition to generate CSV files.
 * Input data are contained in a RINEX observation or navigation file containing header and epoch data.
 * The output is a CSV (Comma Separated Values) text data file. This type of files can be used to import data to some available
 * application (like MS Excel).
 * A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange
 * Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists
 * also for Version 3.01.
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
 *		- (7) inconsistent data in RINEX VERSION header record
 */
int main(int argc, char* argv[]) {
	int anInt;		//a general purpose int variable
	string aStr;	//a general purpose string variable
	string fileName;
	double aDouble;	//a general purpose double variable
	/**The main process sequence follows:*/
	/// 1- Defines and sets the error logger object
	Logger log("LogFile.txt", string(), string(argv[0]) + MYVER + string(" START"));
	/// 2- Setups the valid options in the command line. They will be used by the argument/option parser
	TOT = parser.addOption("-t", "--totime=TOT", "TOT", "Select epochs before the given date and time (comma separated yyyy,mm,dd,hh,mm,sec", "");
	SELSAT = parser.addOption("-s", "--selsat", "SELSAT", "Select system-satellite from input (comma separated list of sys-prn, like G01,G02)", "");
	SELOBS2 = parser.addOption("-p", "--selobs2", "SELOBS2", "Select system-observable (ver.2.10 notation) from input (comma separated list, like C1,L1,L2)", "");
	SELOBS3 = parser.addOption("-o", "--selobs", "SELOBS3", "Select system-observable (ver.3.01 notation) from input (comma separated list, like GC1C,GL1C)", "");
	LOGLEVEL = parser.addOption("-l", "--llevel", "LOGLEVEL", "Maximum level to log (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)", "INFO");
	HELP = parser.addOption("-h", "--help", "HELP", "Show usage data and stops", false);
	FROMT = parser.addOption("-f", "--fromtime=FROMT", "FROMT", "Select epochs from the given date and time (comma separated yyyy,mm,dd,hh,mm,sec", "");
	/// 3- Setups the default values for operators in the command line
	INRINEX = parser.addOperator("RINEX.DAT");
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
		parser.usage("Parses and read the given observation RINEX file generating a CSV ot TXT file with the requested characteristics", CMDLINE);
		return 0;
	}
	/// 5- Sets logging level stated in option. Default level is INFO
	log.setLevel(parser.getStrOpt(LOGLEVEL));
	/// 7 - Set 1st and last epoch time tags (if selected from / to epochs time) 
	TimeIntervalParams timeInterval;
	int week, year, month, day, hour, minute;
	double tow, second;
	aStr = parser.getStrOpt(FROMT);
	if (!aStr.empty()) {
		if (sscanf(aStr.c_str(), "%d,%d,%d,%d,%d,%lf", &year, &month, &day, &hour, &minute, &second) != 6) {
			log.severe("Cannot state 'from time' for the time interval");
			return 1;
		}
		setWeekTow (year, month, day, hour, minute, second, week, tow);
		timeInterval.fromTimeTag = getSecsGPSEphe(week, tow);
		timeInterval.fromTime = true;
	} else timeInterval.fromTime = false;
	aStr = parser.getStrOpt(TOT);
	if (!aStr.empty()) {
		if (sscanf(aStr.c_str(), "%d,%d,%d,%d,%d,%lf", &year, &month, &day, &hour, &minute, &second) != 6) {
			log.severe("Cannot state 'to time' for the time interval");
			return 1;
		}
		setWeekTow (year, month, day, hour, minute, second, week, tow);
		timeInterval.toTimeTag = getSecsGPSEphe(week, tow);
		timeInterval.toTime = true;
	} else timeInterval.toTime = false;
	/// 7- Opens the RINEX input file passed as operator
	FILE* inFile;
	fileName = parser.getOperator(INRINEX);
	if ((inFile = fopen(fileName.c_str(), "r")) == NULL) {
		log.severe("Cannot open file " + fileName);
		return 2;
	}
	/// 8- Create a RINEX object and extract header data from the RINEX input file
	RinexData rinex(RinexData::VTBD, &log);
	char fileType = ' ';
	char sysId = ' ';
	try {
		rinex.readRinexHeader(inFile);
		if (!rinex.getHdLnData(RinexData::INFILEVER, aDouble, fileType, sysId)) {
			log.severe("This RINEX input file version cannot be processed");
			fclose(inFile);
			return 3;
		}
	}  catch (string error) {
		log.severe(error);
		fclose(inFile);
		return 3;
	}
	/// 9 - Set filtering parameters passed in options, if any
	//convert obsV2Tokens to V3 and append them to obsTokens
	vector<string> obsV2Tokens = getTokens(parser.getStrOpt(SELOBS2), ',');
	vector<string> obsTokens = getTokens(parser.getStrOpt(SELOBS3), ',');
	for (vector<string>::iterator it = obsV2Tokens.begin(); it != obsV2Tokens.end(); it++) {
		aStr = rinex.obsV2toV3((*it).substr(1));
		if (aStr.empty()) log.warning("Filtering data: ignored unknown V2 observable " + aStr);
		else obsTokens.push_back((*it).substr(0,1) + aStr);
	}
	//verify coherence of SELSAT w.r.t. fileType and system identifier
	vector<string> selsat = getTokens(parser.getStrOpt(SELSAT), ',');
	if (fileType == 'N' && sysId == 'M') {
		if (selsat.empty()) {
			log.severe("File is Navigation type 'M', and no sytem was selected.");
			return 4;
		} else {
			//state sysId as per the 1st satellite selected
			sysId = selsat[0].at(0);
		}
	}
	if (!rinex.setFilter(selsat, obsTokens))
		log.warning("Ignored inconsistent data filtering parameters for observation files.");
	/// 10 - Create output file for header data (suffix name _HDR.CSV), print them, and close output file
	FILE* outFile;
	while ((anInt = fileName.find('.')) != string::npos) fileName.replace(anInt, 1, "_"); 	//replace . by _ in fileName
	aStr = fileName + "_HDR.CSV";
	if ((outFile = fopen(aStr.c_str(), "w")) == NULL) {
		log.severe("Cannot create file " + aStr);
		return 6;
	}
	generateHeaderCSV(outFile, rinex, &log);
	fclose(outFile);
	rinex.clearHeaderData();
	/// 11 - Create output file for observation or navigation data
	switch (fileType) {
	case 'O':
		/// 11.1- If observation file, create output file (suffix name _OBS.CSV), and epoch by epoch read its data and print them. Close output file
		aStr = fileName + "_OBS.CSV";
		if ((outFile = fopen(aStr.c_str(), "w")) == NULL) {
			log.severe("Cannot create file " + aStr);
			return 6;
		}
		anInt = generateObsCSV(inFile, outFile, rinex, timeInterval, &log);
		fclose(outFile);
		break;
	case 'N':
		/// 11.2 - If navigation file, create output file (suffix name _xxxNAV.CSV), and epoch by epoch read its data and print them. Close output file
		switch (sysId) {
		case 'G':
			aStr = fileName + "_GPSNAV.CSV";
			if ((outFile = fopen(aStr.c_str(), "w")) == NULL) {
			log.severe("Cannot create file " + aStr);
			return 6;
			}
			anInt = generateGPSNavCSV(inFile, outFile, rinex, timeInterval, &log);
			fclose(outFile);
			break;
		case 'E':
			aStr = fileName + "_GALNAV.CSV";
			if ((outFile = fopen(aStr.c_str(), "w")) == NULL) {
				log.severe("Cannot create file " + aStr);
				return 6;
			}
			anInt = generateGalNavCSV(inFile, outFile, rinex, timeInterval, &log);
			fclose(outFile);
			break;
		case 'R':
			aStr = fileName + "_GLONAV.CSV";
			if ((outFile = fopen(aStr.c_str(), "w")) == NULL) {
				log.severe("Cannot create file " + aStr);
				return 6;
			}
			anInt = generateGloNavCSV(inFile, outFile, rinex, timeInterval, &log);
			fclose(outFile);
			break;
		case 'S':
			aStr = fileName + "_SBASNAV.CSV";
			if ((outFile = fopen(aStr.c_str(), "w")) == NULL) {
				log.severe("Cannot create file " + aStr);
				return 6;
			}
			anInt = generateSBASNavCSV(inFile, outFile, rinex, timeInterval, &log);
			fclose(outFile);
			break;
		default:	//should not happen
			log.severe("Unexpected system type for navigation file");
			return 7;
		}
		break;
	default:
			log.severe("Unexpected file type, different from Observation or Navigation");
			return 7;
	}
	fclose(inFile);
	return anInt>0? 0:5;
}

/**generateHeaderCSV prints header data for relevant records in CVS format
 *
 *@param out the already open print stream where header data will be printed in CVS format
 *@param rinex the RINEX data object, source of data to be printed
 *@param plog a pointer to a Logger to be used to record logging messages
 *@return  the exit status
 */
int generateHeaderCSV(FILE* out, RinexData &rinex, Logger* plog) {
	double db1, db2, db3;
	char ch1, ch2;
	vector <string> vs1;
	unsigned int ui1;
	string record, str1, str2, str3;
	int nrec = 0;
	plog->finer("Print CSV header records:");
	fprintf(out, "RINEX header record,Values\n");
	RinexData::RINEXlabel labelId = rinex.get1stLabelId();
	if (labelId != RinexData::VERSION) plog->warning("VERSION record has not data");
	while (labelId != RinexData::LASTONE) {
		record = rinex.idTOlbl(labelId);
		try {
			switch(labelId) {
			case RinexData::VERSION:
				rinex.setHdLnData(RinexData::VERSION);	//set version as per input file
				rinex.getHdLnData(labelId, db1, ch1, ch2);
				fprintf(out,"%s,%f,%c,%c\n", record.c_str(), db1, ch1, ch2);
				break;
			case RinexData::RUNBY:
				rinex.getHdLnData(labelId, str1, str2, str3);
				fprintf(out,"%s,%s,%s,%s\n", record.c_str(), str1.c_str(), str2.c_str(), str3.c_str());
				break;
			case RinexData::APPXYZ:
				rinex.getHdLnData(labelId, db1, db2, db3);
				fprintf(out,"%s,%lf,%lf,%lf\n", record.c_str(),  db1, db2, db3);
				break;
			case RinexData::TOBS:
			case RinexData::SYS:
				for (ui1 = 0; rinex.getHdLnData(labelId, ch1, vs1, ui1); ui1++) {
					fprintf(out,"%s,%c", record.c_str(), ch1);
					for(vector<string>::iterator it = vs1.begin() ; it != vs1.end(); ++it) fprintf(out,",%s", (*it).c_str());
					fprintf(out,"\n");
				}
				break;
			case RinexData::INT:
				rinex.getHdLnData(labelId, db1);
				fprintf(out,"%s,%lf\n", record.c_str(), db1);
				break;
			default:
				nrec--;
				break;
			}
			nrec++;
		} catch (string error) {
			plog->severe(error + string("Incorrect params in getHdLnData call for " + record));
		}
		labelId = rinex.getNextLabelId();
	}
	plog->finer("Records printed:" + to_string((long long) nrec));
	return nrec;
}

/*timeInInterval checks if the given time is in the given time interval.
 *
 *@param epochT the time in seconds to be checked
 *@param timeInterval the values defining the interval and the limits to be checked
 *@return true if the time interval is not defined or the given time is in the interval, false otherwise
*/
bool timeInInterval(double epochT, TimeIntervalParams &timeInterval) {
	if (timeInterval.fromTime || timeInterval.toTime) {
		if (timeInterval.fromTime && ( epochT < timeInterval.fromTimeTag)) return false;
		if (timeInterval.toTime && (epochT > timeInterval.toTimeTag)) return false;
	}
	return true;
}

/**generateObsCSV prints observation data in CVS format
 *
 *@param inFile the already open input RINEX observation file, positioned just after the End of Header record, in the first epoch 
 *@param outFile the already open print stream where header data will be printed in CVS format
 *@param rinex the RINEX data object, source of data to be printed
 *@param timeInterval the values defining the interval and the limits to be checked
 *@param plog a pointer to a Logger to be used to record logging messages
 *@return  the number of epochs transferred to the CSV file
 */
int generateObsCSV(FILE *inFile, FILE* outFile, RinexData &rinex, TimeIntervalParams &timeInterval, Logger* plog) {
	int anInt;		//a general purpose int variable
	string aStr;	//a general purpose string variable
	double aDouble;	//a general purpose double variable
	char sys;
	int week, rdStat, sat, lol, strg;
	double tow, value, tTag;
	string obsType;
	int nrec = 0;
	plog->finer("Print CSV observation epochs:");
	try {
		fprintf(outFile, "Week,TOW,Sys,Sat,Obs,Value,LoL,Strg\n");
		while ((rdStat = rinex.readObsEpoch(inFile)) != 0) {
			if (rdStat == 1 && timeInInterval(rinex.getEpochTime(week, tow, aDouble, anInt), timeInterval) && rinex.filterObsData()) {	//Epoch observables and data are well formatted and it remains data after filtering
				nrec++;
				for (unsigned int index = 0; rinex.getObsData(sys, sat, obsType, value, lol, strg, tTag, index); index++) {
					fprintf(outFile, "%d,%lf,%c,%d,%s,%lf,%d,%d\n", week, tow, sys, sat, obsType.c_str(), value, lol, strg);
				}
			}
		}
	} catch (string error) {
		plog->severe(error);
	}
	plog->finer("Obs epochs to CSV:" + to_string((long long) nrec));
	return nrec;
}

/**generateGPSNavCSV prints GPS navigation data in CVS format
 *
 *@param inFile the already open input RINEX navigation file, positioned just after the End of Header record, in the first epoch 
 *@param outFile the already open print stream where header data will be printed in CVS format
 *@param rinex the RINEX data object, source of data to be printed
 *@param timeInterval the values defining the interval and the limits to be checked
 *@param plog a pointer to a Logger to be used to record logging messages
 *@return  the exit status
 */
int generateGPSNavCSV(FILE *inFile, FILE* outFile, RinexData &rinex, TimeIntervalParams &timeInterval, Logger* plog) {
	int anInt;		//a general purpose int variable
	string aStr;	//a general purpose string variable
	double aDouble;	//a general purpose double variable
	char sys;
	int week, rdStat, sat;
	double tow, tTag, bo[8][4];
	int nrec = 0;
	plog->finer("Print CSV GPS navigation epochs:");
	try {
		fprintf(outFile, "Sys,Sat,Week,TOW,Af0,Af1,Af2,IODE,Crs,Delta N,M0,Cuc,e,Cus,sqrt(A),Toe,Cic,OMEGA0,Cis,i0,Crc,W,WDOT,IDOT,Codes on L2,GPS Week,L2 P flag,SV accuracy,SV health,TGD,IODC,Transm. time,Fit interval\n");
		while ((rdStat = rinex.readNavEpoch(inFile)) != 0) {
			if ((rdStat == 1 || rdStat == 2) && timeInInterval(rinex.getEpochTime(week, tow, aDouble, anInt), timeInterval) && rinex.filterNavData()) {	//Epoch nav. data are well formatted
				if (rinex.getNavData(sys, sat, bo, tTag, 0) && sys == 'G') {
					nrec++;
					fprintf(outFile, "%c,%d,%d,%lf", sys, sat, week, tow);
					for (int j = 1; j != 4; j++) fprintf(outFile, ",%19.12E", bo[0][j]);
					for (int i = 1; i != 7; i++)
						for (int j = 0; j != 4; j++) fprintf(outFile, ",%19.12E", bo[i][j]);
					for (int j = 0; j != 2; j++) fprintf(outFile, ",%19.12E", bo[7][j]);
					fprintf(outFile, "\n");
					rinex.clearNavData();
				} else plog->warning("Expected GPS epoch, but selected an " + string(1, sys) + " sat.");
			}
		}

	} catch (string error) {
		plog->severe(error);
	}
	plog->finer("GPS nav. epochs to CSV:" + to_string((long long) nrec));
	return nrec;
}

/**generateGalNavCSV prints Galileo navigation data in CVS format
 *
 *@param inFile the already open input RINEX navigation file, positioned just after the End of Header record, in the first epoch 
 *@param outFile the already open print stream where header data will be printed in CVS format
 *@param rinex the RINEX data object, source of data to be printed
 *@param timeInterval the values defining the interval and the limits to be checked
 *@param plog a pointer to a Logger to be used to record logging messages
 *@return  the exit status
 */
int generateGalNavCSV(FILE *inFile, FILE* outFile, RinexData &rinex, TimeIntervalParams &timeInterval, Logger* plog) {
	int anInt;		//a general purpose int variable
	string aStr;	//a general purpose string variable
	double aDouble;	//a general purpose double variable
	char sys;
	int week, rdStat, sat;
	double tow, tTag, bo[8][4];
	int nrec = 0;
	plog->finer("Print CSV Galileo navigation epochs:");
	try {
		fprintf(outFile, "Sys,Sat,Week,TOW,Af0,Af1,Af2,IODE,Crs,Delta N,M0,Cuc,e,Cus,sqrt(A),Toe,Cic,OMEGA0,Cis,i0,Crc,W,WDOT,IDOT,Data sources,Gal Week,SISA,SV health,BGD E5a/E1,BGD E5b/E1,Transm. time\n");
		while ((rdStat = rinex.readNavEpoch(inFile)) != 0) {
			if ((rdStat == 1 || rdStat == 2) && timeInInterval(rinex.getEpochTime(week, tow, aDouble, anInt), timeInterval) && rinex.filterNavData()) {	//Epoch nav. data are well formatted
				if (rinex.getNavData(sys, sat, bo, tTag, 0) && sys == 'E') {
					nrec++;
					fprintf(outFile, "%c,%d,%d,%lf", sys, sat, week, tow);
					for (int j = 1; j != 4; j++) fprintf(outFile, ",%19.12E", bo[0][j]);
					for (int i = 1; i != 5; i++)
						for (int j = 0; j != 4; j++) fprintf(outFile, ",%19.12E", bo[i][j]);
					for (int j = 0; j != 3; j++) fprintf(outFile, ",%19.12E", bo[5][j]);
					for (int j = 0; j != 4; j++) fprintf(outFile, ",%19.12E", bo[6][j]);
					fprintf(outFile, ",%19.12E\n", bo[7][0]);
					rinex.clearNavData();
				} else plog->warning("Expected GALILEO epoch, but selected an " + string(1, sys) + " sat.");
			}
		}

	} catch (string error) {
		plog->severe(error);
	}
	plog->finer("Galileo nav. epochs to CSV:" + to_string((long long) nrec));
	return nrec;
}

/**generateGloNavCSV prints GLONASSo navigation data in CVS format
 *
 *@param inFile the already open input RINEX navigation file, positioned just after the End of Header record, in the first epoch 
 *@param outFile the already open print stream where header data will be printed in CVS format
 *@param rinex the RINEX data object, source of data to be printed
 *@param timeInterval the values defining the interval and the limits to be checked
 *@param plog a pointer to a Logger to be used to record logging messages
 *@return  the exit status
 */
int generateGloNavCSV(FILE *inFile, FILE* outFile, RinexData &rinex, TimeIntervalParams &timeInterval, Logger* plog) {
	int anInt;		//a general purpose int variable
	string aStr;	//a general purpose string variable
	double aDouble;	//a general purpose double variable
	char sys;
	int week, rdStat, sat;
	double tow, tTag, bo[8][4];
	int nrec = 0;
	plog->finer("Print CSV GLONASS navigation epochs:");
	try {
		fprintf(outFile, "Sys,Sat,Week,TOW,-TauN,+GammaN,Msg.frm.t,Sat.X,Sat.vel.X,Sat.acc.X,Sat.health,Sat.Y,Sat.vel.Y,Sat.acc.Y,Sat.frq.,Sat.Z,Sat.vel.Z,Sat.acc.Z,Age\n");
		while ((rdStat = rinex.readNavEpoch(inFile)) != 0) {
			if ((rdStat == 1 || rdStat == 2) && timeInInterval(rinex.getEpochTime(week, tow, aDouble, anInt), timeInterval) && rinex.filterNavData()) {	//Epoch nav. data are well formatted
				if (rinex.getNavData(sys, sat, bo, tTag, 0) && sys == 'R') {
					nrec++;
					fprintf(outFile, "%c,%d,%d,%lf", sys, sat, week, tow);
					for (int j = 1; j != 4; j++) fprintf(outFile, ",%19.12E", bo[0][j]);
					for (int i = 1; i != 4; i++)
						for (int j = 0; j != 4; j++) fprintf(outFile, ",%19.12E", bo[i][j]);
					fprintf(outFile, "\n");
					rinex.clearNavData();
				} else plog->warning("Expected GLONASS epoch, but selected an " + string(1, sys) + " sat.");
			}
		}

	} catch (string error) {
		plog->severe(error);
	}
	plog->finer("GLONASS nav. epochs to CSV:" + to_string((long long) nrec));
	return nrec;
}

/**generateSBASNavCSV prints SBAS navigation data in CVS format
 *
 *@param inFile the already open input RINEX navigation file, positioned just after the End of Header record, in the first epoch 
 *@param outFile the already open print stream where header data will be printed in CVS format
 *@param rinex the RINEX data object, source of data to be printed
 *@param timeInterval the values defining the interval and the limits to be checked
 *@param plog a pointer to a Logger to be used to record logging messages
 *@return  the exit status
 */
int generateSBASNavCSV(FILE *inFile, FILE* outFile, RinexData &rinex, TimeIntervalParams &timeInterval, Logger* plog) {
	int anInt;		//a general purpose int variable
	string aStr;	//a general purpose string variable
	double aDouble;	//a general purpose double variable
	char sys;
	int week, rdStat, sat;
	double tow, tTag, bo[8][4];
	int nrec = 0;
	plog->finer("Print CSV SBAS navigation epochs:");
	try {
		fprintf(outFile, "Sys,Sat,Week,TOW,aGf0,aGf1,Transm.time,Sat.X,Sat.vel.X,Sat.acc.X,Sat.health,Sat.Y,Sat.vel.Y,Sat.acc.Y,Sat.URA,Sat.Z,Sat.vel.Z,Sat.acc.Z,IODN\n");
		while ((rdStat = rinex.readNavEpoch(inFile)) != 0) {
			if ((rdStat == 1 || rdStat == 2) && timeInInterval(rinex.getEpochTime(week, tow, aDouble, anInt), timeInterval) && rinex.filterNavData()) {	//Epoch nav. data are well formatted
				if (rinex.getNavData(sys, sat, bo, tTag, 0) && sys == 'S') {
					nrec++;
					fprintf(outFile, "%c,%d,%d,%lf", sys, sat, week, tow);
					for (int j = 1; j != 4; j++) fprintf(outFile, ",%19.12E", bo[0][j]);
					for (int i = 1; i != 4; i++)
						for (int j = 0; j != 4; j++) fprintf(outFile, ",%19.12E", bo[i][j]);
					fprintf(outFile, "\n");
					rinex.clearNavData();
				} else plog->warning("Expected SBAS epoch, but selected an " + string(1, sys) + " sat.");
			}
		}
	} catch (string error) {
		plog->severe(error);
	}
	plog->finer("GBAS nav. epochs to CSV:" + to_string((long long) nrec));
	return nrec;
}
