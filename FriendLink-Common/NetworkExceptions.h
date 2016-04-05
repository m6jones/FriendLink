#pragma once
#include <exception>
#include <stdexcept>

namespace Network {
	struct ErrorMessages {
		//Reports the WSA Error as quoted from the winsock api.
		static constexpr auto sysNotReady = "The underlying network subsystem is not ready for network communication.";
		static constexpr auto verNotSupported = "The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.";
		static constexpr auto eProcLim = "A limit on the number of tasks supported by the Windows Sockets implementation has been reached.";
		static constexpr auto efault = "The lpWSAData parameter is not a valid pointer.";
		static constexpr auto tryAgain = "A temporary failure in name resolution occurred.";
		static constexpr auto eInval = "An invalid value was provided for the ai_flags member of the pHints parameter.";
		static constexpr auto noRecovery = "A nonrecoverable failure in name resolution occurred.";
		static constexpr auto eAFNoSupport = "The ai_family member of the pHints parameter is not supported.";
		static constexpr auto notEnoughtMemory = "The lpWSAData parameter is not a valid pointer.";
		static constexpr auto hostNotFound = "The name does not resolve for the supplied parameters or the pNodeName and pServiceName parameters were not provided.";
		static constexpr auto typeNotFound = "The pServiceName parameter is not supported for the specified ai_socktype member of the pHints parameter.";
		static constexpr auto eSocktNoSupport = "The ai_socktype member of the pHints parameter is not supported.";
		static constexpr auto notInitalised = "A successful WSAStartup call must occur before using this function.";
		static constexpr auto eNetDown = "The network subsystem has failed.";
		static constexpr auto inProgress = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
		static constexpr auto notSock = "The descriptor is not a socket.";
		static constexpr auto eIntr = "The (blocking) Windows Socket 1.1 call was canceled through WSACancelBlockingCall.";
		static constexpr auto wouldBlock = "The socket is marked as nonblocking, but the l_onoff member of the linger structure is set to nonzero and the l_linger member of the linger structure is set to a nonzero timeout value.";
		static constexpr auto eConnReset = "An incoming connection was indicated, but was subsequently terminated by the remote peer prior to accepting the call.";
		static constexpr auto eMFile = "The queue is nonempty upon entry to accept and there are no descriptors available.";
		static constexpr auto eNoBufs = "No buffer space is available.";
		static constexpr auto eOpNotSupp = "The referenced socket is not a type that supports connection-oriented service.";
		static constexpr auto unkownError = "Unknown Error";
	};

	class NetworkException : public std::runtime_error {
	public:
		NetworkException(std::string topMessage, int errCode) : std::runtime_error(topMessage), code_{ errCode } {}
		virtual std::string message() { return message_; }
		virtual int code() { return code_; }
	protected:
		std::string message_;
		int code_;
	};
	class WSADataException : public NetworkException {
		static constexpr auto topMessage = "WSA Initalize Error";
		
	public:
		WSADataException(int wsaErrorCode);
	};

	class AddressException : public NetworkException {
		static constexpr auto topMessage = "Get Address Failed";
	public:
		AddressException(int errCode);
	};

	class SocketException : public NetworkException {
		static constexpr auto topMessage = "Socket Failed to Create";
	public:
		SocketException(int errCode);
	};

	class ShutdownException : public NetworkException {
		static constexpr auto topMessage = "Socket Failed to Shutdown";
	public:
		ShutdownException(int errCode);
	};
	class CloseSocketException : public NetworkException {
		static constexpr auto topMessage = "Socket Failed to close.";
	public:
		CloseSocketException(int errCode);
	};
	class ConnectException : public NetworkException {
		static constexpr auto topMessage = "Unable to Connect";
	public:
		ConnectException(int errCode);
	};
	class SendException : public NetworkException {
		static constexpr auto topMessage = "Unable to send data";
	public:
		SendException(int errCode);
	};
	class RecvException : public NetworkException {
		static constexpr auto topMessage = "Unable to recv data";
	public:
		RecvException(int errCode);
	};
	class InitialMessageException : public std::runtime_error {
		static constexpr auto topMessage = "Initial Message Error.";
	public:
    InitialMessageException(std::string message) 
        : std::runtime_error(topMessage+message) {}
	};
	class BindException : public NetworkException {
		static constexpr auto topMessage = "Unable to bind local address.";
	public:
		BindException(int errCode);
	};

	class ListenException : public NetworkException {
		static constexpr auto topMessage = "Unable to listen on socket.";
	public:
		ListenException(int errCode);
	};

	class AcceptException : public NetworkException {
		static constexpr auto topMessage = "Unable to accept connection";
	public:
		AcceptException(int errCode);
	};
  class SocketOptionException : public NetworkException {
    static constexpr auto topMessage = "Unable to accept connection";
  public:
    SocketOptionException(int errCode);
  };
}
