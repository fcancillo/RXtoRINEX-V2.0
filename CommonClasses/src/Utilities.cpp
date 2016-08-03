/** @file Utilities.cpp
 * Contains the implementation of routines used in several places.
 */
#include "Utilities.h"
#include <sstream>
#include <algorithm>
#include <ctype.h>
#include <time.h>
#include <math.h>

/**getTokens gets tokens from a string separated by the given separator
 *
 * @param source a string to be split into tokens
 * @param separator the character used to separate tokens
 * @return a vector of strings containing the extracted tokens 
 */
vector<string> getTokens (string source, char separator) {
	string strBuf;
	stringstream ss(source);
	vector<string> tokensFound;
	while (getline(ss, strBuf, separator))
		if(strBuf.size() > 0) tokensFound.push_back(strBuf);
	return tokensFound;
}

/**isBlank checks if in the given char buffer all chars are blanks
 *
 * @param buffer the array of chars to check
 * @param n the length to check
 * @return true if all n chars are blanks, false otherwise
 */
bool isBlank (char* buffer, int n) {
	while (n-- > 0) if (*(buffer++) != ' ')  return false;
	return true;
}

/**formatGPStime format a GPS time point giving text GPS calendar data using time formats provided. 
 *
 * @param buffer the text buffer where calendar data are placed
 * @param bufferSize of the text buffer in bytes
 * @param fmtYtoM the format to be used for year, month, day, hour and minute (all int), as per strftime
 * @param fmtSec the format to be used for seconds (a double), as per sprintf
 * @param week the GPS week from 6/1/1980
 * @param tow the GPS time of week, or seconds from the beginning of the week
 */
void formatGPStime (char* buffer, int bufferSize, char* fmtYtoM, char * fmtSec, int week, double tow) {
	//get integer and fractional part of tow
	double intTow;
	double modTow;
	modTow = modf(tow, &intTow);
	//set GPS ephemeris 6/1/1980 adding given week and sec increment
	struct tm gpsEphe = { 0 };
	gpsEphe.tm_year = 80;
	gpsEphe.tm_mon = 0;
	gpsEphe.tm_mday = 6 + week * 7;
	gpsEphe.tm_hour = 0;
	gpsEphe.tm_min = 0;
	gpsEphe.tm_sec = 0 + (int) intTow;
	//recompute time
	mktime(&gpsEphe);
	//format data
	strftime (buffer, bufferSize, fmtYtoM, &gpsEphe);
	string yTOm = string(buffer);
	string fmt = "%s" + string(fmtSec);
	sprintf(buffer, fmt.c_str(), yTOm.c_str(), ((double) gpsEphe.tm_sec + modTow)); 
}

/**formatLocalTime gives text calendar data of local time using the format provided (as per strftime). 
 *
 * @param buffer the text buffer where calendar data are placed
 * @param bufferSize of the text buffer in bytes
 * @param fmt the format to be used for conversion, as per strftime
 */
void formatLocalTime (char* buffer, int bufferSize, char* fmt) {
	//get local time and format it as needed
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer, bufferSize, fmt, timeinfo);
}

/**getGPSweek compute number of weeks from the GPS ephemeris (6/1/1980) to a given GPS date and time
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @return the weeks from 6/1/1980 to the given date 
 */
int getGPSweek (int year, int month, int day, int hour, int min, float sec) {
	return (int) (getSecsGPSEphe (year, month, day, hour, min, sec) / 604800.0);	// 7d * 24h * 60min * 60sec = 604800
}

/**getGPSweek compute number of weeks from the GPS ephemeris (6/1/1980) to a given instant in seconds
 *
 * @param secs seconds from of the GPS ephemeris (6/1/1980 00:00:0.0)
 * @return the weeks from 6/1/1980 to the given instant 
 */
int getGPSweek (double secs) {
	return (int) (secs / 604800.0);	// 7d * 24h * 60min * 60sec = 604800
}

/**getGPStow compute the Time Of Week (seconds from the begeing of week at Sunday 00:00h ) for a given GPS date and time
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @return the TOW for the given date 
 */
double getGPStow (int year, int month, int day, int hour, int min, float sec) {
	return getSecsGPSEphe (year, month, day, hour, min, sec) -
			getGPSweek (year, month, day, hour, min, sec) * 604800.0;	// 7d * 24h * 60min * 60sec = 604800
}

/**getGPStow compute the Time Of Week (seconds from the begeing of week at Sunday 00:00h ) for a given instant
 *
 * @param secs seconds from of the GPS ephemeris (6/1/1980 00:00:0.0)
 * @return the TOW for the given instant 
 */
double getGPStow (double secs) {
	return secs - (double) getGPSweek (secs) * 604800.0;	// 7d * 24h * 60min * 60sec = 604800
}

/**setWeekTow compute GPS week and tow for a given GPS date and time
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @param week GPS week to be computed
 * @param tow GPS tow to be computed
 */
void setWeekTow (int year, int month, int day, int hour, int min, double sec, int & week, double & tow) {
	//get integer and fractional part of sec
	double intSec;
	double modSec;
	modSec = modf(sec, &intSec);
	//set GPS ephemeris 6/1/1980
	struct tm gpsEphe = { 0 };
	gpsEphe.tm_year = 80;
	gpsEphe.tm_mon = 0;
	gpsEphe.tm_mday = 6;
	gpsEphe.tm_hour = 0;
	gpsEphe.tm_min = 0;
	gpsEphe.tm_sec = 0;
	//set given date
	struct tm date = { 0 };
	date.tm_year = year - 1900;
	date.tm_mon = month - 1;
	date.tm_mday = day;
	date.tm_hour = hour;
	date.tm_min = min;
	date.tm_sec = 0 + (int) intSec;
	//compute time difference in integer seconds
	intSec = difftime(mktime(&date), mktime(&gpsEphe));
	week = int (intSec / 604800.0);
	tow = fmod (intSec, 604800.0) + modSec;
}

/**getSecsGPSEphe computes time instant in seconds from the GPS ephemeris (6/1/1980) to a given date and time
 *
 * @param year of the date
 * @param month of the date
 * @param day of the date
 * @param hour of the date
 * @param min of the date
 * @param sec second of the date
 * @return the seconds from 0h of 6/1/1980 to the given date 
 */
double getSecsGPSEphe (int year, int month, int day, int hour, int min, float sec) {
	//set GPS ephemeris 6/1/1980
	struct tm gpsEphe = { 0 };
	gpsEphe.tm_year = 80;
	gpsEphe.tm_mon = 0;
	gpsEphe.tm_mday = 6;
	gpsEphe.tm_hour = 0;
	gpsEphe.tm_min = 0;
	gpsEphe.tm_sec = 0;
	//set given date
	struct tm date = { 0 };
	date.tm_year = year - 1900;
	date.tm_mon = month - 1;
	date.tm_mday = day;
	date.tm_hour = hour;
	date.tm_min = min;
	date.tm_sec = 0 + (int) sec;
	//compute time difference
	return difftime(mktime(&date), mktime(&gpsEphe));
}

/**getSecsGPSEphe compute instant in seconds from the GPS ephemeris (6/1/1980) to a given date stated in GPS week and tow
 *
 * @param week the GPS week number (continuous, without roll over)
 * @param tow the time of week
 * @return the seconds from 0h of 6/1/1980 to the given date stated in GPS week and tow
 */
double getSecsGPSEphe (int week, double tow) {
	return (double) week * 604800.0 + tow; // 7d * 24h * 60min * 60sec = 604800
}

/**strToUpper converts the characters in the given string to upper case
 *
 * @param strToConvert the string to convert to upper case
 * @return the converted string 
 */
string strToUpper(string strToConvert) {
	string strConverted;
	for (string::iterator p = strToConvert.begin(); p!= strToConvert.end(); p++) strConverted += string(1,toupper(*p));
    return strConverted;
}

/**getTwosComplement converts a two's complement representation from a given number of bits to 32 bits.
 * Number is an integer containing a bit stream of nbits length which represents a nbits integer in two's complement, that is:
 * - if number is in the range 0 to 2^(nbits-1) is positive. Its value in a 32 bits representation is the same.
 * - if number is in the range 2^(nbits-1) to 2^(nbits)-1, is negative and must be converted subtracting 2^(nbits)
 * 
 * @param number the 32 bits pattern with the number to be interpreted
 * @param nbits	the number of significative bits in the pattern
 * @return the value of the number (its 32 bits representation)
 */
int getTwosComplement(unsigned int number, unsigned int nbits) {
	int value = number;
	if (nbits >= 32) return value;					//the conversion is imposible or not necessary
	if (number < ((unsigned int) 1 << (nbits-1))) return value;	//the number is posive, it do not need conversion
	return value - (1 << nbits);
}

/**getSigned converts a signed representation from a given number of bits to standard 32 bits signed representation (two's complement).
 * Number is an integer containing a bit stream of nbits length which represents a nbits signed integer, that is:
 * - if the most significant bit is 0, the number is positive. Its value in a 32 bits representation is the same.
 * - if the most significant bit is 1, the number is negative and must be converted to standard negative number of 32 bits
 * 
 * @param number the 32 bits pattern with the number to be interpreted
 * @param nbits	the number of significative bits in the pattern
 * @return the value of the number (its 32 bits representation)
 */
int getSigned(unsigned int number, int nbits) {
	unsigned int signMask;
	int value = number;
	if ((nbits <= 32) && (nbits > 0)) {		//the conversion is possible
		signMask = 1 << (nbits-1); 
		if ((number & signMask) != 0) value = - (int) (number - signMask); //the number is negative
	}
	return value;
}

/*reverseWord swap LSB given bits in the given word
 *
 * @param wordToReverse	a 32 bits word containing the bits to reverse
 * @param nBits	the number of bits in the word to reverse
 * @return a 32 bits word with the bits to reverse in reverse order
*/
unsigned int reverseWord(unsigned int wordToReverse, int nBits) {
	unsigned int reversed = 0;
	int shifts = nBits;
	while (shifts > 0) {
		reversed <<= 1;
		reversed |= wordToReverse & 0x00000001;
		wordToReverse >>= 1;
		shifts--;
	}
	return reversed;
}

/*getBits gets a number of bits starting at a given position of the bits stream passed.
*The bit stream is an array of 32 bits words, being bit position 0 of the stream the bit 0 of word 0,
*position 1 of the stream the bit 1 of word 0, and so on
*The extracted bits are returned in a 32 bits word.
*
 * @param stream the array of 32 bits words containing the bit stream
 * @param bitpos the position in the stream of the LSB to extract (bitpos = 0 is the position of the first bit)
 * @param len the number of bits to extract (from bitpos to bitpos+len-1). It shall be: 32 >= len >= 0
 * @return a 32 bits word with extracted bits, with stream bit bitpos in bit position 0 of this word 
 */
unsigned int getBits(unsigned int *stream, int bitpos, int len) {
	unsigned int bits = 0;
	for (int i=bitpos+len-1; i>=bitpos; i--) {
		bits = (bits << 1) | ((stream[i/32] >> i%32) & 0x01);
	}
	return bits;
}

