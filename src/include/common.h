// Client-Server_client_service common definition


#ifndef COMMON_H
#define COMMON_H


/* Include files:
*****************/
// Standard Library:
// "cstdlib" could be removed if in the future a ".config" file is
// used for reading the arguments - some other include files
// will be used for the file solution
#include <cstdlib> // Use of atoi()
#include <stdint.h> // Use of fixed width int types
// Application Specific:
#include "Socket.h" // The socket lib
#include "Thread.h" // The thread lib
// DEBUG Specific:
// Uncomment the next #define line for setting the DEBUG definition
// OR use "g++ -D USEDEBUG .." to define USEDEBUG at compile time
//#define USEDEBUG
#include "Debug.h"



/* Parameter Definition:
************************/
// Define max number of clients
#define MAX_CLIENT_ID 10
// Define max client-server communication failed attempts
#define MAX_COMM_ATTEMPTS 100

#endif
