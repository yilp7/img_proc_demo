
// MCDEF.H -- MULTICAM DEFINITIONS

#ifndef _MULTICAM_DEF_
#define _MULTICAM_DEF_

// CALLBACK REASONS
#define MC_MAX_EVENTS                       12
#define MC_SIG_ANY                          0
#define MC_SIG_SURFACE_PROCESSING           1
#define MC_SIG_SURFACE_FILLED               2
#define MC_SIG_UNRECOVERABLE_OVERRUN        3
#define MC_SIG_FRAMETRIGGER_VIOLATION       4
#define MC_SIG_START_EXPOSURE               5
#define MC_SIG_END_EXPOSURE                 6
#define MC_SIG_ACQUISITION_FAILURE          7
#define MC_SIG_CLUSTER_UNAVAILABLE          8
#define MC_SIG_RELEASE                      9
#define MC_SIG_END_ACQUISITION_SEQUENCE     10
#define MC_SIG_START_ACQUISITION_SEQUENCE   11
#define MC_SIG_END_CHANNEL_ACTIVITY         12

#define MC_SIG_GOLOW            (1 << 12)
#define MC_SIG_GOHIGH           (2 << 12)
#define MC_SIG_GOOPEN           (3 << 12)

#define MC_MAX_BOARD_EVENTS     (3 << 12)

// STATUS CODE
#define MC_MAX_ERRORS_STD               27
#define MC_OK                           0
#define MC_NO_BOARD_FOUND               -1
#define MC_BAD_PARAMETER                -2
#define MC_IO_ERROR                     -3
#define MC_INTERNAL_ERROR               -4
#define MC_NO_MORE_RESOURCES            -5
#define MC_IN_USE                       -6
#define MC_NOT_SUPPORTED                -7
#define MC_DATABASE_ERROR               -8
#define MC_OUT_OF_BOUND                 -9
#define MC_INSTANCE_NOT_FOUND           -10
#define MC_INVALID_HANDLE               -11
#define MC_TIMEOUT                      -12
#define MC_INVALID_VALUE                -13
#define MC_RANGE_ERROR                  -14
#define MC_BAD_HW_CONFIG                -15
#define MC_NO_EVENT                     -16
#define MC_LICENSE_NOT_GRANTED          -17
#define MC_FATAL_ERROR                  -18
#define MC_HW_EVENT_CONFLICT            -19
#define MC_FILE_NOT_FOUND               -20
#define MC_OVERFLOW                     -21
#define MC_INVALID_PARAMETER_SETTING    -22
#define MC_PARAMETER_ILLEGAL_ACCESS     -23
#define MC_CLUSTER_BUSY                 -24
#define MC_SERVICE_ERROR                -25
#define MC_INVALID_SURFACE              -26

#define MC_MAX_ERRORS_MPF               3
#define MC_MPF_ERROR_BASE               -100
#define MC_BAD_GRABBER_CONFIG           -101
#define MC_ILLEGAL_PAGELENGTH_VALUE     -102

// MULTICAM CONSTANTS
#define MC_INFINITE         -1
#define MC_INDETERMINATE    -1
#define MC_LOW_PART         0
#define MC_HIGH_PART        1
#define MC_DISABLE          0
#define MC_UNKNOWN          -2

#endif // _MULTICAM_DEF_
