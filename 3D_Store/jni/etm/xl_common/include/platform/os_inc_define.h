#ifndef OS_INC_DEFINE_H
#define OS_INC_EDFINE_H

#ifdef LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h> 
#include <memory.h>
#ifndef MACOS
#include <malloc.h>
#endif
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#elif defined(WINCE)
#include <winsock2.h>
#include <windows.h>
#elif defined(SUNPLUS)
#include <sys/param.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#endif

#endif
