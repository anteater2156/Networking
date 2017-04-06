#ifndef __STRING_UTILITY_H
#define __STRING_UTILITY_H

#include <string>
#include <vector>

class StringUtility
{
public:

	static std::vector<std::string> Parse (std::string iString, const char& iDelimiter = ' ');

	static int Decimal (std::string iString);

};

#endif