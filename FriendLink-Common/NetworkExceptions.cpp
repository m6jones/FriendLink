#include "NetworkExceptions.h"
#include <ws2tcpip.h>

namespace Network {
	WSADataException::WSADataException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		case WSASYSNOTREADY:
			message_ = ErrorMessages::sysNotReady;
			break;
		case WSAVERNOTSUPPORTED:
			message_ = ErrorMessages::verNotSupported;
			break;
		case WSAEINPROGRESS:
			message_ = ErrorMessages::inProgress;
			break;
		case WSAEPROCLIM:
			message_ = ErrorMessages::eProcLim;
			break;
		case WSAEFAULT:
			message_ = ErrorMessages::efault;
			break;
		default:
			message_ = ErrorMessages::unkownError;
		}
	}
	AddressException::AddressException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		case WSATRY_AGAIN:
			message_ = ErrorMessages::tryAgain;
			break;
		case WSAEINVAL:
			message_ = ErrorMessages::eInval;
			break;
		case WSANO_RECOVERY:
			message_ = ErrorMessages::noRecovery;
			break;
		case WSAEAFNOSUPPORT:
			message_ = ErrorMessages::eAFNoSupport;
			break;
		case WSA_NOT_ENOUGH_MEMORY:
			message_ = ErrorMessages::notEnoughtMemory;
			break;
		case WSAHOST_NOT_FOUND:
			message_ = ErrorMessages::hostNotFound;
			break;
		case WSATYPE_NOT_FOUND:
			message_ = ErrorMessages::typeNotFound;
			break;
		case WSAESOCKTNOSUPPORT:
			message_ = ErrorMessages::eSocktNoSupport;
			break;
		default:
			message_ = ErrorMessages::unkownError;
		}
	}

	SocketException::SocketException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		default:
			message_ = ErrorMessages::unkownError;
		}
	}

	ShutdownException::ShutdownException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		default:
			message_ = ErrorMessages::unkownError;
		}
	}
	CloseSocketException::CloseSocketException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
      case WSANOTINITIALISED:
        message_ = ErrorMessages::notInitalised;
        break;
      case WSAENETDOWN:
        message_ = ErrorMessages::eNetDown;
        break;
      case WSAENOTSOCK:
        message_ = ErrorMessages::notSock;
        break;
      case WSAEINPROGRESS:
        message_ = ErrorMessages::inProgress;
        break;
      case WSAEINTR:
        message_ = ErrorMessages::eIntr;
        break;
      case WSAEWOULDBLOCK:
        message_ = ErrorMessages::wouldBlock;
        break;
      default:
        message_ = ErrorMessages::unkownError;
    }
	}
	ConnectException::ConnectException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		default:
			message_ = ErrorMessages::unkownError;
		}
	}
	SendException::SendException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		default:
			message_ = ErrorMessages::unkownError;
		}
	}

	RecvException::RecvException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		default:
			message_ = ErrorMessages::unkownError;
		}
	}

	BindException::BindException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		default:
			message_ = ErrorMessages::unkownError;
		}
	}
	ListenException::ListenException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		default:
			message_ = ErrorMessages::unkownError;
		}
	}
	AcceptException::AcceptException(int errCode)
		: NetworkException(topMessage, errCode) {
		switch (code_) {
		case WSANOTINITIALISED:
			message_ = ErrorMessages::notInitalised;
			break;
		case WSAECONNRESET:
			message_ = ErrorMessages::eConnReset;
			break;
		case WSAEFAULT:
			message_ = ErrorMessages::efault;
			break;
		case WSAEINTR:
			message_ = ErrorMessages::eIntr;
			break;
		case WSAEINVAL:
			message_ = ErrorMessages::eInval;
			break;
		case WSAEINPROGRESS:
			message_ = ErrorMessages::inProgress;
			break;
		case WSAEMFILE:
			message_ = ErrorMessages::eMFile;
			break;
		case WSAENETDOWN:
			message_ = ErrorMessages::eNetDown;
			break;
		case WSAENOBUFS:
			message_ = ErrorMessages::eNoBufs;
			break;
		case WSAENOTSOCK:
			message_ = ErrorMessages::notSock;
			break;
		case WSAEOPNOTSUPP:
			message_ = ErrorMessages::eOpNotSupp;
			break;
		case WSAEWOULDBLOCK:
			message_ = ErrorMessages::wouldBlock;
			break;
		default:
			message_ = ErrorMessages::unkownError;
		}
	}
  SocketOptionException::SocketOptionException(int errCode)
    : NetworkException(topMessage, errCode) {
    switch (code_) {
    default:
      message_ = ErrorMessages::unkownError;
    }
  }
}