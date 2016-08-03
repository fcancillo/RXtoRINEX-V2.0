/** @file Logger.cpp
 * contains the implementation of the Logger class
 */

#include "Logger.h"
#include "Utilities.h"

/**Constructs a Logger using default parameters.
 *<p>It set stderr as log file, and sets the default log level to INFO. 
 *
 */
Logger::Logger(void) {
	levelSet = INFO;
	fileLog = stderr;
}

/**Constructs a Logger using the given file name for recording messages.
 *<p>It opens a file with the given fileName, and sets the default log level to INFO.
 *If the file exists, further messages will be appended at the end of the existing file. If it does not exist, it is created.
 *
 *@param fileName the name of the log file
 */
Logger::Logger(string fileName) {
	levelSet = INFO;
	fileLog = fopen(fileName.c_str(), "a");
	if (fileLog == NULL) fileLog = stderr;
}

/**Constructs a Logger using the given file name for recording messages, sets the logging message prefix, and prints the given initial message.
 *<p>It opens a file with the given fileName, and sets the default log level to INFO.
 *If the file exists, messages are appended at the end of the existing file. If it does not exist, it is created.
 *
 *@param fileName the name of the log file
 *@param prefix is a text to prefix logging messages (for exemple the name of the program, or an empty string)
 *@param initMsg a text message to be logged when the logging object is created
 */
Logger::Logger(string fileName, string prefix, string initMsg) {
	levelSet = INFO;
	fileLog = fopen(fileName.c_str(), "a");
	if (fileLog == NULL) fileLog = stderr;
	program = prefix;
	logMsg(SEVERE, initMsg);
}

/**Destructs the Logger object after closing its log file. 
 */
Logger::~Logger(void) {
	logMsg (SEVERE, "logging END");
	if (fileLog != stderr) fclose(fileLog);
}

/**setPrgName sets the prefix name to be used in message tagging
 *
 *@param prefix the text to prefix messages (usually the program name)
 */
void Logger::setPrgName(string prefix) {
	program = prefix;
}

/**setLevel states the log level to be taken into account when logging messages.
 *<p>Log level can be SEVERE, WARNING, INFO, CONFIG, FINE, FINER or FINEST.
 *Only meesages having log level from SEVERE to setLevel will be actually recorded.
 *
 *@param level the log level to set
 */
void Logger::setLevel(logLevel level) {
	levelSet = level;
}

/**setLevel states the log level to be taken into account when logging messages.
 *<p>Level is given as a string containing the word "S[EVERE]", "W[ARNING]", "I[NFO]", "C[ONFIG]", "[FIN]E", "[FINE]R" or "[FINES]T",
 *which correspond with the log lavel having the same name. Note: characters between braces are optional.
 *<p>Only meesages having log level from SEVERE to the level set will be actually recorded.
 *<p>If level do not match with any of above stated words, the default INFO log level is set.
 *<p>Level characters can be in upper o lower case.
 *
 *@param levelDescription the word describing the log level to set
 */
void Logger::setLevel(string levelDescription) {
	levelSet = identifyLevel(levelDescription);
}

/**isLevel gives result of comparing the current log level with the level given.
 *<p>Level can be SEVERE, WARNING, INFO, CONFIG, FINE, FINER or FINEST.
 *The result of the comparison is true the given level is between SEVERE and the current level, that is
 *messages at the given level would be actually recorded.
 *<p>This methos is a way to know in advance if messages at the given level would be logged or not.
 *
 *@param level the log level to compare
 *@return true when messages at the given level would be logged, false otherwise.
 */
bool Logger::isLevel(logLevel level) {
	if (levelSet <= level) return true;
	return false;
}

/**isLevel gives result of comparing the current log level with the level given.
 *<p>Level is given as a string containing the word "S[EVERE]", "W[ARNING]", "I[NFO]", "C[ONFIG]", "[FIN]E", "[FINE]R" or "[FINES]T",
 *which correspond with the log lavel having the same name. Note: characters between braces are optional.
 *<p>Level characters can be in upper o lower case.
 *<p>If the given level do not match with any of above stated words, it is assumed the default INFO log level.
 *<p>The result of the comparison is true the given level is between SEVERE and the current level, that is
 *messages at the given level would be actually recorded.
 *<p>This methos is a way to know in advance if messages at the given level would be logged or not.
 *
 *@param levelDescription the word describing the log level to set
 */
bool Logger::isLevel(string levelDescription) {
	if (levelSet <= identifyLevel(levelDescription)) return true;
	return false;
}

/**severe logs a message at SEVERE level.
 * All SEVERE messages are appended to the log file.
 *
 *@param toLog the message text to log
 */
void Logger::severe(string toLog) {
	logMsg(SEVERE, toLog);
}

/**warning logs a message at WARNING level.
 * The message is appended to the log file if the current level is in the range SEVERE to WARNING
 *
 *@param toLog the message text to log
 */
void Logger::warning(string toLog) {
	if (levelSet < WARNING) return;
	logMsg(WARNING, toLog);
}

/** info logs message at INFO level.
 * The message is appended to the log file if the current level is in the range SEVERE to INFO
 *
 *@param toLog the message text to log
 */
void Logger::info (string toLog) {
	if (levelSet < INFO) return;
	logMsg(INFO, toLog);
}

/**config logs the message at CONFIG level.
 * The message is appended to the log file if the current level is in the range SEVERE to CONFIG
 *
 *@param toLog the message text to log
 */
void Logger::config(string toLog) {
	if (levelSet < CONFIG) return;
	logMsg(CONFIG, toLog);
}

/**fine logs the message at FINE level.
 * The message is appended to the log file if the current level is in the range SEVERE to FINE
 *
 *@param toLog the message text to log
 */
void Logger::fine(string toLog) {
	if (levelSet < FINE) return;
	logMsg(FINE, toLog);
}

/**finer logs the message at FINER level.
 * The message is appended to the log file if the current level is in the range SEVERE to FINER
 *
 *@param toLog the message text to log
 */
void Logger::finer(string toLog) {
	if (levelSet < FINER) return;
	logMsg(FINER, toLog);
}

/**finest log the message at FINEST level.
 * The message is appended to the log file if the current level is in the range SEVERE to FINEST
 *
 *@param toLog the message text to log
 */
void Logger::finest(string toLog) {
	if (levelSet < FINEST) return;
	logMsg(FINEST, toLog);
}

//*Private methods

/**logMsg is an internal method to tag, format, and log messages data passed by log level methods.
 *
 *@param logLevel states the level to tag the message
 *@param message contains its description
 */
void Logger::logMsg(logLevel msgLevel, string msg) {
	time_t rawtime;
	struct tm * timeinfo;
	char txtBuf[80];

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	if (msgLevel == SEVERE) strftime(txtBuf, sizeof txtBuf, " %Y-%m-%d %H:%M:%S ", timeinfo);
	else strftime(txtBuf, sizeof txtBuf, " %H:%M:%S ", timeinfo);
	fprintf(fileLog, "%s%s", program.c_str(), txtBuf);
	switch (msgLevel) {
	case SEVERE: fprintf(fileLog, "(SVR) "); break;
	case WARNING: fprintf(fileLog, "(WRN) "); break;
	case INFO: fprintf(fileLog, "(INF) "); break;
	case CONFIG: fprintf(fileLog, "(CFG) "); break;
	case FINE:  fprintf(fileLog, "(FNE) "); break;
	case FINER:  fprintf(fileLog, "(FNR) "); break;
	case FINEST: fprintf(fileLog, "(FNS) "); break;
	}
	fprintf(fileLog, "%s\n", msg.c_str());
	fflush(fileLog);
}

/**identifyLevel gives the log level corresponding to level description given.
 *<p>Level description is given as a string containing the word "S[EVERE]", "W[ARNING]", "I[NFO]", "C[ONFIG]", "[FIN]E", "[FINE]R" or "[FINES]T",
 *which correspond with the log lavel having the same name. Note: characters between braces are optional.
 *<p>If level do not match with any of above stated words, the default INFO log level is returned.
 *<p>Level characters can be in upper o lower case.
 *
 *@param levelDescription the word describing the log level to set
 *@return the log level identifier corresponding to the level description in param level
 */
Logger::logLevel Logger::identifyLevel(string levelDescription) {
	logLevel levelId;
	string lvlUpp = strToUpper(levelDescription);
	if (lvlUpp.front() == 'S') levelId = SEVERE;
	else if (lvlUpp.front() == 'W') levelId = WARNING;
	else if (lvlUpp.front() == 'I') levelId = INFO;
	else if (lvlUpp.front() == 'C') levelId = CONFIG;
	else if (lvlUpp.back() == 'E') levelId = FINE;
	else if (lvlUpp.back() == 'R') levelId = FINER;
	else if (lvlUpp.back() == 'T') levelId = FINEST;
	else levelId = INFO;
	return levelId;
}
