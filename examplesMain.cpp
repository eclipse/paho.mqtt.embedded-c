/*
 * Used for examples compilation via PlatformIO.
 * See platformio.ini build_flags=-DMAIN_CPP_FILE=<value> option
 */

#define QUOTEME(M)										#M
#define INCLUDE_FILE(M)									QUOTEME(M)

#include INCLUDE_FILE(MAIN_CPP_FILE)
