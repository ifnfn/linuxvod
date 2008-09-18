#ifdef WIN32
#	if defined(UNDER_CE) && defined(sockaddr_storage)
#		undef sockaddr_storage
#	endif
#	if defined(UNDER_CE)
#		define HAVE_STRUCT_ADDRINFO
#	else
#		include <io.h>
#	endif
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	define ENETUNREACH WSAENETUNREACH
#	define SETPOINT char*
#else
#		include <sys/socket.h>
#		include <netinet/in.h>
#		include <arpa/inet.h>
#		include <netdb.h>
#	define SETPOINT void*
#endif
