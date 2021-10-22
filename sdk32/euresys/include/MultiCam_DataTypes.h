// MultiCam_DataTypes.h

#ifndef __EURESYS_DATA_TYPES_1_0__
#define __EURESYS_DATA_TYPES_1_0__

// Standard types
typedef int BOOL;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

typedef char CHAR, *PCHAR; 
typedef void *PVOID; 

typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;

#ifdef __GNUC__
typedef signed long long int INT64, *PINT64;
typedef unsigned long long int UINT64, *PUINT64;
#else
typedef signed __int64      INT64, *PINT64;
typedef unsigned __int64    UINT64, *PUINT64;
#endif

// Custom types
typedef unsigned short UNICHAR;
typedef float FLOAT32, *PFLOAT32;
typedef double FLOAT64, *PFLOAT64;
typedef const char *PCCHAR; 

#endif // __EURESYS_DATA_TYPES_1_0__
