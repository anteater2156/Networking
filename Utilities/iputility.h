#ifndef __IP_UTILITY
#define __IP_UTILITY

#include <string>

class IPUtility
{
public:

	IPUtility ();
	virtual ~IPUtility ();

	static bool isIPv4 (std::string iAddress);
};

#endif