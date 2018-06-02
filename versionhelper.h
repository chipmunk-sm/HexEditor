/* Copyright (C) 2017 chipmunk-sm <dannico@linuxmail.org> */

#ifndef VERSIONHELPER_H
#define VERSIONHELPER_H

#include "ver.h"

#define stringify(X) #X
#define quote(X)  stringify(X)

#ifndef FVER1
#define FVER1 0
#endif // !FVER1

#ifndef FVER2
#define FVER2 0
#endif // !FVER2

#ifndef FVER3
#define FVER3 0
#endif // !FVER3

#ifndef FVER4
#define FVER4 0
#endif // !FVER4

#if FVER1 > 65535
#error FVER1 overflow! Max 65535 for use in FILEVERSION
#endif // !FVER1

#if FVER2 > 65535
#error FVER2 overflow! Max 65535 for use in FILEVERSION
#endif // !FVER2

#if FVER3 > 65535
#error FVER3 overflow! Max 65535 for use in FILEVERSION
#endif // !FVER3

#if FVER4 > 65535
#error FVER4 overflow! Max 65535 for use in FILEVERSION
#endif // !FVER4

#ifndef FVER_NAME
#define FVER_NAME "test build"
#endif // !FVER_NAME

#ifndef PRODUCTVERSIONSTRING
#define PRODUCTVERSIONSTRING quote(FVER1.FVER2.FVER3.FVER4) " " FVER_NAME
#endif // !PRODUCTVERSIONSTRING

#ifndef S_VER_YEAR
    #ifndef BUILDDATETIME
    #define BUILDDATETIME __DATE__ " " __TIME__
    #endif // !BUILDDATETIME
#else
    #ifndef BUILDDATETIME
    #define BUILDDATETIME S_VER_YEAR "." S_VER_MONTH "." S_VER_DAY " " S_VER_HOUR ":" S_VER_MINUTE ":" S_VER_SECOND
    #endif // !BUILDDATETIME
#endif // !

#endif // !VERSIONHELPER_H
