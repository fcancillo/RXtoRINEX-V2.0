/** @file Utilities.h
 * Contains definition of routines used in several places.
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
 *<p>V2.0	|2/2016	|Added functions
 */
#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <vector>

using namespace std;

vector<string> getTokens (string source, char separator);			//extract tokens from a string
bool isBlank (char* buffer, int n);		//checks if all chars in the buffer are spaces
void formatGPStime (char* buffer, int bufferSize, char* fmtYtoM, char * fmtSec, int week, double tow); //convert to printable format the given GPS time
void formatLocalTime (char* buffer, int bufferSize, char* fmt);		//convert to printable format the computer current local time
int getGPSweek (int year, int month, int day, int hour, int min, float sec); //computes GPS weeks from the GPS ephemeris (6/1/1980) to a given date
int getGPSweek (double secs); //computes GPS weeks from the GPS ephemeris (6/1/1980) to a given instant
double getGPStow (int year, int month, int day, int hour, int min, float sec); //computes the TOW for a given date
double getGPStow (double secs); //computes the GPS TOW for a given instant
void setWeekTow (int year, int month, int day, int hour, int min, double sec, int & week, double & tow);
double getSecsGPSEphe (int year, int month, int day, int hour, int min, float sec); //compute instant in seconds from the GPS ephemeris (6/1/1980) to a given date and time
double getSecsGPSEphe (int week, double tow); //compute instant seconds from the GPS ephemeris (6/1/1980) to a given GPS time (week and tow)
string strToUpper(string strToConvert);
int getTwosComplement(unsigned int number, unsigned int nbits);
int getSigned(unsigned int number, int nbits);
unsigned int reverseWord(unsigned int wordToReverse, int nBits=32);
unsigned int getBits(unsigned int *stream, int bitpos, int len);
#endif