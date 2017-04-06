#ifndef __IP_UTILITY_H
#define __IP_UTILITY_H

#include <string>

class IPUtility
{
public:

	IPUtility ();
	virtual ~IPUtility ();

	static bool isIPv4 (std::string iAddress);
};

#endif