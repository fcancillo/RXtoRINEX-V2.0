/**  @file GNSSdataFromOSP.h
 * Contains the GNSSdataFromOSP class definition.
 * Using the methods provided, GNSS data acquired from OSP records are stored in RinexData objects (header data, observations data, etc.)
 * or RTKobservation data objects (header, measurements, etc.) to allow further printing in RINEX or RTK file formats. 
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
 *<p>V2.0	|2/2016	|Added functionality:
 *<p>				|-# Class name changed from GNSSdataAcq to GNSSdataFromOSP
 *<p>				|-#	To convert GPS navigation messages to RINEX broadcast orbit parameters, applying the conversion factors
 *<p>				|-# To adquire GLONASS navigation data from OSP messages
 */
#ifndef GNSSDATAFROMOSP_H
#define GNSSDATAFROMOSP_H

//from CommonClasses
#include "Logger.h"
#include "OSPMessage.h"
#include "RinexData.h"
#include "RTKobservation.h"

//@cond DUMMY
//Receiver and GPS specific data
/// The maximum number of channels in the receiver
#define MAXCHANNELS 32
///In navigation messages, the highest number of GPS subframe or GLONASS string having ephemeris data (almanac data excluded)
#define MAXSUBFR 5
///The maximum number of slots in the GLONASS constellation 
#define MAXGLOSLOTS 24
///Subrange for GLONASS satellite numbers in SiRFV receivers
#define FIRSTGLOSAT 70
#define LASTGLOSAT 83
#define MAXGLOSATS 14
///Subrange for GPS satellites numbers
#define FIRSTGPSSAT 1
#define LASTGPSSAT 32
#define MAXGPSSATS 31
///Subrange for SBAS satellites numbers
#define FIRSTSBASSAT 101
#define LASTSBASSAT 200
#define MAXSBASSATS 100

//Constants usefull for computations
const double LSPEED = 299792458.0;	//the speed of light
const double C1CADJ = 299792458.0;	//to adjust C1C (pseudorrange L1 in meters) = C1CADJ (the speed of light) * clkOff
const double L1CADJ = 1575420000.0;	//to adjust L1C (carrier phase in cycles) =  L1CADJ (L1 carrier frequency) * clkOff
const double L1WLINV = 1575420000.0 / 299792458.0; //the inverse of L1 wave length to convert m/s to Hz.
const double ThisPI = 3.1415926535898;

//a bit mask definition for the bits participating in the computation of parity (see GPS ICD)
//bit mask order: D29 D30 d1 d2 d3 ... d24 ... d29 d30
//parityBitMask[i] identifies bits participating (set to 1) or not (set to 0) in the computation of parity bit i.
const unsigned int parityBitMask[] = {0xBB1F3480, 0x5D8F9A40, 0xAEC7CD00, 0x5763E680, 0x6BB1F340, 0x8B7A89C0};
//Default value for unknown data
const string unknown ("UNKNOWN");
const string msgEOM (" error getting data after end of message: ");
const string msgMID8Ign ("MID8 ignored: ");
const string msgFew (" ignored: few SVs in solution");
//@endcond

/**GNSSdataFromOSP class defines data and methods used to acquire RINEX or RTK header and epoch data from a binary OSP file containing receiver messages.
 * Such header and epoch data can be used to generate and print RINEX or RTK files.
 *<p>
 * A program using GNSSdataFromOSP would perform the following steps:
 *	-# Declare a GNSSdataFromOSP object stating the receiver, the file name with the binary messages containing the data
 *		to be acquired, and optionally the logger to be used
 *	-# Acquire header data and save them into an object of RinexData or RTKobservation class using acqHeaderData methods
 *	-# Header data acquired can be used to generate / print RINEX or RTK files (see available methods in RinexData and RTKobservation classes for that)
 *	-# As header data may be sparse among the binary file, rewind it before performing any other data acquisition
 *	-# Acquire an epoch data and save them into an object of RinexData or RTKobservation class using acqEpochData methods
 *	-# Epoch data acquired can be used to generate / print RINEX or RTK file epoch (see available methods in RinexData and RTKobservation classes)
 *	-# Repeat above steps 5 and 6 while epoch data are available in the input file.
 *<p>
 * This version implements acquisition from binary files containing OSP messages collected from SiRFIV receivers.
 * Each OSP message starts with the payload length (2 bytes) and follows the n bytes of the message payload.
 *<p>
 *A detailed definition of OSP messages can be found in the document "SiRFstarIV (TM) One Socket Protocol 
 * Interface Control Document Issue 9".
 *<p>
 * Note that messages recorded in the OSP binary file have the structure described in this ICD for the receiver message packets,
 * except that the start sequence (A0 A3), the checksum, and the end sequence (B0 B3) have been removed from the packet.
 */
class GNSSdataFromOSP {
public:
	GNSSdataFromOSP(string rcv, int minxfix, bool bias, FILE* f, Logger * pl);
	GNSSdataFromOSP(string rcv, int minxfix, bool bias, FILE* f);
	~GNSSdataFromOSP(void);
	bool acqHeaderData(RinexData &);
	bool acqHeaderData(RTKobservation &);
	bool acqEpochData(RinexData &, bool, bool);
	bool acqEpochData(RTKobservation &);
	bool acqGLOparams();

private:
	string receiver;
	int minSVSfix;
	bool applyBias;
	int epochGPSweek;
	double epochGPStow;
	double epochClkBias;
	double epochClkDrift;
	FILE* ospFile;
	OSPMessage message;
	struct SubframeData {		//A type to store 50bps message data
		int sv;					//the satelite number
		unsigned int words[10];	//the ten words with nav data
	};
	SubframeData subfrmCh[MAXCHANNELS][MAXSUBFR];
	struct GLONASSslot {	//A type to store the GLONASS slot number for each satellite
		int rcvCh;			//The receiver channel tracking this GLONASS satellite
		int rcvSat;			//The number (range 80 to 94) assigned by receiver to this GLONASS satellite 
		int slot;			//The GLONASS slot number of this satellite, extracted from nav string 5 
/*
		GLONASSslot(int ch, int sat, int slt) {
			rcvCh = ch;
			rcvSat = sat;
			slot = slt;
		}
*/
	};
//	vector<GLONASSslot> chsatGLOslt;
	GLONASSslot satGLOslt[MAXGLOSATS];
	struct GLONASSfreq {	//A type to match GLONASS slots and carrier frequency numbers received in almanac strings
		int nA;				//The slot number for a set of almanac data 
		int strFhnA;		//The string number from where the carrier frequency number must be extracted,
	};
	GLONASSfreq nAhnA[MAXCHANNELS];	//for each channel, the nA and frequency data
	int carrierFreq[MAXGLOSLOTS];
	struct ChannelObs {		//storage for channel/satellite observables during one epoch
		char system;		//system identification
		int satPrn;			//satellite number
		double psedrng;		//pseudorange in m
		double carrPh;		//carrier phase in m
		double carrFq;		//carrier frequency in m/s
		double signalStrg;	//signal strength
		int limitOl;		//limit of liability
		int strgIdx;		//strength index
		double timeT;		//a time tag to identify these measurements
		//constructor
		ChannelObs(char sy, int sa, double ps, double ca, double cf, double si, int li, int st, double ti) {
			system = sy;
			satPrn = sa;
			psedrng = ps;
			carrPh = ca;
			carrFq = cf;
			signalStrg = si;
			limitOl = li;
			strgIdx = st;
			timeT = ti;
		}
	};
	vector<ChannelObs> chSatObs;
	//Constant data used to convert GPS broadcast navigation data to "true" values
	double GPS_SCALEFACTOR[8][4];	//the scale factors to apply to GPS broadcast orbit data to obtain ephemeris (see GPS ICD)
	double GPS_URA[16];			//the User Range Accuracy values corresponding to URA index in the GPS SV broadcast data (see GPS ICD)
	double GLO_SCALEFACTOR[4][4];	//the scale factors to apply to GLONASS broadcast orbit data to obtain ephemeris (see GLONASS ICD)
	//Logger
	Logger* plog;		//the place to send logging messages
	bool dynamicLog;	//true when created dynamically here, false when provided externally

	void setTblValues();
	bool checkGPSparity (unsigned int );
	bool checkGLOhamming (unsigned int* );
	unsigned int bitsSet(unsigned int );
	bool allGPSEphemReceived(int );
	bool extractGPSEphemeris(const unsigned int (&navW)[45], unsigned int &sat, int (&bom)[8][4]);
	bool extractGLOEphemeris(int ch, unsigned int &sat, double &tTag, int (&bom)[8][4]);
	void scaleGPSEphemeris(int (&bom)[8][4], double &tTag, double (&bo)[8][4]);
	void scaleGLOEphemeris(int (&bom)[8][4], double (&bo)[8][4]);
	bool allGLOEphemReceived(int );
	int getGLOstring(unsigned int (&stringW)[3]);
	int getGLOslot(int ch, int sat);

	bool getMID2PosData(RinexData &);
	bool getMID2PosData(RTKobservation &);
	bool getMID2xyz(float &, float &, float &, int &);
	bool getMID6RxData(RinexData &);
	bool getMID7TimeData(RinexData &);
	bool getMID7Interval(RinexData &);
	bool getMID8GPSNavData(int, int, RinexData &);
	bool getMID8GLONavData(int, int, RinexData &);
	bool getMID15NavData(RinexData &);
	bool getMID19Masks(RTKobservation &);
	bool getMID28ObsData(RinexData &, bool &);
	bool getMID70NavData(RinexData &);
};
#endif