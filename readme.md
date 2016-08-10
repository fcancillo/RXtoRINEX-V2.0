##Introduction

RXtoRINEX project is aimed to provide tools:
 - To collect data from GPS / GNSS receivers embedded in mobile devices or connected to them through a serial port.
 - To analyze or print data collected
 - To convert data collected to RINEX or RTK formats
 - To read / filter / convert / write RINEX files

RINEX is the standard format used to feed with observation and navigation data files software packages for computing positioning solutions with high accuracy.

RINEX data files are not aimed to compute solutions in real time, but to perform post-processing, that is, GNSS/GPS receiver data are first collected into files using the receiver specific format, then converted to standard RINEX data files, and finally processed using, may be, facilities having �number-crunching� capabilities, and additional data (like data from reference stations available in UNAVCO, EUREF, GDC, IGN, and other data repositories) to allow removal of receiver data errors.

A detailed definition of the RINEX format can be found in the document "RINEX: The Receiver Independent Exchange Format Version 2.10" from Werner Gurtner; Astronomical Institute; University of Berne. An updated document exists also for Version 3.00.

RTK refers here to the data format defined for the RTKLIB (http://www.rtklib.com/) that would allow analysis of data collected with tools in this project using the RTKLIB.

Tools are provided at two levels:
 - C++ classes that could be used by developers to implement their own tools. The most relevant are: RinexData to process RINEX data files; RTKobservation to generate RTK files; and OSPdata to acquire navigation and observation data from SiRF (TM) One Socket Protocol (OSP) records.
 - Commands to acquire OSP data from SiRF receivers, generate RINEX files from OSP data, convert and filter RINEX files, and other.

Project documents can be found in the doc/html directory (https://cdn.rawgit.com/fcancillo/RXtoRINEX-V2.0/master/doc/html/index.html )

##Implementation status

A first version of the package was released in February 2015.

The current version released in April 2016 includes improvements to allow reading / filtering / converting / writing RINEX files, process GLONASS data, better isolation between OSP data acquisition and RINEX generation.

The current implementation has the following scope:
- Binaries provided are for win32 environment.
- GPS receivers shall be SiRF IV or V (TM) (for L1 signals) based, which should be able to provide OSP data.

Future versions to include support for other receivers would depend on availability of information from manufacturers. Any information on ICDs for other receivers (like SiRFIV-t, Broadcom, etc) is welcome.

As future releases of Android would include classes and methods to allow access from Apps to the GNSS receiver raw data (http://gpsworld.com/google-to-provide-raw-gnss-measurements/), it is expected to include in this package tools to generate RINEX files from such raw data.

Future releases would provide versions for Linux and Android.

##Data Files

The data tools provided are used to process or generate the following file types.


###RINEX observation

A RINEX observation file is a text file containing a header with data related to the file data acquisition, and sequences of epoch data. For each epoch the observables for each satellite tracked are included. See above reference on RINEX for a detailed description of this file format.


###RINEX navigation

A RINEX navigation file is a text file containing a header with data related to the file data acquisition, and satellite ephemeris obtained from the navigation message transmitted inside the navigation satellite signal. See above reference on RINEX for a detailed description of this file format.


###OSP binary

OSP binary are files containing OSP message data obtained from SiRF(TM) based GPS receivers. They can be embedded in the smartphone or connected to the computer or device through a serial port.

The file contains a sequence of OSP messages, where for each message they are stored the two bytes with the payload length and the n bytes of the message payload. The OSP binary file is simply the byte sequence of the transport message packet generated by the receiver after removing the bytes for "Start Sequence" (A0 A2), "Checksum" (2 bytes), and "End Sequence" (B0 B3).

A detailed definition of OSP messages can be found in the document "SiRFstarIV(TM) One Socket Protocol Interface Control Document".


###GP2 debug

GP2 debug are files containing receiver specific measurement data obtained in Android smartphones with embedded SiRFIV-T(TM) GPS receivers when DEBUGGING_FILES is set to 1 in the sirfgps.conf file.

They are text files containing in each line a time tag followed by an OSP message with byte contents written in hexadecimal.

Each line in the GP2 file has format as per the following example:

29/10/2014 20:31:08.942 (0) A0 A2 00 12 33 06 00 00 00 00 00 00 00 19 00 00 00 00 00 00 64 E1 01 97 B0 B3

Where:
       - Time tag: 29/10/2014 20:31:08.942
       - Unknown: (0) 
       - Head: A0 A2
       - Payload length: 00 12
       - Payload: 33 06 00 00 00 00 00 00 00 19 00 00 00 00 00 00 64 E1
       - Checksum: 01 97
       - Tail: B0 B3

Note that in above format, data from "head" to "tail" is an OSP messages with values written in hexadecimal.


###PKT messages

Packet message files contain a sequence of binary OSP full messages. Each full message starts with the head (two bytes with values 0xA0 and 0xA2), following the payload length (a two byte integer), the payload bytes, a two bytes checksum, and finishing with the tail (two bytes with values 0xB0 and 0xB3). Between the tail of one message and the head of the following one it may be garbage data.

Such files may be captured directly through the serial port where the receiver is connected using for example a terminal emulator.


##C++ Classes

###RinexData

The RinexData class defines a data container for the RINEX file header records, epoch observables, and satellite navigation ephemeris.

The class provides methods:
 - To store RINEX header data and set parameters in the container
 - To store epoch satellite ephemeris
 - To store epoch observables
 - To print RINEX file header
 - To print RINEX ephemeris data
 - To print RINEX epoch observables

Also it includes methods to read existing RINEX files, to store their data in the container, and to access and print them.

Methods are provided to set data filtering criteria (system, satellite, observables), and to filter data before printing.


###RTKobservation

The RTKobservation class defines data to be used for storing and further printing of a RTK file header and the position solution data of each epoch.


###GNSSdataFromOSP

The GNSSdataFromOSP class defines data and methods used to acquire RINEX or RTK header and epoch data from a binary OSP file containing SiRF receiver messages.

Such header and epoch data can be used to generate and print RINEX or RTK files.


###OSPMessage

The OSPMessage class provides resources to perform data acquisition from OSP message payloads. Note that the payload is a part of the OSP message described in the SiRF ICD.

The class allows a cursor based, buffered acquisition process from the OSP file and include methods: to fill the buffer with messages read from a OSP binary file, to get values for the message data types taking into account sign and bit and byte ordering in the source, and skip unused data from the buffer.


###ArgParser

The ArgParser class defines a data container for options and operators passed to a program as arguments in the command line, and it provides methods to manage them.


###Logger

The Logger class allows recording of tagged messages in a logging file. The class defines a hierarchy of log levels (SEVERE, WARNING, INFO, CONFIG, FINE, FINER or FINEST) and provides methods to set the current log level and log messages at each level.

Only those messages having level from SEVERE to the current level stated are actually recorded in the log file.


##Data Tools

###RXtoOSP command

This command line program can be used to capture OSP message data from a SiRF receiver connected to the computer / device serial port and to store them in an OSP binary file.

Data acquisition can be controlled using options to:
 - Show usage data
 - Set log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - State the serial port name where receiver is connected
 - Set the serial port baud rate
 - State the name of the OSP binary output file
 - Set duration of the acquisition period
 - Configure generation of OSP messages with satellite ephemeris data (MID8, MID15, MID7)
 - Set the observation interval (in seconds) for epoch data
 - Stop epoch data acquisition when a message with given MID arrives

Note: use of this command requires a GPS receiver state compatible with data being requested: transmitting serial data at the bit rate, length and parity expected, and in OSP format. See SynchroRX command below for details.


###GP2toOSP command

This command line program is used to translate data captured in GP2 debug files to OSP message files.

Data translation can be controlled using options to:
 - Show usage data and stops
 - Set log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - State the GP2 input file name
 - State the time interval for extracting lines in the GP2 file
 - Set the OSP binary output file name
 - State the list of �wanted� messages MIDs. The rest of messages will be ignored


###OSPtoTXT

This command line program is used to print contents of OSP files into readable format. Printed data are sent to the stdout.

The output contains descriptive relevant data from each OSP message:
 - Message identification (MID, in decimal) and payload length for all messages
 - Payload parameter values for relevant messages used to generate RINEX or RTK files
 - Payload bytes in hexadecimal, for MID 255


###OSPtoRINEX

This command line program is used to generate RINEX files from an OSP data file containing SiRF receiver message data.

The generation of RINEX files can be controlled using options to:
 - Show usage data and stops
 - Set log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - Set RINEX file name prefix
 - Set RINEX version to generate (V211, V302)
 - State the specific data to be included in the RINEX file header, like receiver marker name, observer name, agency name, who run the RINEX file generator, receiver antenna type, antenna number.
 - Set code measurements to include (like C1C,L1C, etc.)
 - Set minimum satellites needed in a fix to include its observations
 - Set if end-of-file comment lines will be appended or not to RINEX observation file
 - Generate or not RINEX navigation files, and which data has to be used to generate it: MID8 messages with 50bps data, or MID15/MID70 with receiver collected ephemeris
 - State the selected systems to print in addition to GPS (GLONASS and or SBAS)


###OSPtoRTK

This command line program is used to generate a RTK file with positioning data extracted from an OSP data file containing SiRF IV receiver messages.

The generation of RTK files can be controlled using options to:
 - Show usage data and stops
 - Set the log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - Set the minimum number of satellites in a fix to include its positioning data


###SynchroRX

This command line program can be used to synchronize SiRF based receiver and computer to allow communication between both.

The receiver state is defined by the following:
 - The serial port bit rate, stop bits, and parity being used by receiver
 - The protocol mode being used by receiver to send data: NMEA or OSP
 - If data is flowing from the receiver or not.

The computer data acquisition process state is defined by:
 - The port (port name) used for the communication with the GPS receiver
 - The serial port bit rate, stop bits, and parity being used
 - The type of data required: NMEA or OSP

To allow communication between GPS receiver and computer, states of both elements shall be synchronized according to the needs stated.

Synchronization requires first to know the current state of receiver and computer port, and then change both to the new state. But, to know the receiver state is necessary to communicate with it, setting the computer port at the same speed, stop bits and parity that the receiver. This will allow receiving and analyzing the data being generated by the receiver to know the protocol it is using. As receiver state may be not known initially, it would be necessary to scan different communication alternatives to know it.

To allow synchronization it is assumed the following for the receiver:
 - It is providing data continuously: ASCII NMEA or binary OSP messages, depending on the mode.
 - It is receiving/sending data at 1200, 2400, 4800, 9600, 38400, 57600 or 115200 bps, with one stop bit and parity set to none.

After synchronization, bit rates will be set to 9600 bps when exchanging NMEA data or to 57600 bps when exchanging OSP messages.

The synchronization process can be controlled using options to:
 - Show usage data and stops
 - Set the log level (SEVERE, WARNING, INFO, CONFIG, FINE, FINER, FINEST)
 - Set the serial port name where receiver is connected
 - Set the receiver protocol to NMEA or OSP


###PacketToOSP

This command line program is used to extract from an input binary file containing SiRF receiver message packets their payload data, and store them into an OSP binary file. Such input files can be obtained from the receiver data stream using system tools, or application specific ones.

The command extracts and verifies message packets, and writes the payload data of the correct ones to the OSP binary file.


###RINEXtoRINEX

This command line program is used to generate a RINEX file from data contained in another RINEX file.

The command allows user to change version of the output file (with regard to input file), and filtering its data.

To filter input observation data the user can define:
 - The time of the first and last epoch to be included.
 - The list of system / observables to be included
 - The list of system / satellites to be included
 - The version of the output RINEX files

To filter input navigation data the user can define similar criteria, except system / observable.


###RINEXtoCSV

This command line program is used to generate a CSV or TXT file from data contained in a given observation or navigation RINEX file.

Input data are contained in a RINEX observation or navigation file containing header and epoch data.

The outputs are CSV (Comma Separated Values) text data files: one containing header data, and the other containing epoch data (observation or navigation, depending on the RINEX file type). This type of files can be used to import data to some available applications (like MS Excel).

The command allows user to filter input file data to be converted. To do that, user can define:
 - The time of the first and last epoch to be included.
 - The list of system / observables to be included
 - The list of system / satellites to be included


##Test files

The directory ./Data contains sample files obtained with the above described tools.

###GStarIV

Contains GPS data acquired with an external GlobalSat G-Star IV model BU-353S4 (SiRFIV-e receiver). Each subdirectory inside includes data files from a survey:
 - The OSP file acquired with RXtoOSP in a given point.
 - A readable TXT file generated with OSPtoTXT
 - The RINEX files generated with OSPtoRINEX
 - a RTK file generated with OSPtoRTK
 - LogFile.txt from executions of above commands

In addition there are files with analysis data from teqc and from PRSolve (part of the GPS ToolKit).

###SamsungGlxyS2

Contains GPS data acquired with a Samsung Galaxy S2 (GT-I9100) from its  embedded receiver (a SiRFIV-t). Each subdirectory inside includes data for different surveys, including:
 - The gp2 file downloaded from the smartphone
 - An OSP file generated with GP2toOSP
 - Readable TXT file generated with OSPtoTXT
 - RINEX files generated with OSPtoRINEX
 - RTK files generated with OSPtoRTK
 - LogFile.txt from executions of above commands

###SiRFV based receiver

As per GStarIV, but using data captured by a SiRFV based receiver.

