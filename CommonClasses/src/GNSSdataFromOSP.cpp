/** @file GNSSdataFromOSP.cpp
 * Contains the implementation of the GNSSdataFromOSP class for OSP data collected from SiRF receivers.
 *
 */
#include <stdio.h>

#include "GNSSdataFromOSP.h"
//from CommonClasses
#include "Utilities.h"

///Macro to check message payload length and to log an error message if not correct 
#define CHECK_PAYLOADLEN(LENGTH, ERROR_MSG) \
	if (message.payloadLen() != LENGTH) { \
		plog->warning(ERROR_MSG); \
	}
///Macro to check if the number of satellites in the fix is lower than required and to log an error message if true 
#define CHECK_SATSREQUIRED(NSV, ERROR_MSG) \
	if (NSV < minSVSfix) { \
		plog->warning(ERROR_MSG); \
		return false; \
	}

/**Constructs a GNSSdataFromOSP object using parameters passed.
 *
 *@param rcv the receiver name
 *@param minxfix the minimum of satellites required for a fix to be considered valid
 *@param applBias when true, apply clock bias obtained by receiver to correct observables, when false, do not apply them.
 *@param f the FILE pointer to the already open input OSP binary file
 *@param pl a pointer to the Logger to be used to record logging messages
 */
GNSSdataFromOSP::GNSSdataFromOSP(string rcv, int minxfix, bool applBias, FILE* f, Logger * pl) {
	receiver = rcv;
	minSVSfix = minxfix;
	applyBias = applBias;
	ospFile = f;
	plog = pl;
	dynamicLog = false;
	for (int i=0; i<MAXCHANNELS; i++)
		for (int j=0; j<MAXSUBFR; j++)
			subfrmCh[i][j].sv = 0;
	setTblValues();
}

/**Constructs a GNSSdataFromOSP object using parameters passed, and logging data into the stderr.
 *
 *@param rcv the receiver name
 *@param minxfix the minimum of satellites required for a fix to be considered
 *@param applBias when true, apply clock bias obtained by receiver to correct observables, when false, do not apply them.
 *@param f the FILE pointer to the the already open input OSP binary file
 */
GNSSdataFromOSP::GNSSdataFromOSP(string rcv, int minxfix, bool applBias, FILE* f) {
	receiver = rcv;
	minSVSfix = minxfix;
	applyBias = applBias;
	ospFile = f;
	for (int i=0; i<MAXCHANNELS; i++)
		for (int j=0; j<MAXSUBFR; j++)
			subfrmCh[i][j].sv = 0;
	plog = new Logger();
	dynamicLog = true;
	setTblValues();
}

/**Destroys a GNSSdataFromOSP object
 */
GNSSdataFromOSP::~GNSSdataFromOSP(void) {
	if (dynamicLog) delete plog;
}

/**acqHeaderData extracts data from the binary OSP file for RINEX file header.
 * The RINEX header data to be extracted from the binary file are:
 * - the receiver identification contained in the first MID6 message
 * - the initial X,Y,Z position contained in the first MID2 message
 * - the time of first epoch contained in the first valid MID7 message
 * - the measurement interval, computed as the time difference between two consecutive valid MID7 
 *<p>The method iterates over the input file extracting messages until above describe data are acquired
 * or it reaches the end of file.
 * <p>It logs at FINE level a message stating which header data have been acquired or not.
 *
 * @param rinex the RinexData object where data got from receiver will be placed
 * @return	true if all above described header data are properly extracted, false otherwise
 */
bool GNSSdataFromOSP::acqHeaderData(RinexData &rinex) {
	bool rxIdSet = false;	//identification of receiver not set
	bool apxSet = false;	//approximate position not set
	bool frsEphSet = false;	//first epoch time not set
	bool intrvBegin = false; //interval begin time has been stated		
	bool intrvSet = false;	//observations interval not set
	int mid;
	plog->info("RINEX header data acquisition:");
	while (message.fill(ospFile) &&		//there are messages in the binary file
			!(apxSet && rxIdSet && frsEphSet && intrvSet)) {	//not all header data have been adquired
		mid = message.get();		//get first byte (MID)
		switch(mid) {
		case 2:		//collect first MID2 data to obtain approximate position (X, Y, Z)
			if (!apxSet) apxSet = getMID2PosData(rinex);
			break;
		case 6:		//extract the software version
			if (!rxIdSet) rxIdSet = getMID6RxData(rinex);
			break;
		case 7:		//collect MID7 data for first epoch and the following MID7 data for interval
			if (!frsEphSet) {
				intrvBegin = frsEphSet = getMID7TimeData(rinex);
				rinex.setHdLnData(rinex.TOFO);
			}
			else if (!intrvBegin) intrvBegin = getMID7TimeData(rinex);
			else if (!intrvSet) intrvBegin = intrvSet = getMID7Interval(rinex);
			break;
		default:
			break;
		}
	}
	//log data sources available or not
	string logMessage = "Header data acquired:";
	logMessage += apxSet? " Aprox. position;" : ";";
	logMessage += frsEphSet? " 1st epoch time;" : ";";
	logMessage += intrvSet? " Observation interval;" : ";";
	logMessage += rxIdSet? " Receiver version" : "";
	plog->info(logMessage);
	return (apxSet && frsEphSet && rxIdSet && intrvSet);
}

/**acqHeaderData extracts data from the binary OSP file for a RTK file header.
 * The RTK header data to be extracted from the binary file are:
 * - the time of first computed solution contained in the first valid MID2 message
 * - the time of last computed solution contained in the last valid MID2 message
 * - the elevation and SNR masks used by the receiver contained in the first valid MID19 message
 *<p>
 * The method iterates over the input file extracting messages until above describe data are acquired
 * or it reaches the end of file.
 *<p>
 * It logs at FINE level a message stating which header data have been acquired or not.
 *
 * @param rtko the RTKobservation object where data got from receiver will be placed
 * @return true if all above described header data are properly extracted, false otherwise
 */
bool GNSSdataFromOSP::acqHeaderData(RTKobservation &rtko) {
	bool maskSet = false;	//mask data set
	bool fetSet = false;	//first epoch time set
	int mid;
	//acquire mask data and first and last epoch time
	plog->info("RTK header data acquisition:");
	while (message.fill(ospFile)) {	//there are messages in the binary file
		mid = message.get();		//get first byte (MID)
		switch(mid) {
		case 2:
			if (getMID2PosData(rtko)) {
				if (!fetSet)	{
					rtko.setStartTime();
					fetSet = true;
				}
				rtko.setEndTime();
			}
			break;
		case 19:	//collect MID19 with masks
			maskSet = getMID19Masks(rtko);
			break;
		default:
			break;
		}
	}
	//log data sources available or not
	string logMessage = "Header data adquired:";
	logMessage += fetSet? "1ts epoch time;" : ";";
	logMessage += maskSet? "Mask data" : "";
	plog->info(logMessage);
	return maskSet && fetSet;
}

/**acqEpochData extracts observation and time data from binary OSP file messages for a RINEX epoch.
 *<p>Epoch RINEX data are contained in a sequence of {MID28} [MID2] [MID7] messages.
 *<p>Each MID28 contains observables for a given satellite. Successive MID28 messages belong to the same epoch
 * if they have the same receiver time. In each epoch the receiver sends a MID28 message for each satellite
 * being tracked.
 *<p>After sending the epoch MID28 messages, the receiver would send a MID2 message containing the computed
 * position solution data.
 *<p>The epoch finishes with a MID7 message. It contains clock data for the current epoch, including its GPS time.
 * An unexpected end of epoch occurs when a MID28 message shows different receiver time from former received messages.
 * This event would be logged at info level. Former message data are discarded because no time solution for this epoch 
 * can be computed, and the receiver time is unknown.
 *<p>The method acquires data reading messages recorded in the binary file according to the above sequence. 
 * Epoch data are stored in the RINEX data structure for further generation/printing of RINEX observation records.
 * Method returns when it is read the last message (MID2 or MID7) of the current epoch.
 *<p>Ephemeris data messages (MID15, MID8, MID70) can appear in any place of the input message sequence. Their data
 * would be stored for further generation of the RINEX navigation file.
 *<p>Other messages in the input binary file are ignored.
 *
 * @param rinex the RinexObsData object where data got from receiver will be placed
 * @param useMID8G when true GPS navigation data will be acquired from MID8 messages, when false these data would be acquired from MID15
 * @param useMID8R when true GLONASS navigation data will be acquired from MID8 messages , when false these data would be acquired from MID70
 * @return true when observation data from an epoch messages have been acquired, false otherwise (End Of File reached)
 */
bool GNSSdataFromOSP::acqEpochData(RinexData &rinex, bool useMID8G, bool useMID8R) {
	int mid, ch, sv;
	bool sameEpoch;
	double anObservable;
	while (message.fill(ospFile)) {	//one message has been read from the binary file
		mid = message.get();		//get first byte (MID) from message
		switch(mid) {
		case 7:		//the Rx sends MID7 when position for current epoch is computed (after sending MID28 msgs)
			if (getMID7TimeData(rinex)) {
				plog->fine("Epoch " + to_string((long double) epochGPStow) + " sats=" + to_string((long long) chSatObs.size()));
				if(!chSatObs.empty()) {
					for (vector<ChannelObs>::iterator it = chSatObs.begin(); it != chSatObs.end(); it++) {
						//convert observables from the OSP units to RINEX units when necessary
						//and apply corrections due to clock bias, when requested
						anObservable = it->psedrng;		//unit are m
						if (applyBias && (anObservable != 0.0)) anObservable -= epochClkBias * C1CADJ;
						rinex.saveObsData(it->system, it->satPrn, "C1C", anObservable, it->limitOl, it->strgIdx, it->timeT);
						anObservable = it->carrPh * L1WLINV;	//convert from initial unit (m) to cycles
						if (applyBias && (anObservable != 0.0)) anObservable -= epochClkBias * L1CADJ;
						rinex.saveObsData(it->system, it->satPrn, "L1C", anObservable, it->limitOl, it->strgIdx, it->timeT);
						anObservable = it->carrFq * L1WLINV;	//convert from initial unit (m/s) to Hz
						if (applyBias && (anObservable != 0.0)) anObservable -=  epochClkDrift;
						rinex.saveObsData(it->system, it->satPrn, "D1C", anObservable, it->limitOl, it->strgIdx, it->timeT);
						rinex.saveObsData(it->system, it->satPrn, "S1C", it->signalStrg, it->limitOl, it->strgIdx, it->timeT);
					}
					chSatObs.clear();
					return true;
				}
			}
			break;
		case 8:		//collect 50BPS ephemerides data in MID8
			if (useMID8G || useMID8R) {
				try {
					//extract channel number an satellite number from the OSP message
					ch = (int) message.get();
					if (ch>=0 && ch<MAXCHANNELS) {	//channel in range, continue data extraction
						sv = (int) message.get();	//the satellite number
						if ((sv >= FIRSTGPSSAT) && (sv <= LASTGPSSAT)) {
							if (useMID8G) getMID8GPSNavData(ch, sv, rinex);
						} else if ((sv >= FIRSTGLOSAT) && (sv <= LASTGLOSAT)) {	//it is a GLONASS satellite SirfV
							if (useMID8R) getMID8GLONavData(ch, sv, rinex);
						} else {
							plog->warning(msgMID8Ign + " satellite number out of GPS, GLONASS ranges:" + to_string((long long) sv));
						}
					} else plog->warning(msgMID8Ign + "channel not in range");
				} catch (int error) {
					plog->severe(msgMID8Ign + msgEOM + to_string((long long) error));
				}
			}
			break;
		case 15:	//collect complete GPS ephemerides data in MID15
			if (!useMID8G) getMID15NavData(rinex);
			break;
		case 28:	//collect satellite measurements from a channel in MID28
			if (getMID28ObsData(rinex, sameEpoch)) {	//message data are correct and have been stored
				if (!sameEpoch) {	//last data stored belong to a new epoch, and no MID7 has arrived!
					//as no MID7 has been received, the epoch time is not availble and current epoch observables shall be discarded 
					plog->warning("Epoch " + to_string((long double)chSatObs[0].timeT) + " ignored: MID7 lost");
					chSatObs.erase(chSatObs.begin(), chSatObs.end() - 1);
				}
			}
			break;
		case 70:	//collect complete GLONASS ephemerides data in MID70
			if (!useMID8R) getMID70NavData(rinex);
			break;
		default:
			break;
		}
	}
	return false;
}

/**acqGLOparams acquires some relevant parameters from GLONASS navigation data in MID8 messages
 *<p>GLONASS navigation data contains:
 * - the slot number of the satellite being tracked in a given receiver channel
 * - the carrier frequency number for each slot
 *<p>Such parameters are needed to stablish correpondence between data provided by receiver
 * and used in the GNSS data processing.
 *
 * @return true if data properly extracted, false otherwise  (End Of File reached)
 */
bool GNSSdataFromOSP::acqGLOparams() {
	int mid, ch, sat, strNum, n, nA, hnA;
	unsigned int gloStrg[3];		//a place to store the 84 bits of the GLONASS nav string
	vector<GLONASSslot>::iterator itSlot;
	char txtBuffer[80];
	bool dataAcq = false;

	rewind(ospFile);
	plog->info("Acquisition of GLONASS parameters:");
	try {
		while (message.fill(ospFile)) {	//a message has been read from the binary file
			mid = message.get();		//get first byte (MID) from message
			if (mid == 8) {
				CHECK_PAYLOADLEN(43,"MID8 msg len <> 43")
				ch = (int) message.get();
				if (ch>=0 && ch<MAXCHANNELS) {	//channel in range, continue data extraction
					sat = (int) message.get();	//the satellite number
					if ((sat >= FIRSTGLOSAT) && (sat <= LASTGLOSAT)) {	//it is a GLONASS satellite (in SirfV), extract nav data params needed
						//get from message payload the GLONASS string and the string number
						strNum = getGLOstring(gloStrg);
						switch (strNum) {
						case 4:
							//get slot number (n) in string 4, bits 15-11
							//and store its first occurrence in the table of GLONASS satellites
							n = getBits(gloStrg, 10, 5);
							sat -= FIRSTGLOSAT;
							if (satGLOslt[sat].slot == 0) {	//if this table entry is empty 
								satGLOslt[sat].rcvCh = ch;
								satGLOslt[sat].slot = n;
							}
							break;
						case 6:
						case 8:
						case 10:
						case 12:
						case 14:
							//get from almanac data the slot number (nA) in bits 77-73 (see GLONASS ICD for details)
							//and prepare slot - carrier frequency table to receive the value corresponding to this slot (if not already received)
							nA = getBits(gloStrg, 72, 5);
							if (nA > 0 && nA <= MAXGLOSLOTS) {
								nAhnA[ch].nA = nA;
								nAhnA[ch].strFhnA = strNum + 1;	//set the string number where continuation data should come
							} else plog->warning("MID8 GLO almanac string " + to_string((long long) strNum) + " bad slot number = " + to_string((long long) nA));
							break;
						case 7:
						case 9:
						case 11:
						case 13:
						case 15:
							//check in the slot - carrier frequency table the expected string number in this channel
							//if current string is the expected one to provide the carrier frequency data, store it
							if (nAhnA[ch].strFhnA == strNum) {
								hnA = getBits(gloStrg, 9, 5);	//HnA in almanac: bits 14-10
								if (hnA >= 25) hnA -= 32;	//set negative values as per table 4.11 of the GLONASS ICD
								carrierFreq[nAhnA[ch].nA - 1] = hnA;
							}
							break;
						default:
							break;

						}
					}
				} else plog->warning(msgMID8Ign + "channel not in range");
			}
		}
		//log data acquired
		plog->finer("GLONASS slot numbers used (from string 4 in MID8):");
		for (int i=0; i<MAXGLOSATS; i++) {
			sprintf(txtBuffer, "->sv=%2d slot=%2d rxChannel=%2d ", i+FIRSTGLOSAT, satGLOslt[i].slot, satGLOslt[i].rcvCh);
			plog->finer(string(txtBuffer));
			dataAcq = true;
		}
		plog->finer("GLONASS carrier frequency numbers (from almanac in MID8):");
		for (int i = 0; i < MAXGLOSLOTS; i++) {
			sprintf(txtBuffer, "->slot=%2d frequency=%2d", i+1, carrierFreq[i]);
			plog->finer(string(txtBuffer));
			dataAcq = true;
		}
	} catch (int error) {
		plog->severe("MID8 GLO" + msgEOM + to_string((long long) error));
	}
	return  dataAcq;
}

/**acqEpochData acquires epoch position data from binary OSP file messages for RTK observation files.
 *<p>Epoch RTK data are contained in a MID2 message.
 *<p>The method skips messages from the input binary file until a MID2 message is read.
 * When this happens, it stores MID2 data in the RTKobservation object passed and returns.
 *
 * @param rtko the RTKobservation object where data acquired will be placed
 * @return true if epoch position data properly extracted, false otherwise  (End Of File reached)
 */
bool GNSSdataFromOSP::acqEpochData(RTKobservation &rtko) {
	int mid;
	while (message.fill(ospFile)) {	//one message has been read from the binary file
		mid = message.get();		//get first byte (MID) from message
		if ((mid == 2) && getMID2PosData(rtko)) return true;
	}
	return  false;
}

//PRIVATE METHODS
//===============
/**setTblValues set conversion parameter tables used to translate scaled normalized GPS message data to values in actual units.
 * Called by the construtors
 *
 */
void GNSSdataFromOSP::setTblValues() {
	//SV clock data
	GPS_SCALEFACTOR[0][0] = pow(2.0, 4.0);		//T0c
	GPS_SCALEFACTOR[0][1] = pow(2.0, -31.0);	//Af0: SV clock bias
	GPS_SCALEFACTOR[0][2] = pow(2.0, -43.0);	//Af1: SV clock drift
	GPS_SCALEFACTOR[0][3] = pow(2.0, -55.0);	//Af2: SV clock drift rate
	//broadcast orbit 1
	GPS_SCALEFACTOR[1][0] = 1.0;				//IODE
	GPS_SCALEFACTOR[1][1] = pow(2.0, -5.0);	//Crs
	GPS_SCALEFACTOR[1][2] = pow(2.0, -43.0) * ThisPI;	//Delta N
	GPS_SCALEFACTOR[1][3] = pow(2.0, -31.0) * ThisPI;	//M0
	//broadcast orbit 2
	GPS_SCALEFACTOR[2][0] = pow(2.0, -29.0);	//Cuc
	GPS_SCALEFACTOR[2][1] = pow(2.0, -33.0);	//e
	GPS_SCALEFACTOR[2][2] = pow(2.0, -29.0);	//Cus
	GPS_SCALEFACTOR[2][3] = pow(2.0, -19.0);	//sqrt(A) ????
	//broadcast orbit 3
	GPS_SCALEFACTOR[3][0] = pow(2.0, 4.0);		//TOE
	GPS_SCALEFACTOR[3][1] = pow(2.0, -29.0);	//Cic
	GPS_SCALEFACTOR[3][2] = pow(2.0, -31.0) * ThisPI;	//Omega0
	GPS_SCALEFACTOR[3][3] = pow(2.0, -29.0);	//Cis
	//broadcast orbit 4
	GPS_SCALEFACTOR[4][0] = pow(2.0, -31.0) * ThisPI;	//i0
	GPS_SCALEFACTOR[4][1] = pow(2.0, -5.0);	//Crc
	GPS_SCALEFACTOR[4][2] = pow(2.0, -31.0) * ThisPI;	//w
	GPS_SCALEFACTOR[4][3] = pow(2.0, -43.0) * ThisPI;	//w dot
	//broadcast orbit 5
	GPS_SCALEFACTOR[5][0] = pow(2.0, -43.0) * ThisPI;	//Idot ??
	GPS_SCALEFACTOR[5][1] = 1.0;				//codes on L2
	GPS_SCALEFACTOR[5][2] = 1.0;				//GPS week + 1024
	GPS_SCALEFACTOR[5][3] = 1.0;				//L2 P data flag
	//broadcast orbit 6
	GPS_SCALEFACTOR[6][0] = 1.0;				//SV accuracy (index)
	GPS_SCALEFACTOR[6][1] = 1.0;				//SV health
	GPS_SCALEFACTOR[6][2] = pow(2.0, -31.0);	//TGD
	GPS_SCALEFACTOR[6][3] = 1.0;				//IODC
	//broadcast orbit 7
	GPS_SCALEFACTOR[7][0] = 0.01;	//Transmission time of message in sec x 100
	GPS_SCALEFACTOR[7][1] = 1.0;	//Fit interval
	GPS_SCALEFACTOR[7][2] = 0.0;	//Spare
	GPS_SCALEFACTOR[7][3] = 0.0;	//Spare
	//GPS_URA table to obtain in meters the value associated to the satellite accuracy index
	//fill GPS_URA table as per GPS ICD 20.3.3.3.1.3
	GPS_URA[0] = 2.0;		//0
	GPS_URA[1] = 2.8;		//1
	GPS_URA[2] = 4.0;		//2
	GPS_URA[3] = 5.7;		//3
	GPS_URA[4] = 8.0;		//4
	GPS_URA[5] = 11.3;		//5
	GPS_URA[6] = pow(2.0, 4.0);		//<=6
	GPS_URA[7] = pow(2.0, 5.0);
	GPS_URA[8] = pow(2.0, 6.0);
	GPS_URA[9] = pow(2.0, 7.0);
	GPS_URA[10] = pow(2.0, 8.0);
	GPS_URA[11] = pow(2.0, 9.0);
	GPS_URA[12] = pow(2.0, 10.0);
	GPS_URA[13] = pow(2.0, 11.0);
	GPS_URA[14] = pow(2.0, 12.0);
	GPS_URA[15] = 6144.00;
	//SV clock data
	GLO_SCALEFACTOR[0][0] = 1;		//T0c
	GLO_SCALEFACTOR[0][1] = pow(2.0, -30.0);		//-TauN
	GLO_SCALEFACTOR[0][2] = pow(2.0, -40.0);		//+GammaN
	GLO_SCALEFACTOR[0][3] = 1;					//Message frame time
	//broadcast orbit 1
	GLO_SCALEFACTOR[1][0] = pow(2.0, -11.0);	//X
	GLO_SCALEFACTOR[1][1] = pow(2.0, -20.0);	//Vel X
	GLO_SCALEFACTOR[1][2] = pow(2.0, -30.0);	//Accel X
	GLO_SCALEFACTOR[1][3] = 1;					//SV Health
	//broadcast orbit 2
	GLO_SCALEFACTOR[2][0] = pow(2.0, -11.0);	//Y
	GLO_SCALEFACTOR[2][1] = pow(2.0, -20.0);	//Vel Y
	GLO_SCALEFACTOR[2][2] = pow(2.0, -30.0);	//Accel Y
	GLO_SCALEFACTOR[2][3] = 1;					//Frequency number
	//broadcast orbit 3
	GLO_SCALEFACTOR[3][0] = pow(2.0, -11.0);	//Z
	GLO_SCALEFACTOR[3][1] = pow(2.0, -20.0);	//Vel Z
	GLO_SCALEFACTOR[3][2] = pow(2.0, -30.0);	//Accel Z
	GLO_SCALEFACTOR[3][3] = 1;					//Age of oper.
	//set tables to 0
	memset(subfrmCh, 0, sizeof subfrmCh);
	memset(satGLOslt, 0, sizeof satGLOslt);
	memset(carrierFreq, 0, sizeof carrierFreq);
	memset(nAhnA, 0, sizeof nAhnA);
}

/**getMID2PosData gets position solution data from a MID2 message and store them into "APPROX POSITION XYZ" record of a RinexData object.
 *
 *@param rinex the object where acquired data are stored
 *@return true if data properly extracted (correct message length and satellites in solution greather than minimum), false otherwise
 */
bool GNSSdataFromOSP::getMID2PosData(RinexData &rinex) {
	float x, y, z;
	int nsv;
	bool status;
	try {
		if (status = getMID2xyz (x, y, z, nsv)) status = rinex.setHdLnData(RinexData::APPXYZ, x, y, z);
	} catch (string error) {
		plog->severe(error + " in getMID2");
		return false;
	}
	return status;
}

/**getMID2PosData gets position solution data (X, Y, Z coordinates and time) from a MID2 message and store them into a RTKobservation object. 
 *
 *@param rtko the object where acquired data are stored
 *@return true if data properly extracted (correct message length and satellites in solution greather than minimum), false otherwise
 */
bool GNSSdataFromOSP::getMID2PosData(RTKobservation &rtko) {
	float x, y, z;
	int nsv;
	bool status;
	//it is assumed that "quality" is 5. No data exits in OSP messages to obtain it
	if (status = getMID2xyz (x, y, z, nsv)) rtko.setPosition(epochGPSweek, epochGPStow, (double) x, (double) y, (double) z, 5, nsv);
	return status;
}

/**getMID2xyz gets position solution data (X, Y, Z coordinates and time) from a MID2 message.
 * X, Y, Z data are provided througth parameters, time data are updated in the object (epoch week and TOW)
 *
 *@param x the x coordinate
 *@param y the y coordinate
 *@param z the z coordinate
 *@param nsv the number of satellites used to compute the solution
 *@return true if data properly extracted (correct message length and satellites in solution greather than minimum), false otherwise
 */
bool GNSSdataFromOSP::getMID2xyz(float &x, float &y, float &z, int &nsv) {
	char msgBuf[100];
	CHECK_PAYLOADLEN(41,"MID2 msg len <> 41")
	//extract X, Y, Z for the rinex header
	try {
		x = (float) message.getInt();	//get X
		y = (float) message.getInt();	//get Y
		z = (float) message.getInt();	//get Z
		message.skipBytes(9);	//skip from vX to Mode2: 3*2S 3*1U
		epochGPSweek = (int) message.getUShort() + 1024;
		epochGPStow = (double) message.getInt() / 100.0;	//get GPS TOW (scaled by 100)
		//check if fix has the minimum SVs required
		nsv = message.get();
		CHECK_SATSREQUIRED(nsv, "MID2" + msgFew)
		sprintf(msgBuf, "MID2 tow=%g x=%g y=%g z=%g", epochGPStow, x, y, z);
		plog->finer(string(msgBuf));
	} catch (int error) {
		plog->severe("MID2 " + msgEOM + to_string((long long) error));
		return false;
	}
	return true;
}


/**getMID6RxData gets receiver identification data from a MID6 message. Data acquired are stored in the RECEIVER record of the RinexData object.
 *
 *@param rinex the class instance where data are stored
 *@return true if data properly extracted, false otherwise
 */
bool GNSSdataFromOSP::getMID6RxData(RinexData &rinex) {
	//Note: current structure of this message does not correspond with what is stated in ICD
	string swVersion;
	string swCustomer;
	int SWversionLen;
	int SWcustomerLen;
	int msgLen;
	char c;
	try {
		SWversionLen = message.get();
		SWcustomerLen = message.get();
		//verify length of fields and message
		msgLen = 1 + 2 + SWversionLen + SWcustomerLen;
		CHECK_PAYLOADLEN(msgLen,"In MID6, message/receiver/customer length do not match")
		//extract swVersion from message char by char
		while (SWversionLen-- > 0) {
			c = (char) (message.get() & 0xFF);
			swVersion += c;
		}
		//extract swCustomer from buffer
		while (SWcustomerLen-- > 0) {
			c = (char) (message.get() & 0xFF);
			swCustomer += c;
		}
		rinex.setHdLnData(RinexData::RECEIVER, swVersion, receiver, swCustomer);
	} catch (int error) {
		plog->severe("MID6" + msgEOM + to_string((long long) error));
		return false;
	} catch (string error) {
		plog->severe(error + " in getMID6");
		return false;
	}
	plog->finer("MID6 swV=" + swVersion + " swC=" + swCustomer);
	return true;
}

/**getMID7TimeData gets GPS time data from a MID7 message. Data acquired are stored as current epoch time into the RinexData object.
 *
 * @param rinex	the class instance where data will be stored
 * @return true if data properly extracted (correct message length and satellites in solution greather than minimum), false otherwise
 */
bool GNSSdataFromOSP::getMID7TimeData(RinexData &rinex) {
	int sats;
	char msgBuf[100];
	CHECK_PAYLOADLEN(20,"MID7 msg len <> 20")
	try {
		epochGPSweek = (int) message.getUShort();		//get GPS Week (includes rollover)
		epochGPStow = (double) message.getUInt() / 100.0;	//get GPS TOW (scaled by 100)
		sats = (int) message.get();			//get number of satellites in the solution
		CHECK_SATSREQUIRED(sats, "MID7" + msgFew)
		epochClkDrift = (double) message.getUInt();	//get receiver clock drift (change rate of bias in Hz)
		//get receiver clock bias in nanoseconds (unsigned 32 bits int) and convert to seconds
		epochClkBias = (double) message.getUInt() * 1.0e-9;
		sprintf(msgBuf, "MID7 week=%d tow=%g bias=%g", epochGPSweek, epochGPStow, epochClkBias);
		plog->finer(string(msgBuf));
		if (!applyBias) {
			epochGPStow += epochClkBias;
			epochClkBias = 0.0;
		}
	} catch (int error) {
		plog->severe("MID7TimeData" + msgEOM + to_string((long long) error));
		return false;
	}
	rinex.setEpochTime(epochGPSweek, epochGPStow, epochClkBias, 0);
	return true;
}

/**getMID7Interval get GPS time data from MID7 message, compute interval from current epoch time and store them into INTERVAL record of the RinexData object.
 * 
 * @param rinex	the class instance where data will be stored
 * @return true if data properly extracted (correct message length and satellites in solution greather than minimum), false otherwise
 */
bool GNSSdataFromOSP::getMID7Interval(RinexData &rinex) {
	int week, sats;
	double tow, interval;
	CHECK_PAYLOADLEN(20,"MID7 msg len <> 20")
	try {
		week = (int) message.getUShort();		//get GPS Week (includes rollover)
		tow = (double) message.getUInt() / 100.0;	//get GPS TOW (scaled by 100)
		sats = (int) message.get();			//get number of satellites in the solution
		CHECK_SATSREQUIRED(sats, "MID7" + msgFew)
		interval = tow - epochGPStow + (double) ((week - epochGPSweek) * 604800.0);
		rinex.setHdLnData(rinex.INT, interval);
	} catch (int error) {
		plog->severe("MID7interval" + msgEOM + to_string((long long) error));
		return false;
	} catch (string error) {
		plog->severe(error + " in getMID7interval");
		return false;
	}
	plog->finer("MID7 interval=" + to_string((long double) interval));
	return true;
}

/**getMID8GPSNavData gets GPS navigation data from a MID 8 message and store them into satellite ephemeris (bradcast orbit data) of the RinexData object.
 * 
 * @param ch	the receiver channel number providing data
 * @param sv	the satellite number given by the receiver
 * @param rinex	the class instance where data are stored
 * @return true if data properly extracted (correct message length, channel number in range, and correct parity in navigation data), false otherwise
 */
bool GNSSdataFromOSP::getMID8GPSNavData(int ch, int sv, RinexData &rinex) {
	CHECK_PAYLOADLEN(43,"MID8 msg len <> 43")
	unsigned int wd[10];	//a place to store the ten words of OSP message
	unsigned int navW[45];	//a place to pack message data as per MID 15 (see SiRF ICD)
	unsigned int sat;		//the satellite number in the satellite navigation message
	int bom[8][4];		//the RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
	double tTag;		//the time tag for ephemeris data
	double bo[8][4];	//the RINEX broadcats orbit arrangement for satellite ephemeris
	bool parityOK;
	unsigned int subfrmID, pgID;
	char msgBuf[100];
	try {
		//read ten words with navigation data from the OSP message. Bits in each 32 bits word are: D29 D30 d1 d2 ... d30
		//that is: two last parity bits from previous word followed by the 30 bits of the current word
		for (int i=0; i<10; i++) wd[i] = message.getUInt();
		//check parity of each subframe word
		parityOK = checkGPSparity(wd[0]);
		for (int i=1; parityOK && i<10; i++) parityOK &= checkGPSparity(wd[i]);
		//if parity not OK, ignore all subframe data and return
		if (!parityOK) {
			plog->warning(msgMID8Ign + "GPS wrong parity");
			return false;
		}
		//remove parity from each GPS word getting the useful 24 bits
		//Note that when D30 is set, data bits are complemented (a non documented SiRF OSP feature)
		for (int i=0; i<10; i++)
			if ((wd[i] & 0x40000000) == 0) wd[i] = (wd[i]>>6) & 0xFFFFFF;
			else wd[i] = ~(wd[i]>>6) & 0xFFFFFF;
		//get subframe and page identification (page identification valid only for subframes 4 & 5)
		subfrmID = (wd[1]>>2) & 0x07;
		pgID = (wd[2]>>16) & 0x3F;
		sprintf(msgBuf, "MID8 GPS ch=%d sv=%d subfrm=%d page=%d", ch, sv, subfrmID, pgID);
		plog->finer(string(msgBuf));
		//only have interest subframes: 1,2,3 & page 18 of subframe 4 (pgID = 56 in GPS ICD Table 20-V)
		if ((subfrmID>0 && subfrmID<4) || (subfrmID==4 && pgID==56)) {
			subfrmID--;		//convert it to its index
			//store satellite number and message words
			subfrmCh[ch][subfrmID].sv = sv;
			for (int i=0; i<10; i++) subfrmCh[ch][subfrmID].words[i] = wd[i];
			//check if all ephemerides have been already received
			if (allGPSEphemReceived(ch)) {
				//if all 3 frames received , pack their data as per MID 15 (see SiRF ICD)
				for (int i=0; i<3; i++) {	//for each subframe index 0, 1, 2
					for (int j=0; j<5; j++) { //for each 2 WORDs group
						navW[i*15+j*3] = (subfrmCh[ch][i].words[j*2]>>8) & 0xFFFF;
						navW[i*15+j*3+1] = ((subfrmCh[ch][i].words[j*2] & 0xFF)<<8) | ((subfrmCh[ch][i].words[j*2+1]>>16) & 0xFF);
						navW[i*15+j*3+2] = subfrmCh[ch][i].words[j*2+1] & 0xFFFF;
					}
					//the exception is WORD1 (TLM word) of each subframe, whose data are not needed
					navW[i*15] = sv;
					navW[i*15+1] &= 0xFF;
				}
				//extract ephemeris data and store them into the RINEX instance
				if (extractGPSEphemeris(navW, sat, bom)) {
					scaleGPSEphemeris(bom, tTag, bo);
					rinex.saveNavData('G', sat, bo, tTag);
				}
				//TBW check if iono data exist & extract and store iono data in subfrmCh[ch][3]
				//clear storage
				for (int i=0; i<MAXSUBFR; i++) subfrmCh[ch][i].sv = 0;
			}
		}
	} catch (int error) {
		plog->severe("MID8" + msgEOM + to_string((long long) error));
		return false;
	}
	return true;
}

/**getMID8GLONavData gets GLONASS navigation data from a MID 8 message and store them into satellite ephemeris (bradcast orbit data) of the RinexData object.
 * 
 * @param ch	the receiver channel number providing data
 * @param sv	the satellite number given by the receiver
 * @param rinex	the class instance where data are stored
 * @return true if data properly extracted (correct message length, channel number in range, and correct parity in navigation data), false otherwise
 */
bool GNSSdataFromOSP::getMID8GLONavData(int ch, int sv, RinexData &rinex) {
	CHECK_PAYLOADLEN(43,"MID8 msg len <> 43")
	int sltNum, svx;				//the slot number (n) extracted from from string 4
	double tTag;			//the time tag for ephemeris data
	unsigned int gloStrg[3];//a place to store the 84 bits of a GLONASS string
	int bom[8][4];			//the RINEX broadcats orbit like arrangement for satellite ephemeris mantissa (as extracted from nav message)
	double bo[8][4];		//the RINEX broadcats orbit arrangement for satellite ephemeris (after applying scale factors)
	unsigned int strNum;	//the GLONASS string number
	string msgTxt;			//a place to build log messages
	unsigned int sat = sv;	//the satellite number in the satellite navigation message (slot number for GLONASS). Initially the one given by the receiver
	try {
		//get from message payload the GLONASS string and the string number
		strNum = getGLOstring(gloStrg);
		if (!checkGLOhamming (gloStrg)) {
			plog->warning(msgMID8Ign + "GLONASS wrong Hamming code");
			return false;
		}
		msgTxt = "MID8 GLONASS ch=" + to_string((long long) ch) + " sv=" + to_string((long long) sv) + " str=" + to_string((long long) strNum);
		//store satellite number and message words with inmediate data (strings # 1 to 5)
		if ((strNum > 0) && (strNum <= MAXSUBFR)) {
			//if string received is 4, it could be necessary to update inmediately the slot number
			if (strNum == 4) {
				//get slot number (n) in string 4, bits 15-11 and update the table of GLONASS satellites
				sltNum = getBits(gloStrg, 10, 5);
				if ((sltNum >= 0) && (sltNum <= MAXGLOSATS)) {
					svx = sv - FIRSTGLOSAT;
					if (satGLOslt[svx].slot != sltNum) {
						plog->finer(msgTxt
							+ " slot=" + to_string((long long) satGLOslt[svx].slot)
							+ " updated to slot=" + to_string((long long) sltNum));
						satGLOslt[svx].rcvCh = ch;
						satGLOslt[svx].slot = sltNum;
					}
				} else {
					msgTxt += " wrong slot=" + to_string((long long) sltNum); 
				}
			}
			strNum--;		//convert string number to its index
			//store satellite number and message words
			subfrmCh[ch][strNum].sv = sv;
			for (int i=0; i<3; i++) subfrmCh[ch][strNum].words[i] = gloStrg[i];
			for (int i=3; i<10; i++) subfrmCh[ch][strNum].words[i] = 0;
			//check if all ephemerides have been already received
			msgTxt += " saved";
			if (allGLOEphemReceived(ch)) {
				//extract ephemeris data and store them into the RINEX instance
				if (extractGLOEphemeris(ch, sat, tTag, bom)) {
					scaleGLOEphemeris(bom, bo);
					rinex.saveNavData('R', sat, bo, tTag);
				}
				//clear storage
				for (int i=0; i<MAXSUBFR; i++) subfrmCh[ch][i].sv = 0;
			}
		} else msgTxt += " ignored";
		plog->finer(msgTxt);
	} catch (int error) {
		plog->severe("MID8 GLO" + msgEOM + to_string((long long) error));
		return false;
	}
	return true;
}

/**getMID15NavData gets GPS ephemeris data from a MID 15 message
 * 
 * @param rinex	the class instance where data are stored
 * @return if data properly extracted (correct message length), false otherwise
 */
bool GNSSdataFromOSP::getMID15NavData(RinexData &rinex) {
	CHECK_PAYLOADLEN(92,"MID15 msg len <> 92")
	unsigned int navW[45];		//to store the 3x15 data items in the message
	unsigned int sat;		//the satellite number in the satellite navigation message
	int bom[8][4];		//the RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
	double tTag;		//the time tag for ephemeris data
	double bo[8][4];	//the RINEX broadcats orbit arrangement for satellite ephemeris
	int svID;
	string msgMID ("MID15 GPS ephemeris sv="); 
	try {
		svID = (int) message.get();
		msgMID += to_string((long long) svID);
		for (int i=0; i<45; i++) navW[i] = (unsigned int) message.getUShort();
		//set HOW bits in navW[1] and navW[2] to 0 (MID15 does not provide data from HOW)
		navW[1] &= 0xFF00;
		navW[2] &= 0x0003;
		//extract ephemerides data and store them into the RINEX instance
		if (!extractGPSEphemeris(navW, sat, bom)) {
			plog->warning(msgMID + " Wrong data");
			return false;
		}
		plog->finer(msgMID + " Ephemeris OK");
		//set bom[7][0] (MID15 has no HOW data) with current GPS seconds scaled by 100 as transmission time
		bom[7][0] = (int) (epochGPStow * 100.0);
		scaleGPSEphemeris(bom, tTag, bo);
		rinex.saveNavData('G', sat, bo, tTag);
	} catch (int error) {
		plog->severe("MID15" + msgEOM + to_string((long long) error));
		return false;
	}
	return true;
}

/**getMID19Masks gets receiver setting data (elevation and SNR masks) from a MID 19 message
 * 
 * @param rtko	the RTKobservation class instance where data are stored
 * @return true if data properly extracted (correct message length), false otherwise
 */
bool GNSSdataFromOSP::getMID19Masks(RTKobservation &rtko) {
	CHECK_PAYLOADLEN(65,"MID19 msg len <> 65")
	double elevationMask;
	double snrMask;
	try {
		message.skipBytes(19);	//skip from SubID to DOPmask: 1U 2U 3*1U 2S 6*1U 4U 1U
		elevationMask = (double) message.getShort();
		snrMask = (double) message.get();
		rtko.setMasks(elevationMask/10.0, snrMask);
	} catch (int error) {
		plog->severe("MID19" + msgEOM + to_string((long long) error));
		return false;
	}
	plog->finer("MID19 elevation=" + to_string((long double) elevationMask) + " s/n=" + to_string((long double) snrMask));
	return true;
}

/**getMID28ObsData gets observables / measurements (time, pseudorange, carrier phase, etc.) for a satellite from a MID28 message
 * 
 * @param rinex	the RinexData class instance where data are to be stored
 * @param sameEpoch is set to true when measurements belongs to the current epoch and have been stored in the rinex object, set to false otherwise
 * @return true if data properly extracted (correct message length and receiver gives confidence on observables), false otherwise
 */
bool GNSSdataFromOSP::getMID28ObsData(RinexData &rinex, bool &sameEpoch) {
	char sys;
	int channel, sv, satID, syncFlags, carrier2noise, strength, strengthIndex;
	unsigned short int deltaRangeInterval;
	double gpsSWtime, pseudorange, carrierFrequency, carrierPhase;
	char msgBuf[100];
	CHECK_PAYLOADLEN(56,"MID28 msg len <> 20")
	sameEpoch = false;
	try {
		//get data from message MID28
		channel = message.get();	
		message.getInt();			//a time tag not used
		sv = message.get();			//the satellite number assigned by the receiver
		if ((sv >= FIRSTGPSSAT) && (sv <= LASTGPSSAT)) {
			sys = 'G';
			satID = sv;
		} else if ((sv >= FIRSTGLOSAT) && (sv <= LASTGLOSAT)) {	//it is a GLONASS satellite SirfV
			sys = 'R';
			satID = getGLOslot(channel, sv);
		} else if ((sv >= FIRSTSBASSAT) && (sv <= LASTSBASSAT)) {			//it is a SBAS satellite
			sys = 'S';
			satID = sv - 100;
		} else {
			plog->warning("MID28 satellite number out of GPS, SBAS, GLONASS ranges:" + to_string((long long) sv));
			return false;
		}
		gpsSWtime = message.getDouble();
		pseudorange = message.getDouble();
		carrierFrequency = (double) message.getFloat(); //sign - ¿?
		carrierPhase = message.getDouble();
		message.getUShort();		//the timeIntrack is not used
		syncFlags = message.get();
		//get the signal strength as the worst of the C/N0 given
		carrier2noise = 0;
		strength = message.get();
		for (int i=1; i<10; i++)
			if ((carrier2noise = message.get()) < strength) strength = carrier2noise;
		deltaRangeInterval = message.getUShort();
	} catch (int error) {
		plog->severe("MID28 " + msgEOM + to_string((long long) error));
		return false;
	}
	sprintf(msgBuf,"MID28 tTag=%g ch=%2d sv=%2d sat=%c%02d psr=%g SynFlg=%02X ", gpsSWtime, channel, sv, sys, satID, pseudorange, syncFlags);
	//compute strengthIndex as per RINEX spec (5.7): min(max(strength / 6, 1), 9)
	strengthIndex = strength / 6;
	if (strengthIndex < 1) strengthIndex = 1;
	if (strengthIndex > 9) strengthIndex = 9;
	//TBW Phase Error Count could be used to compute LoL: if for this satellite PhEC != former PhEC, there is a slip in the carrierPhase
	if ((syncFlags & 0x01) != 0) {	//bit 0 is set only when acquisition is complete
		if ((syncFlags & 0x02) == 0) carrierPhase = 0.0;
		if ((syncFlags & 0x10) == 0) carrierFrequency = 0.0;
		chSatObs.push_back(ChannelObs(sys, satID, pseudorange, carrierPhase, carrierFrequency, (double) strength, 0, strengthIndex, gpsSWtime));
		sameEpoch = gpsSWtime == chSatObs[0].timeT;
		plog->finer(string(msgBuf) + "SAVED");
		return true;
	}
	plog->finer(string(msgBuf) + "IGNORED");
	return false;
}

/**getMID70NavData gets GLONASS ephemeris data from a MID 70 SID 12 message
 * 
 * @param rinex	the class instance where data are stored
 * @return if data properly extracted (correct SID data), false otherwise
 */
bool GNSSdataFromOSP::getMID70NavData(RinexData &rinex) {
	//----------
	//incomplete and not verified code
	//----------
	unsigned int sat;		//the satellite number in the satellite navigation message
	int bom[8][4];		//the RINEX broadcats orbit like arrangement for satellite ephemeris mantissa
	double tTag;		//the time tag for ephemeris data
	double bo[8][4];	//the RINEX broadcats orbit arrangement for satellite ephemeris
	int tauGPS, tauUTC, b1, b2, n4, kp, nSvs, day, time;
	bool validEphem;
	//CHECK_PAYLOADLEN(,"MID70 msg len <> ")
	try {
		if (message.get() != 12) return false;	//SID is not for a GLONASS Broadcast Ephemeris Response message
		if (message.get() != 1) return false;	//Fields TAU_GPS through KP are not valid, and some data becomes ambiguous
		tauGPS = message.getInt3();
		tauUTC = message.getInt();
		b1 = message.getShort();
		b2 = message.getShort();
		n4 = message.get();
		kp = message.get();
		nSvs = message.get();
		plog->finer("MID70 SID12 GLONASS ephem. for nSVs=" + to_string((long long) nSvs));
		while (nSvs > 0) {
			validEphem = message.get() == 1;	//Validity flag
			sat = message.get();	//Slot number
			if (validEphem && (sat > 0) && (sat <= MAXGLOSATS)) {	
				bom[2][3] = message.get();	//Frequency offset
				bom[1][3] = message.get();	//Satellite health
				day = message.getUShort();	//day number
				time = message.get() * 900;	//Ephemeris reference time
				tTag = getSecsGPSEphe(1996 + n4*4, 0, day, 0, 0, (float) time);	//convert ephmeris time (in GLONASS time) to an instant (use GPS ephemeris for convenience)
				tTag -= 3*60*60;	//correct GLONASS time to UTC
				bom[0][0] = (int) tTag;			//Toc
				bom[3][3] = message.get();		//Age of operational information
				bom[1][0] = message.getInt();	//Satellite position, X
				bom[2][0] = message.getInt();	//Satellite position, Y
				bom[3][0] = message.getInt();	//Satellite position, Z
				bom[1][1] = message.getInt3();	//Satellite velocity, X
				bom[2][1] = message.getInt3();	//Satellite velocity, Y
				bom[3][1] = message.getInt3();	//Satellite velocity, Z
				bom[1][2] = message.get();		//Satellite acceleration, X
				bom[2][2] = message.get();		//Satellite acceleration, Y
				bom[3][2] = message.get();		//Satellite acceleration, Z
				message.get();	//Group delay
				bom[0][1] = -message.getInt3();	//SV clock bias (sec) (-TauN) <- Correction to satellite clock with respect to GLONASS system time(-TauN?)
				bom[0][2] = carrierFreq[sat-1];			//SV relative frequency bias +GammaN
				bom[0][3] =  (int) tTag;	//Message frame time (tk+nd*86400) in seconds of the UTC week?
				bom[3][3] = 0;			//Age of oper. information (days) (E)
				scaleGLOEphemeris(bom, bo);
				rinex.saveNavData('R', sat, bo, tTag);
			} else plog->warning("GLONASS ephem. not valid for " + to_string((long long) sat));
			nSvs--;
		}
	} catch (int error) {
		plog->severe("MID70 SID12" + msgEOM + to_string((long long) error));
		return false;
	}
	return true;
}

/**checkGPSparity checks the parity of a GPS message subframe word using procedure in GPS ICD.
 * To check the parity, the six bits of parity are computed for the word contents, and then compared with the current parity in the 6 LSB of the word passed.
 *  
 * @param d The subframe word passed, where the two LSB bits of the previous word have been added  
 * @return true if parity computed is equal to the current parity in the six LSB of the word
 */
bool GNSSdataFromOSP::checkGPSparity (unsigned int d) {
	//d has de form: D29 D30 d1 .. d30)
	unsigned int toCheck = d;
	if ((d & 0x40000000) != 0) toCheck = (d & 0xC0000000) | (~d & 0x3FFFFFFF);
	//compute the parity of the bit stream
	unsigned int parity = 0;
	for (int i=0; i<6; i++) parity |= (bitsSet(parityBitMask[i] & toCheck) % 2) << (5-i);
	return parity == (d & 0x3F);
}

/**checkGLOhamming checks the GLONASS string for correct hamming code.
 *  
 * @param ps a pointer to the string to be checked contained in the 3 words pointed by pw   
 * @return true if code computed is equal to the current one in bits 1-8 of the string
 */
bool GNSSdataFromOSP::checkGLOhamming (unsigned int* ps) {
	//To be implemented
	return true;
};

/**bitsSet counts the number of bits set in a given 32 bits stream.
 * Used to compute the XOR of a bit stream for computing parity
 * 
 * @param lw The 32 bit stream to be processed (32 LSB in the long)
 * @return the number of bits set to 1 in lw 
 */
unsigned int GNSSdataFromOSP::bitsSet(unsigned int lw) {
	int count = 0;
	for (int i=0; i<32; i++) count += (int)((lw>>i) & 0x01);
	return count;
}

/**allGPSEphemReceived checks if all GPS ephemerides in a given channel have been received
 *All ephemerides have been received if subframes 1, 2 and 3 have been received, all subframes belong
 *to the same satellite (the satellite tracked by the channel has not changed), and their data belong to the same IOD (Issue Of Data)
 *
 * @param ch The data channel to be checked
 * @return true, if all data received, false otherwise
 */
bool GNSSdataFromOSP::allGPSEphemReceived(int ch) {
	bool allReceived =  (subfrmCh[ch][0].sv == subfrmCh[ch][1].sv)
						&& (subfrmCh[ch][0].sv == subfrmCh[ch][2].sv);
	//IODC (8LSB in subframe 1) must be equal to IODE in subframe 2 and IODE in subframe 3
	unsigned int iodcLSB = (subfrmCh[ch][0].words[7]>>16) & 0xFF;
	allReceived &= iodcLSB == ((subfrmCh[ch][1].words[2]>>16) & 0xFF);
	allReceived &= iodcLSB == ((subfrmCh[ch][2].words[9]>>16) & 0xFF);
	return allReceived;
}

/**allGLOEphemReceived checks if all GLONASS ephemerides in a given channel have been received
 *All ephemerides have been received if strings 1 to 5 have been received,  *All ephemerides have been received if subframes 1, 2 and 3 have been received, all subframes belong
 *to the same satellite (the satellite tracked by the channel has not changed), and their data belong to the same IOD (Issue Of Data)
 * 
 * @param ch The data channel to be checked
 * @return true, if all data received, false otherwise
 */
bool GNSSdataFromOSP::allGLOEphemReceived(int ch) {
	for(int i = 0; i < MAXSUBFR; i++) if (subfrmCh[ch][i].sv == 0) return false;
	return true;
}

/**extractGPSEphemeris extract satellite number and ephemeris from the given navigation message transmitted by GPS satellites.
 * The navigation message is an arrangement of the 3 x 10 x 24 bits of the GPS message data words (without parity) into
 * an array of 3 x 15 = 45 words (where 16 bits are effective). See SiRF ICD (A.5 Message # 15: Ephemeris Data) for details.
 * The ephemeris extracted are store into a RINEX broadcast orbit like arrangement consisting of  8 lines with 3 ephemeris parameters each.
 * This method stores the MANTISSA of each satellite ephemeris into a broadcast orbit array, whithout applying any scale factor.
 * 
 * @param navW an array of 45 elements containing the GPS data words of the navigation message arranged as explained in the SiRF ICD A.5
 * @param sv the satellite number
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 * @return true if data properly extracted (SV consistency in the channel data, version data consistency and receiver gives confidence on observables), false otherwise
 */
bool GNSSdataFromOSP::extractGPSEphemeris(const unsigned int (&navW)[45], unsigned int &sv, int (&bom)[8][4]) {
	sv = navW[0] & 0xFF;		//the SV identification
	//check for consistency in the channel data
	if (!((sv==(navW[15] & 0xFF)) && (sv==(navW[30] & 0xFF)))) {
		plog->info("Different SVs in the channel data");
		return false;
	}
	//check for version data consistency
	unsigned int iodcLSB = navW[10] & 0xFF;
	unsigned int iode1 = (navW[15+3]>>8) & 0xFF;
	unsigned int iode2 = navW[30+13] & 0xFF;
	if (!(iode1==iode2 && iode1==iodcLSB)) {
		plog->warning("Different IODs:SV <" + to_string((unsigned long long) sv) +
						"> IODs <" + to_string((unsigned long long) iodcLSB) +
						"," + to_string((unsigned long long) iode1) +
						"," + to_string((unsigned long long) iode2) + ">");
		return false;
	}
/* TBC: to analyze the following code for correctness
	//check for SVhealth
	unsigned int svHealth = (navW[4]>>10) & 0x3F;
	if ((svHealth & 0x20) != 0) {
		sprintf(errorBuff, "Wrong SV %2u health <%02hx>", sv, svHealth);
		plog->info(string(errorBuff));
		return false;
	}
*/
	//fill bom extracting data according SiRF ICD (see A.5 Message # 15: Ephemeris Data) 
	//broadcast line 0
	bom[0][0] = navW[11];	//T0C
	bom[0][1] = getTwosComplement(((navW[13] & 0x00FF)<<14) | ((navW[14]>>2) & 0x3FFF), 22);	//Af0
	bom[0][2] = getTwosComplement(((navW[12] & 0x00FF)<<8) | ((navW[13]>>8) & 0x00FF), 16);	//Af1
	bom[0][3] = getTwosComplement((navW[12]>>8) & 0x00FF, 8);								//Af2
	//broadcast line 1
	bom[1][0] = iode1;	//IODE
	bom[1][1] = getTwosComplement(((navW[15+3] & 0x00FF)<<8) | ((navW[15+4]>>8) & 0x00FF), 16);	//Crs
	bom[1][2] = getTwosComplement(((navW[15+4] & 0x00FF)<<8) | ((navW[15+5]>>8) & 0x00FF), 16);	//Delta n
	bom[1][3] = getTwosComplement(((navW[15+5] & 0x00FF)<<24) | ((navW[15+6] & 0xFFFF)<<8) | ((navW[15+7]>>8) & 0x00FF),32);//M0
	//broadcast line 2
	bom[2][0] = getTwosComplement(((navW[15+7] & 0x00FF)<<8) | ((navW[15+8]>>8) & 0x00FF), 16);			//Cuc
	bom[2][1] = (((navW[15+8] & 0x00FF)<<24) | ((navW[15+9] & 0xFFFF)<<8) | ((navW[15+10]>>8) & 0x00FF));	//e
	bom[2][2] = getTwosComplement(((navW[15+10] & 0x00FF)<<8) | ((navW[15+11]>>8) & 0x00FF), 16);		//Cus
	bom[2][3] = ((navW[15+11] & 0x00FF)<<24) | ((navW[15+12] & 0xFFFF)<<8) | ((navW[15+13]>>8) & 0x00FF);	//sqrt(A)
	//broadcast line 3
	bom[3][0] = ((navW[15+13] & 0x00FF)<<8) | ((navW[15+14]>>8) & 0x00FF);	//Toe
	bom[3][1] = getTwosComplement(navW[30+3], 16);						//Cic
	bom[3][2] = getTwosComplement(((navW[30+4] & 0xFFFF)<<16) | (navW[30+5] & 0xFFFF), 32);	//OMEGA
	bom[3][3] = getTwosComplement(navW[30+6], 16);						//CIS
	//broadcast line 4
	bom[4][0] = getTwosComplement(((navW[30+7] & 0xFFFF)<<16) | (navW[30+8] & 0xFFFF), 32);	//i0
	bom[4][1] = getTwosComplement(navW[30+9], 16);											//Crc
	bom[4][2] = getTwosComplement(((navW[30+10] & 0xFFFF)<<16) | (navW[30+11] & 0xFFFF), 32);	//w (omega)
	bom[4][3] = getTwosComplement(((navW[30+12] & 0xFFFF)<<8) | ((navW[30+13]>>8) & 0x00FF), 24);//w dot
	//broadcast line 5
	bom[5][0] = getTwosComplement((navW[30+14]>>2) & 0x03FFF, 14);	//IDOT
	bom[5][1] = (navW[3]>>4) & 0x0003;				//Codes on L2
	bom[5][2] = ((navW[3]>>6) & 0x03FF) + 1024L;	//GPS week#
	bom[5][3] = (navW[4]>>7) & 0x0001;				//L2P data flag
	//broadcast line 6
	bom[6][0] = navW[3] & 0x000F;			//SV accuracy
	bom[6][1] = (navW[4]>>10) & 0x003F;	//SV health
	bom[6][2] = getTwosComplement((navW[10]>>8) & 0x00FF, 8);//TGD
	bom[6][3] = iodcLSB | (navW[4] & 0x0300);		//IODC
	//broadcast line 7
	bom[7][0] = (((navW[1] & 0x00FF)<<9) | ((navW[2]>>7) & 0x01FF)) * 600;	//the 17 MSB of the Zcount in HOW (W2) converted to sec and scaled by 100
	bom[7][1] = (navW[15+14]>>7) & 0x0001;			//Fit flag
	bom[7][2] = 0;		//Spare. Not used
	bom[7][3] = iode2;	//Spare. Used for temporary store of IODE in subframe 3
	return true;
}

/**extractGLOEphemeris extract satellite number, time tag, and ephemeris from the given navigation message string transmitted by GLONASS satellites.
 * The ephemeris extracted are store into a RINEX broadcast orbit like arrangement consisting of  8 lines with 3 ephemeris parameters each.
 * This method stores the mantissa of each satellite ephemeris into a broadcast orbit array, whithout applying any scale factor.
 * 
 * @param ch the channel number
 * @param sv the satellite number passed, updated by the method to the slot number
 * @param tTag the time tag for ephemris
 * @param bom an array of broadcats orbit data containing the mantissa of each satellite ephemeris
 * @return true if data properly extracted (SV consistency in the channel data, version data consistency and receiver gives confidence on observables), false otherwise
 */
bool GNSSdataFromOSP::extractGLOEphemeris(int ch, unsigned int &sv, double &tTag, int (&bom)[8][4]) {
	sv = getBits(subfrmCh[ch][3].words, 10, 5);			//slot number (n) in string 4, bits 15-11
	if ((sv == 0) || (sv > MAXGLOSATS)) {
		plog->warning("50bps NAV ignored. In string 4 slot number out of range:" + to_string((long long) sv));
		return false;
	}
	int n4 = getBits(subfrmCh[ch][4].words, 31, 5);		//four-year interval number N4: bits 36-32 string 5
	int nt = getBits(subfrmCh[ch][3].words, 15, 11);	//day number NT: bits 26-16 string 4
	int tb = getBits(subfrmCh[ch][1].words, 69, 7);		//time interval index tb: bits 76-70 string 2
	tb *= 15*60;	//convert index to secs
	unsigned int tk =  getBits(subfrmCh[ch][0].words, 64, 12);	//Message frame time: bits 76-65 string 1
	int tkSec = (tk >> 7) *60*60;		//hours in tk to secs.
	tkSec += ((tk >> 1) & 0x3F) * 60;	//add min. in tk to secs.
	tkSec += (tk & 0x01) == 0? 0: 30;	//add sec. interval to secs.
	//convert frame time (in GLONASS time) to an UTC instant (use GPS ephemeris for convenience)
	tTag = getSecsGPSEphe(1996 + (n4-1)*4, 1, nt, 0, 0, (float) tb) - 3*60*60;
	bom[0][0] = (int) tTag;													//Toc
	bom[0][1] = - getSigned(getBits(subfrmCh[ch][3].words, 58, 22), 22);	//Clock bias TauN: bits 80-59 string 4
	bom[0][2] = getSigned(getBits(subfrmCh[ch][2].words, 68, 11), 11);	//Relative frequency bias GammaN: bits 79-69 string 3
	bom[0][3] = ((int) getGPStow(tTag) + 518400) % 604800;		//seconds from UTC week start (mon 00:00). Note that GPS week starts sun 00:00. 
	bom[1][0] = getSigned(getBits(subfrmCh[ch][0].words, 8, 27), 27);	//Satellite position, X: bits 35-9 string 1
	bom[1][1] = getSigned(getBits(subfrmCh[ch][0].words, 40, 24), 24);	//Satellite velocity, X: bits 64-41 string 1
	bom[1][2] = getSigned(getBits(subfrmCh[ch][0].words, 35, 5), 5);	//Satellite acceleration, X: bits 40-36 string 1
	bom[1][3] = getBits(subfrmCh[ch][1].words, 77, 3);					//Satellite health Bn: bits 80-78 string 2
	bom[2][0] = getSigned(getBits(subfrmCh[ch][1].words, 8, 27), 27);	//Satellite position, Y: bits 35-9 string 2
	bom[2][1] = getSigned(getBits(subfrmCh[ch][1].words, 40, 24), 24);	//Satellite velocity, Y: bits 64-41 string 2
	bom[2][2] = getSigned(getBits(subfrmCh[ch][1].words, 35, 5), 5);	//Satellite acceleration, Y: bits 40-36 string 2
	bom[2][3] = carrierFreq[sv-1];										//Frequency number (-7 ... +13)
	bom[3][0] = getSigned(getBits(subfrmCh[ch][2].words, 8, 27), 27);	//Satellite position, Z: bits 35-9 string 3
	bom[3][1] = getSigned(getBits(subfrmCh[ch][2].words, 40, 24), 24);	//Satellite velocity, Z: bits 64-41 string 3
	bom[3][2] = getSigned(getBits(subfrmCh[ch][2].words, 35, 5), 5);	//Satellite acceleration, Z: bits 40-36 string 3
	bom[3][3] = getBits(subfrmCh[ch][1].words, 48, 5);			//Age of oper. information (days) (E): bits 53-49 string 2
	return true;
}

/**scaleGPSEphemeris apply scale factors to satellite ephemeris mantissas to obtain true satellite ephemirs stored in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa perameters as transmitted in the GPS navigation message, that shall be
 * converted to the true values applying the corresponding scale factor before being used.
 * Also a time tag is obtained from the week and time of week inside the satellite ephemeris.
 *
 * @param tTag the time tag of the satellite ephemeris
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each 
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each 
 */
void GNSSdataFromOSP::scaleGPSEphemeris(int (&bom)[8][4], double &tTag, double (&bo)[8][4]) {
	//time tag obtained from the calendar navigation data time (positive data in broadcastOrbit)
	tTag = getSecsGPSEphe (
				bom[5][2],	//GPS week#
				bom[0][0] * GPS_SCALEFACTOR[0][0] ); //T0C
	double aDouble;
	int iodc = bom[6][3];
	for(int i=0; i<8; i++) {
		for(int j=0; j<4; j++) {
			if (i==7 && j==1) {		//compute the Fit Interval from fit flag
				if (bom[7][1] == 0) aDouble = 4.0;
				else if (iodc>=240 && iodc<=247) aDouble = 8.0;
				else if (iodc>=248 && iodc<=255) aDouble = 14.0;
				else if (iodc==496) aDouble = 14.0;
				else if (iodc>=497 && iodc<=503) aDouble = 26.0;
				else if (iodc>=1021 && iodc<=1023) aDouble = 26.0;
				else aDouble = 6.0;
				bo[7][1] = aDouble;
			} else if (i==6 && j==0) {	//compute User Range Accuracy value
				bo[6][0] = bom[6][0] < 16? GPS_URA[bom[6][0]] : GPS_URA[15];
			} else if (i==2 && (j==1 || j==3)) {	//e and sqrt(A) are 32 bits unsigned
				bo[2][j] = ((unsigned int) bom[2][j]) * GPS_SCALEFACTOR[2][j];
			} else bo[i][j] = bom[i][j] * GPS_SCALEFACTOR[i][j]; //the rest signed
		}
	}
	return;
}

/**scaleGLOEphemeris apply scale factors to satellite ephemeris mantissas to obtain true satellite ephemirs stored in RINEX broadcast orbit like arrangements.
 * The given navigation data are the mantissa perameters as transmitted in the GLONASS navigation message, that shall be
 * converted to the true values applying the corresponding scale factor before being used.
 * Also a time tag is obtained from the 
 *
 * @param tTag the time tag of the satellite ephemeris
 * @param bom the mantissas of orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each 
 * @param bo the orbital parameters data arranged as per RINEX broadcast orbit into eight lines with four parameters each 
 */
void GNSSdataFromOSP::scaleGLOEphemeris(int (&bom)[8][4], double (&bo)[8][4]) {
	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			bo[i][j] = ((int) bom[i][j]) * GLO_SCALEFACTOR[i][j];
		}
	}
	return;
}

/**getGLOstring gets ten words from a MID8 OSP message payload and packs the 84 bits of a GLONASS string they contain into three words.
 *It is assumed that:
 *a-the OSP word 0, bits 23 to  0 contain GLONASS string bits 84 to 61	(24 b)
 *b-the OSP word 1, bits 24 to  0 contain GLONASS string bits 60 to 36	(25 b)
 *c-the OSP word 2, bits 24 to  0 contain GLONASS string bits 35 to 11	(25 b)
 *d-the OSP word 3, bits 24 to 15 contain GLONASS string bits 10 to  1	(10 b)
 *Data fields in the string have most significant bit (MSB) in left positions (as per C++ notation).
 *f-bits are moved to a compact string word array keeping bit order:
 *	- bit 1 is moved to bit 0 of compact string word 0, 2 to bit 1 of compact string word 0, and so on
 * 	- string bit 84 becomes bit 19 of compact string word 2
 *
 * @param stringW the three words array where the 84 bits of the GLONASS string are packed 
 * @return string number extracted from the bit stream passed, or 0 in case of error occurred
 */
int GNSSdataFromOSP::getGLOstring(unsigned int (&stringW)[3]) {
	unsigned int ospW[10];	//a place to store the ten words of OSP message payload
	try {
		for (int i=0; i<10; i++) ospW[i] = message.getUInt();
	} catch (int error) {
		plog->severe("MID8 " + msgEOM + to_string((long long) error));
		return 0;
	}
	stringW[0] = ((ospW[2] & 0x003FFFFF) << 10) | ((ospW[3] & 0x01FF8000) >> 15);
	stringW[1] = ((ospW[0] & 0x0000000F) << 28) | ((ospW[1] & 0x01FFFFFF) <<  3) | ((ospW[2] & 0x01C00000) >> 22);
	stringW[2] = ((ospW[0] & 0x00FFFFF0) >>  4);
	return getBits(stringW, 80, 4);
}

/**getGLOslot gets the GLONASS slot for a satellite number and channel given by the receiver
 *
 * @param ch the channel receiver where this satellite is being tracked
 * @param sat the satellite number that receiver has assigned to this satellite
 * @return the slot number in the GLONASS constellation for this satellite, or the own sat if this correpondence could not be stated
 */
int GNSSdataFromOSP::getGLOslot(int ch, int sat) {
	if ((sat >= FIRSTGLOSAT) && (sat <= LASTGLOSAT) && (satGLOslt[sat-FIRSTGLOSAT].slot > 0)) return satGLOslt[sat-FIRSTGLOSAT].slot;
	return sat;
}

