/** @file RTKobservation.h
 * Contains the definition of the RTKobservation class.
 * A RTKobservation object contains data for the header and epochs of the RTK files.
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
 *<p>V1.1	|2/2016	|Removed Logger param and resulting empty constructor and destructor
 */
#ifndef RTKOBSERVATION_H
#define RTKOBSERVATION_H

#include <string>
#include <stdio.h>

using namespace std;

/**RTKobservation class defines data to be used for storing and further printing of a RTK file header
 * and the position solution data of each epoch.
 *
 * A detailed definition of the format used for RTK data files can be found in the RTKLIB portal (http://www.rtklib.com/).
 */
class RTKobservation {
public:
	RTKobservation(string prg, string in);
	~RTKobservation();
	void setId(string prg, string in);
	void setMasks(double elev, double snr);
	void setStartTime();
	void setStartTime(int week, double tow);
	void setEndTime();
	void setEndTime(int week, double tow);
	void setPosition(int week, double tow, double x, double y, double z, int qlty, int nSat);
	void printHeader(FILE* out);
	void printSolution (FILE* out);
private:
	//RTK observation file header data
	string program;
	string inpFile;
	string posMode;
	string freqs;
	string solution;
	double elevMask;
	double snrMask;
	string ionosEst;
	string troposEst;
	string ambEst;
	string valThres;
	string ephemeris;
	int startWeek;
	double startTOW;
	int endWeek;
	double endTOW;
	//Solution data
	double xSol;
	double ySol;
	double zSol;
	int qSol;
	int nSol;
	//Time related data
	int gpsWeek;	//extended week number: 0 - no limit 
	double gpsTOW;	//time of week in seconds as estimated by the receiver
};
#endif