#if !defined(WIN32)
#include <sys/socket.h>
typedef int SOCKET_FD_TYPE;
#else
#include <winsock2.h>
typedef SOCKET SOCKET_FD_TYPE;
#undef X509_NAME
#undef DELETE
#undef ERROR
#undef TRUE
#endif