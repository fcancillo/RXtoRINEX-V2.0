/** @file ArgParser.cpp
 * Contains the implementation of the ArgParser class.
 */

#include "ArgParser.h"
#include <iostream>

/**Constructs an empty Option object
  */
Option::Option(void) {
}

/**Contructs an Option object with a given name, value (if applicable) and type.
 *
 *@param id the identification of the Option object
 *@param name the option name having the format: -x (when short) or --x{x} (when long)
 *@param value the option value if string, or empty if boolean
 *@param isLong true when the option name has long format ( --x{x} ), false when is short ( -x )
 *@param isString true when the option is a string, false when is boolean
 */
Option::Option(int id, string name, string value, bool isLong, bool isString) {
	identification = id;
	if (isLong) {	//It is an long format option
		sName.clear();
		lName = name;
	} else {		//It is a short format option
		lName.clear();
		sName = name;
	}
	isStr = isString;
	strSet = value;
	boolSet = true;
}

/**Constructs an Option string type object.
 *
 *@param id the identification of the Option object
 *@param s short name for the optin (-x)
 *@param l long name for the optin (--xxxxx)
 *@param d a word describing the option value
 *@param u a explanation for usage of this option
 *@param v default value for the option
 */
Option::Option(int id, char * s, char * l, char * d, char * u, char * v) {
	identification = id;
	sName = s;
	lName = l;
	description = d;
	usage = u;
	isStr = true;
	defStr = v;
	strSet = v;
}

/**Constructs an Option boolean type object.
 *
 *@param id the identification of the Option object
 *@param s short name for the option (-x)
 *@param l long name for the option (--xxxxx)
 *@param d a word describing the option value
 *@param u a explanation for usage of this option
 *@param v default value for the option
 */
Option::Option (int id, char * s, char * l, char * d, char * u, bool v) {
	identification = id;
	sName = s;
	lName = l;
	description = d;
	usage = u;
	isStr = false;
	defBool = v;
	boolSet = v;
}

/**Constructs an empty ArgParser object.
 */
ArgParser::ArgParser(void) {
	lastId = -1;
}

/**Destructs ArgParser objects.
 */
ArgParser::~ArgParser(void) {
	optionsLst.clear();
}

/**addOption adds a string type option to the ArgParser object.
 *
 *@param s short name for the option (-x)
 *@param l long name for the option (--xxxxx)
 *@param d a word describing the option value
 *@param u an explanation on usage of this option
 *@param sv default string value for the option
 *@return the identification assigned to the option
 */
int ArgParser::addOption (char * s, char * l, char * d, char * u, char * sv) {
	optionsLst.emplace_front(Option(++lastId, s, l, d, u, sv));
	return lastId;
}

/**addOption adds a boolean type option to the ArgParser object.
 *
 *@param s short name for the option (-x)
 *@param l long name for the option (--xxxxx)
 *@param d a word describing the option value
 *@param u an explanation on usage of this option. Note that if default value is positive,
 *		explanation has to be stated in negative terms, like "Don't ..."
 *@param bv default boolean value for the option
 *@return the identification assigned to the option
 */
int ArgParser::addOption (char * s, char * l, char * d, char * u, bool bv) {
	optionsLst.emplace_front(Option(++lastId, s, l, d, u, bv));
	return lastId;
}

/**addOperator adds an operator with the give default string value to the ArgParser object.
 *
 *@param v default value of the operator
 *@return the index assigned to the operator (0 to the 1st, 1 to the 2nd, ...)
 */
int ArgParser::addOperator (char * v) {
	operatorsVector.push_back(v);
	return operatorsVector.size() - 1;
}

/**getBoolOpt gets the current value of the boolean type option with the given identification.
 *
 *@param id identification of the option
 *@return the current value of the option requested
 *@throws error string with message, when it cannot find the option
 */
bool ArgParser::getBoolOpt (int id) {
	forward_list<Option>::iterator iterator;
	for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
		if (iterator->identification == id) return iterator->boolSet;
	}
	string error(MSG_UnkId + to_string((long long) id));
	throw error;
}

/**getStrOpt gets the current value of the string type option with the given identification.
 *
 *@param id identification of the option
 *@return the current value of the option requested
 *@throws error string with message, when it cannot find the option
 */
string ArgParser::getStrOpt (int id) {
	forward_list<Option>::iterator iterator;
	for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
		if (iterator->identification == id) return iterator->strSet;
	}
	string error(MSG_UnkId + to_string((long long) id));
	throw error;
}

/**getOperator gets the value of the string operator located in the given index.
 *
 *@param index of the operator requested in the operators vector (0 to the 1st, 1 to the 2nd, ...)
 *@return the current string value of the operator requested
 *@throws error string with a message, when the operator does not exist
 */
string ArgParser::getOperator (int index) {
	if ((unsigned int) index < operatorsVector.size()) return operatorsVector[index];
	string error(MSG_UnkId + to_string((long long) index));
	throw error;
}

/**usage provides information through cerr on usage using data from the options definition.
 *
 *@param message argument error to be displayed 
 *@param howUse text describing how to use the command line 
 */
void  ArgParser::usage(string message, string howUse) {
	cerr << message << "\n";
	cerr << "Usage:\n" << howUse << "\nOptions are:\n";
	forward_list<Option>::iterator iterator;
	for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
		if (iterator->isStr)
			cerr << iterator->sName << " " << iterator->description << " or " <<
				iterator->lName << "=" << iterator->description <<
				" : " << iterator->usage <<
				". Default value " << iterator->description << " = " << iterator->defStr << "\n";
		else
			cerr << iterator->sName << " or " << iterator->lName <<
				" : " << iterator->usage <<
				". Default value " << iterator->description << (iterator->defBool? "=TRUE" : "=FALSE") << "\n";
	}
	if (operatorsVector.size() > 0) {
		cerr << "Default values for operators are: ";
		for (unsigned int i=0; i<operatorsVector.size(); i++)
			cerr << operatorsVector[i] << " ";
		cerr << "\n";
	}
}

/**showOptValues provides a text description of options and their current values.
 *
 *@return a text description with name and value of options
 */
string ArgParser::showOptValues() {
	string values = "Options:";
	forward_list<Option>::iterator iterator;
	for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
		values += "\n\t(" + iterator->sName + ")" + iterator->description + "=";
		if (iterator->isStr) values += iterator->strSet;
		else values += (iterator->boolSet? "TRUE" : "FALSE");
	}
	return values;
}

/**showOpeValues provides the values of operators.
 *
 *@return a text description with the current values of operators
 */
string ArgParser::showOpeValues() {
	string values = "Operators:";
	for (unsigned int i=0; i<operatorsVector.size(); i++) {
		values += "\n\t" + operatorsVector[i];
	}
	return values;
}

/**setOptVal update in the ArgParser container the given option value.
 *When it is a string type option, the given value will replace the pre-assigned default.
 *When it is a boolean type option, the new value will be the oposite of the pre-assigned default one.
 *
 *@param opt the option data (name, type and value) to be update 
 */
void ArgParser::setOptVal (Option opt) {
	forward_list<Option>::iterator iterator;
	//find the given option name (short or long) in the list
	if (!opt.sName.empty())		//search a short name
		for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
			if (opt.sName.compare(iterator->sName) == 0) break; //short name found
		}
	else		//it has a long name, search it
		for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
			if (opt.lName.compare(iterator->lName) == 0) break; //long name found
		}
	if (iterator != optionsLst.end())	//update option value in the found position
		if (iterator->isStr) iterator->strSet = opt.strSet;
		else iterator->boolSet = !iterator->defBool;
}

/**isShortOption checks if a given short name ( -x ) is in the list of options.
 *If requested, it checks also if the given name correspond to a string type option.
 *
 *@param name the option short name to search in the options list
 *@param checkStringType when true, it checks also if the given name is a string type option 
 *
 *@return false when the given name is not in the list,
 *	true when the given name is in the list and checkStringType is false,		
 *	the value of isStr when the given name is in the list and checkStringType is true. 
 */
bool ArgParser::isShortOption (string name, bool checkStringType) {
	forward_list<Option>::iterator iterator;
	for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
		if (name.compare(iterator->sName) == 0) {	//name found
			if (checkStringType) return iterator->isStr;
			else return true;
		}
	}
	return false;
}

/**isLongOption checks if a given long name ( --x{x} ) is in the list of options.
 *If requested, it checks also if the given name correspond to a string type option.
 *
 *@param name the option long name to search in the options list
 *@param checkStringType when true, it checks if the given name is a string type option 
 *@return false when the given name is not in the list,
 *	true when the given name is in the list and checkStringType is false,		
 *	the value of isStr when the given name is in the list and checkStringType is true. 
 */
bool ArgParser::isLongOption (string name, bool checkStringType) {
	forward_list<Option>::iterator iterator;
	for (iterator = optionsLst.begin(); iterator != optionsLst.end(); iterator++) {
		if (name.compare(iterator->lName) == 0) {	//name found
			if (checkStringType) return iterator->isStr;
			else return true;
		}
	}
	return false;
}

/**parseArgs parses the argument list extracting Options and Operands to the ArgParser container.
 * Parsing starts from argv[1], and stops when the last argument is parsed or an error is detected.
 * Correct options and / or operands parsed from correct arguments are stored in the ArgParser object.
 * When error is detected the parsing stops, and an exception is raised. The caller shall catch it. 
 * 
 *@param argc number of arguments passed (as per main)
 *@param argv the command line argument as provided to the program
 *@throws error string with a message describing the argument error
 */
void  ArgParser::parseArgs (int argc, char** argv) {
	string name;
	string value;
	Option opt;
	int n;
	int i = 1;
	unsigned int opeIndex = 0;
	while (i<argc) {
		//get name from current argv and assume that its value is in the next one
		name = string(argv[i++]);
		if (i < argc) value = string(argv[i]);
		else value.clear();
		if (name.at(0) == '-') {	//It would be an Option: verify it
			if(name.size() < 2) throw name + MSG_UnknOption;	//nothing follows -
			if (name.at(1) == '-') {	//It would be a long name Option: verify it
				if(name.size() < 3) throw name + MSG_UnknOption;//nothing follows --
				n = name.find('=', 2);		//find value separator
				if (n == name.npos) {		// = not found. It would be a boolean long name option
					if (!isLongOption(name, false)) throw name + MSG_UnknOption;//name not in the list
					if (isLongOption(name, true)) throw name + MSG_ValueNotSet;	//name found, but not boolean
					//set data for a boolean long name option
					value.clear();
					opt = Option(-1, name, value, true, false);
				}
				else {		// = found. It would be a string long name option
					value = name.substr(n+1);
					name = name.substr(0, n);
					if (!isLongOption(name, false)) throw name + MSG_UnknOption;//name not in the list
					if (!isLongOption(name, true)) throw name + MSG_ValueSet;	//name found, but not string type
					//set data for a string long name option
					opt = Option(-1, name, value, true, true);
				}
			}
			else {		//It would be a short name Option: verify it
				if (!isShortOption(name, false)) throw name + MSG_UnknOption;	//name not in the list
				if (isShortOption(name, true)) {	//this arg is a string short option
					//value must be follow (not empty, cannot start with - to be a true value)
					if (value.empty() || value.at(0)=='-') throw name + MSG_ValueNotSet;
					//set data for a string short name option
					opt = Option(-1, name, value, false, true);
					i++;	//compute index of next argv to examine
				}
				else {
					//set data for a boolen short name option
					value.clear();
					opt = Option(-1, name, value, false, false);
				}
			}
			//an option has been parsed, store it
			setOptVal(opt);
		} else {		//It is an Operand. Put it in its place of the operators vector
			if (opeIndex < operatorsVector.size()) {
				operatorsVector[opeIndex] = name;
				opeIndex++;
			}
			else throw name + MSG_TooOpe;
		}
	}
}
