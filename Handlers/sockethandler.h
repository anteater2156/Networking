#ifndef __TCP_HANDLER_H
#define __TCP_HANDLER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <string>
#include <vector>


class SocketHandler
{
public:

	// Helper class which creates and kills sockets

	SocketHandler ();
	SocketHandler (int iDescription,
				   struct addrinfo iInfo,
				   bool iOpened = false);

	virtual ~SocketHandler ();

	// Accessors

	// Check if the port is ready to send or receive data

	virtual bool opened () const { return mOpened; }

	// This function attempts to read the amount of
	// data on the socket. If read is successful
	// true is returned and oAmount is set with the
	// the amount of data (including 0) on the socket.
	// If read is unsuccessful, function returns
	// false and oAmount is set to -1

	virtual bool hasData (int& oAmount) const;

	// Returns socket. Note: returns -1 if
	// a valid socket has not been created

	int socketDescriptor () const { return mSocketDescr; }

	// Returns the socket information

	struct addrinfo socketInfo () const { return mSocketInfo; }


	// Creates a socket

	virtual int createSocket (const char* iHostName,
					  	      const char* iPortNum);

	virtual bool transmit (std::string iMessage) = 0;

	virtual std::vector<std::string> receive () = 0;

	// Closes socket 

	virtual void kill ();

protected:

	// encapsulate will add a header to
	// the message to be transmitted. 

	std::string encapsulate (std::string iMessage);

	// Will search through received string and find
	// all messages. Note: that more than one message
	// can be received at once. Returns vector of messages
	// without header

	std::vector<std::string> unencapsulate (std::string iReceived);

	virtual void open (bool iEnabled);

	virtual struct addrinfo hints () = 0;

private:

	int mSocketDescr;	
	bool mOpened;

	struct addrinfo mSocketInfo;
};

class UDPHandler : public SocketHandler
{
	// UDP Handler class

public:

	UDPHandler ();	
	UDPHandler (int iDescription,
				struct addrinfo iInfo,
				bool iOpened = false);
	
	virtual ~UDPHandler ();

	// Sets the remote address used for the 
	// udp connection

	bool setRemoteAddress (const char* iHostName,
					  	   const char* iPortNum);


	// These functions are to send / receive 
	// messages over the udp connection.

	// Note: MUST have remote address set before
	//       transmiting

	virtual bool transmit (std::string iMessage);

	// Returns a vector of messages received on the
	// UDP port

	virtual std::vector<std::string> receive ();

protected:

	virtual struct addrinfo hints ();

private:

	struct sockaddr_storage mRemote;

};

class UDPServerHandler : public UDPHandler
{
	// UDP server for handling operations.

public:

	// Callback function ReadCallback is called
	// when the server is done receiving a message.
	// - iData is received message
	// - returns the reply to received message

	// NOTE: This is a synchronous message. In other words,
	//		 the server will not be accepting new messages
	//	     until this function returns

	typedef std::vector<std::string> (*TransferCallback)(std::vector<std::string> iData);

	// Callback used when UDP Server is done transmitting

	typedef void (*TransferComplete) ();


	UDPServerHandler (TransferCallback iCallBack = NULL,
					  TransferComplete iComplete = NULL);
	virtual ~UDPServerHandler ();

	// This function will create a socket and bind it
	// to a port on the host running this program

	bool bindSocket (std::string iPortNum);

	// This starts the server's loop.
	// It will receive and send data.
	// Note: it uses the callback to pass
	//	     data between loop and caller

	bool start ();

private:

	TransferCallback mTransfer;
	TransferComplete mComplete;

};

class TCPHandler : public SocketHandler
{
	// TCP Handler class 

public:

	TCPHandler ();
	TCPHandler (int iDescription,
				struct addrinfo iInfo,
				bool iOpened = false);

	virtual ~TCPHandler ();

	// Mutators

	// Attempts to send iMessage over the 
	// TCP connection. Function blocks until
	// message sends

	virtual bool transmit (std::string iMessage);

	// Returns a vector of messages received on the
	// TCP port

	virtual std::vector<std::string>  receive ();

protected:

	virtual struct addrinfo hints ();
};


class TCPClientHandler : public TCPHandler
{
public:

	TCPClientHandler ();
	virtual ~TCPClientHandler ();

	// Client based function. Will attempt a connection
	// to iHostname on port iPortNum

	bool connectSocket (std::string iHostName,
						std::string iPortNum );
	
protected:

	virtual struct addrinfo hints ();

};


class TCPServerHandler : public TCPHandler
{
public:

	// Callback functions 
	// Note: The listen() function blocks, BUT
	// it will call the callback functions when
	// it has accepted a port and is reading/writing 
	// from/to it

	typedef std::vector<std::string> (*ReadCallback) (std::vector<std::string> iData);

	// Callback used when TCP Server is done transmitting

	typedef void (*TransferComplete) ();

	TCPServerHandler ( ReadCallback iRead = NULL,
					   TransferComplete iComplete = NULL );
	virtual ~TCPServerHandler ();

	// This function will create a socket and bind it
	// to a port on the host running this program

	bool bindSocket (std::string iPortNum);

	// This function is a blocking function.
	// It uses the callbacks to pass data between
	// blocking loop and other objects
	// Note: The parameter iBackLog sets the num
	// of incoming connections.
	// return false when execution is over

	bool listenSocket (int iBackLog = 5);
	
private:

	ReadCallback mReadCallback;
	TransferComplete mComplete;
};

#endif