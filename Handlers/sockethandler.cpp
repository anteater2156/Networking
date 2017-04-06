#include "sockethandler.h"
#include "debug.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <sstream>

#include <cstring>

namespace Ports
{

	std::string cBACKEND_OR = "21278";
	std::string cBACKEND_AND = "22278";
	std::string cEDGE_SERVER_UDP = "23278";
	std::string cEDGE_SERVER_TCP = "24278";

	std::string cHOSTNAME = "localhost";

	// static port address

	std::string backendOr() { return cBACKEND_OR; }
	std::string backendAnd() { return cBACKEND_AND; }
	std::string edgeserverTCP() { return cEDGE_SERVER_TCP; } 
	std::string edgeserverUDP() { return cEDGE_SERVER_UDP; }


	std::string hostname () { return cHOSTNAME; }
}


SocketHandler::SocketHandler () :
	
	mSocketDescr(-1),
	mOpened(false)

{}

SocketHandler::SocketHandler (int iDescription,
			   				  struct addrinfo iInfo,
			   				  bool iOpened) :

	mSocketDescr(iDescription),
	mOpened(iOpened),
	mSocketInfo(iInfo)

{}

SocketHandler::~SocketHandler () 
{ 
	kill();
}

// Accessors

bool SocketHandler::hasData (int& oAmount) const
{
	// Grabs the number of immediately readable bytes for the socket

	int tBytesReady = 0;

	if (ioctl (socketDescriptor(), FIONREAD, &tBytesReady) >= 0)
	{
		oAmount = tBytesReady;
		return true;
	}	

	// error reading, return false

	oAmount = -1;
	return false;
}

// Mutators

int SocketHandler::createSocket (const char* iHostName,
				  				  const char* iPortNum)
{
	// Destroy old socket before creating new

	kill();	

	struct addrinfo* tServiceResults; 
	struct addrinfo tHints = hints();

    // Get address info, return false if error

	if (getaddrinfo(iHostName, iPortNum, &tHints, &tServiceResults) != 0)
		return -1;

	// Grab first result and use address info results to create a socket

	mSocketInfo = (*tServiceResults);
	mSocketDescr = socket(mSocketInfo.ai_family, mSocketInfo.ai_socktype, mSocketInfo.ai_protocol);

	// ** Code sets the socket to be reusable. In other words, 
	// it should allow the socket to be bound (binded?) again
	// after the application closes

	int tOption = 1;
	setsockopt(mSocketDescr, SOL_SOCKET, SO_REUSEADDR, &tOption, sizeof(int));

	freeaddrinfo(tServiceResults);

	return mSocketDescr;
}

void SocketHandler::kill ()
{
	close(mSocketDescr);
	open(false);
	
	mSocketDescr = -1;
}

// Protected methods


std::string SocketHandler::encapsulate (std::string iMessage)
{
	iMessage.append("<");
	return iMessage;
}

std::vector<std::string> SocketHandler::unencapsulate (std::string iData)
{
	std::vector<std::string> tMessages;

	if (iData.size() == 0)
		return tMessages;

	std::stringstream tStream;
	tStream.str(iData);

	// Parse token

	std::string tToken;
	while(std::getline(tStream, tToken, '<'))
		tMessages.push_back(tToken);

	// Remove last token. Because '<' is a terminator
	// the last token is whatever is leftover after the 
	// the last message. Normally its an empty string

	tMessages.pop_back();

	return tMessages;
}

void SocketHandler::open (bool iEnabled) { mOpened = iEnabled; }


UDPHandler::UDPHandler () :

	SocketHandler()

{
	open(false);
}


UDPHandler::UDPHandler (int iDescription,
						struct addrinfo iInfo,
						bool iOpened) :

	SocketHandler(iDescription, iInfo, iOpened)

{}

UDPHandler::~UDPHandler ()
{
	kill();
}

bool UDPHandler::setRemoteAddress (const char* iHostName,
					   			   const char* iPortNum)
{
	struct addrinfo* tServiceResults; 
	struct addrinfo tHints = hints();

    // Get address info, return false if error

	if (getaddrinfo(iHostName, iPortNum, &tHints, &tServiceResults) != 0)
		return false;

	// Grab first result and use address info results to create a socket

	mRemote = (*((sockaddr_storage*) (*tServiceResults).ai_addr));

	freeaddrinfo(tServiceResults);

	return true;
}

bool UDPHandler::transmit (std::string iMessage)
{
	// Prepare message to be transmitted

	std::string tMessage = encapsulate(iMessage);

	// unix socket method send expects (void*)
	// Turn iMessage into char*

	int tDataSize = tMessage.length()+1;
	char* tData = new char[tDataSize];
	std::strcpy(tData, tMessage.c_str());

	// Continue sending data until whole message is
	// sent or error occurs

	char* tSending = tData;
	while (tDataSize > 0)
	{
		int tNumBytesWritten = sendto(socketDescriptor(), tSending, tDataSize, 0,
									  (struct sockaddr*) &mRemote, sizeof(mRemote));
		
		if (tNumBytesWritten < 1) 
		{
			// Error or connection was closed

			kill();
			return false;
		}

		tSending += tNumBytesWritten;
		tDataSize -= tNumBytesWritten;	
	}

	delete[] tData;

	return true;
}

std::vector<std::string>  UDPHandler::receive () 
{
	std::string tMessage;

    int tByteReady = 0;

    while ( (tByteReady == 0) && (hasData(tByteReady) == true) )
		usleep(100);

	do
	{
		// Continue reading socket until
		// all data is read

	    if (tByteReady == -1)
		{
			// An error occurred attempting to 
			// read amount of data on socket.

			// Return empty list

	    	return {};
		}

	    char* tReceived = new char[tByteReady];
	    memset(tReceived, 0, tByteReady);

		socklen_t tAddressSize = sizeof(mRemote);

	    int tNumOfRXChars = recvfrom(socketDescriptor(), tReceived, tByteReady, 0,
	    	                         (struct sockaddr*) &mRemote, &tAddressSize);

	    if (tNumOfRXChars != -1)
	    {
	    	tMessage.append(tReceived, tNumOfRXChars);
	    	memset(tReceived, 0, tByteReady);
	    }

	    delete [] tReceived;
    }
    while ((hasData(tByteReady) == true) && (tByteReady != 0));

    return unencapsulate(tMessage);
}

struct addrinfo UDPHandler::hints ()
{
	struct addrinfo tHints;

	std::memset(&tHints, 0, sizeof(tHints));

	// Use socket address in the call to bind
	tHints.ai_flags = AI_PASSIVE;  

	// IPv4
	tHints.ai_family = AF_UNSPEC;

	// Connectionless, unreliable
	tHints.ai_socktype = SOCK_DGRAM;

	return tHints;
}


UDPServerHandler::UDPServerHandler (TransferCallback iCallBack,
									TransferComplete iComplete) :

	UDPHandler(),

	mTransfer(iCallBack),
	mComplete(iComplete)

{}

UDPServerHandler::~UDPServerHandler ()
{
	kill();
}

bool UDPServerHandler::bindSocket (std::string iPortNum)
{
    // Create new listening socket and bind to program's host

	if ( createSocket(NULL, iPortNum.c_str()) == -1) 
		return false;

	// Attempt to bind

	open(bind(socketDescriptor(), socketInfo().ai_addr, socketInfo().ai_addrlen) != -1);

	return opened();
}

bool UDPServerHandler::start ()
{
	// Check if the servers socket has been opened (bound)

	if (opened() == false )
		return false;

	for(;;)
	{
		std::vector<std::string> tMessages = receive();

		// Pass messages back to caller

		if (mTransfer != NULL)
			tMessages = mTransfer(tMessages);

		for (unsigned int i = 0; i != tMessages.size(); ++i)
			transmit(tMessages.at(i));

		if ((mComplete != NULL) && (tMessages.size() != 0))
			mComplete();
	}

	return false;
}


TCPHandler::TCPHandler () :

	SocketHandler()

{}


TCPHandler::TCPHandler (int iDescription,
			   		    struct addrinfo iInfo,
			   		    bool iOpened) :
	
	SocketHandler(iDescription, iInfo, iOpened)

{}

TCPHandler::~TCPHandler () 
{
	kill();
}

bool TCPHandler::transmit (std::string iMessage)
{
	// Check if we are connected

	if (opened() == false)
		return false;

	// Prepare message to be transmitted

	std::string tMessage = encapsulate(iMessage);

	// unix socket method send expects (void*)
	// Turn iMessage into char*

	int tDataSize = tMessage.length()+1;
	char* tData = new char[tDataSize];

    memset(tData, 0, tDataSize);
	std::strcpy(tData, tMessage.c_str());

	// Continue sending data until whole message is
	// sent or error occurs

	char* tSending = tData;
    while (tDataSize > 0)
	{
		int tNumBytesWritten = send(socketDescriptor(), tSending, tDataSize, 0);
		if (tNumBytesWritten < 1) 
		{
			// Error or connection was closed

			kill();
			return false;
		}

		tSending += tNumBytesWritten;
		tDataSize -= tNumBytesWritten;	
	}

	delete[] tData;

	return true;
}

std::vector<std::string> TCPHandler::receive ()
{
	std::string tMessage;

    int tByteReady = 0;

    while ( (tByteReady == 0) && (hasData(tByteReady) == true) )
    	usleep(100);

    do
    {
    	// Continue reading socket until all data is read

    	if (tByteReady == -1)
		{
			// An error occurred attempting to 
			// read amount of data on socket.

	    	break;
		}

	    char* tReceived = new char[tByteReady];
	    memset(tReceived, 0, tByteReady);

	    int tNumOfRXChars = recv(socketDescriptor(), tReceived, tByteReady, 0);

	    if (tNumOfRXChars != -1)
	    {
	    	tMessage.append(tReceived, tNumOfRXChars);
	    	memset(tReceived, 0, tByteReady);
	    }


    	delete [] tReceived;

    }
    while ((hasData(tByteReady) == true) && (tByteReady != 0));


    return unencapsulate(tMessage);
}


// Protected methods

struct addrinfo TCPHandler::hints ()
{
	struct addrinfo tHints;

	std::memset(&tHints, 0, sizeof(tHints));

	// Use socket address in the call to bind
	tHints.ai_flags = AI_PASSIVE;  

	// Stream socket - reliable, two-way connection (TCP)
	tHints.ai_socktype = SOCK_STREAM;

	// Use IPv4
	tHints.ai_family = AF_INET;

	// TCP
	tHints.ai_protocol = IPPROTO_TCP;

	return tHints;
}



TCPClientHandler::TCPClientHandler () :

	TCPHandler()
{}

TCPClientHandler::~TCPClientHandler () 
{
	kill();
}

// Public methods

bool TCPClientHandler::connectSocket (std::string iHostName,
							          std::string iPortNum) 
{
    // Create socket

	if ( createSocket(iHostName.c_str(), iPortNum.c_str()) == -1) 
		return false;

	// Attempt connection. 

	open( connect(socketDescriptor(), socketInfo().ai_addr, socketInfo().ai_addrlen) != -1);

	return opened();
}

// Protected methods

struct addrinfo TCPClientHandler::hints ()
{
	struct addrinfo tHints;

	std::memset(&tHints, 0, sizeof(tHints));

	// Stream socket - reliable, two-way connection (TCP)
	tHints.ai_socktype = SOCK_STREAM;

	// Use IPv4
	tHints.ai_family = AF_INET;

	// TCP
	tHints.ai_protocol = IPPROTO_TCP;

	return tHints;
}



TCPServerHandler::TCPServerHandler (ReadCallback iRead,
									TransferComplete iComplete) :

	TCPHandler(),
	mReadCallback(iRead),
	mComplete(iComplete)

{}

TCPServerHandler::~TCPServerHandler () 
{
	kill();
}

bool TCPServerHandler::bindSocket (std::string iPortNum)
{
	// Destroys primary (listening port)

	kill();

    // Create new listening socket and bind to program's host

	if ( createSocket(NULL, iPortNum.c_str()) == -1) 
		return false;

	// Attempt to bind

	open(bind(socketDescriptor(), socketInfo().ai_addr, socketInfo().ai_addrlen) != -1);

	return opened();
}

bool TCPServerHandler::listenSocket (int iBackLog)
{
	// Attempt to begin listening to socket

	if (listen(socketDescriptor(), iBackLog) == -1)
		return false;

	struct addrinfo tIncomingSock;
	socklen_t tAddressSize;

	int tIncomingDesc = -1; 

	for(;;)
	{
		// Accept any incoming connections

		tAddressSize = sizeof(tIncomingSock);
		tIncomingDesc = accept(socketDescriptor(), (struct sockaddr*) &tIncomingSock, &tAddressSize);

		// Check if accept failed

		if (tIncomingDesc == -1)
			continue;

		TCPHandler tHandler(tIncomingDesc,
							tIncomingSock,
							true);

		std::vector<std::string> tOps = tHandler.receive();

		if (mReadCallback != NULL)
		{
			std::vector<std::string> tResults = mReadCallback(tOps);

			for (auto i = tResults.begin(); i != tResults.end(); ++i)
				tHandler.transmit((*i)) ;
		}

		if (mComplete != NULL)
			mComplete();
	}

	return false;
}

