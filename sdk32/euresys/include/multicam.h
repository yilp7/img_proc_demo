// MULTICAM.H -- MULTICAM DRIVER API

#ifndef _MULTICAM_
#define _MULTICAM_

#if defined(WINAPI)
#define MCAPI WINAPI
#elif defined(__BORLANDC__)
#if __BORLANDC__ >= 0x0570
#define MCAPI __stdcall
#else
#define MCAPI stdcall
#endif
#elif defined(__GNUC__)
#define MCAPI
#else
#define MCAPI _stdcall
#endif

#if defined(DLL_EXPORTS) && defined(__GNUC__)
#define DLL_API  __attribute__((visibility("default")))
#elif defined(DLL_EXPORTS)
#define DLL_API __declspec(dllexport) MCAPI
#elif defined(__BORLANDC__)
#define DLL_API _import MCAPI
#elif defined(__GNUC__)
#define DLL_API
#elif defined(EURESYS_STATIC)
#define DLL_API
#else
#define DLL_API __declspec(dllimport) MCAPI
#endif

// MULTICAM PARAMETERS AND DEFINITIONS
#include "McParams.h"
#include "McDef.h"
#include "McIo.h"
#if !defined(DLL_EXPORTS)
#include "McParamsCompat1x.h"
#endif

#include "MultiCam_DataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int MCSTATUS;
typedef UINT32 MCHANDLE, *PMCHANDLE;
typedef UINT32 MCPARAMID, *PMCPARAMID;
typedef int MCEVENT, MCSIGNAL;


// SIGNALING
typedef struct _MC_CALLBACK_INFO {
	PVOID Context;
	MCHANDLE Instance;
	MCSIGNAL Signal;
	UINT32 SignalInfo;
	UINT32 SignalContext;
} MCSIGNALINFO, *PMCSIGNALINFO, *PMCCALLBACKINFO, MCCALLBACKINFO;

typedef void (MCAPI *PMCCALLBACK)(PMCSIGNALINFO CbInfo);
typedef void (MCAPI *PMCCALLBACKEX)(PVOID Context);

// OBJECT CLASSES
#define MC_SURFACE_CLASS		0x4
#define MC_CHANNEL_CLASS		0x8
#define MC_CONFIG_CLASS			0x2
#define MC_BOARD_CLASS			0xE


// DEFAULT INSTANCE TEMPLATES
#define MC_DEFAULT_SURFACE_HANDLE (MCHANDLE)((MC_SURFACE_CLASS << 28) | 0x0FFFFFFF)

#define MC_CONFIGURATION			(MCHANDLE)((MC_CONFIG_CLASS << 28) | 0)

#define MC_BOARD					(MCHANDLE)((MC_BOARD_CLASS << 28) | 0)

// CHANNEL CREATION MODELS
#define MC_CHANNEL					(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x0000FFFF)
#define MC_CHANNEL_ALPHA_X			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x0007FFFF)
#define MC_CHANNEL_ALPHA_Y			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x0008FFFF)
#define MC_CHANNEL_ALPHA_XBIS		(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x0015FFFF)
#define MC_CHANNEL_ALPHA_YBIS		(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x0016FFFF)
#define MC_CHANNEL_ALPHA_STEREOXY	(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x001CFFFF)
#define MC_CHANNEL_VALUE_M			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x0009FFFF)
#define MC_CHANNEL_EXPERT_M			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x000AFFFF)
#define MC_CHANNEL_EXPERT_A			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x000BFFFF)
#define MC_CHANNEL_EXPERT_B			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x000CFFFF)
#define MC_CHANNEL_HARMONY			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x001EFFFF)
#define MC_CHANNEL_MELODY			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x001FFFFF)
#define MC_CHANNEL_AVENUE			(MCHANDLE)((MC_CHANNEL_CLASS << 28) | 0x0024FFFF)


#if !defined(MC_DYNAMIC_BINDING)

// STATIC INSTANCE OBJECTS
// FUNCTIONS

// Driver connections
MCSTATUS DLL_API McOpenDriver(PCCHAR InstanceName);
MCSTATUS DLL_API McCloseDriver(void);

// Object creation / deletion
MCSTATUS DLL_API McCreate(MCHANDLE ModelInstance, PMCHANDLE Instance);
MCSTATUS DLL_API McCreateNm(PCCHAR ModelInstanceName, PMCHANDLE Instance);
MCSTATUS DLL_API McDelete(MCHANDLE Instance);

// Acquisition control
MCSTATUS DLL_API McStartAcq(void);
MCSTATUS DLL_API McStopAcq(void);

// Parameter handling
MCSTATUS DLL_API McSetParamInt(MCHANDLE Instance, MCPARAMID Param, INT32 ValueInt);
MCSTATUS DLL_API McSetParamNmInt(MCHANDLE Instance, PCCHAR ParamName, INT32 ValueInt);
MCSTATUS DLL_API McSetParamStr(MCHANDLE Instance, MCPARAMID Param, PCCHAR ValueStr);
MCSTATUS DLL_API McSetParamNmStr(MCHANDLE Instance, PCCHAR ParamName, PCCHAR ValueStr);
MCSTATUS DLL_API McSetParamFloat(MCHANDLE Instance, MCPARAMID Param, FLOAT64 ValueFloat);
MCSTATUS DLL_API McSetParamNmFloat(MCHANDLE Instance, PCCHAR ParamName, FLOAT64 ValueFloat);
MCSTATUS DLL_API McSetParamInst(MCHANDLE Instance, MCPARAMID Param, MCHANDLE ValueInst);
MCSTATUS DLL_API McSetParamNmInst(MCHANDLE Instance, PCCHAR ParamName, MCHANDLE ValueInst);
MCSTATUS DLL_API McSetParamPtr(MCHANDLE Instance, MCPARAMID Param, PVOID ValuePtr);
MCSTATUS DLL_API McSetParamNmPtr(MCHANDLE Instance, PCCHAR ParamName, PVOID ValuePtr);
MCSTATUS DLL_API McSetParamInt64(MCHANDLE Instance, MCPARAMID Param, INT64 ValueInt64);
MCSTATUS DLL_API McSetParamNmInt64(MCHANDLE Instance, PCCHAR ParamName, INT64 ValueInt64);

MCSTATUS DLL_API McGetParamInt(MCHANDLE Instance, MCPARAMID Param, PINT32 ValueInt);
MCSTATUS DLL_API McGetParamNmInt(MCHANDLE Instance, PCCHAR ParamName, PINT32 ValueInt);
MCSTATUS DLL_API McGetParamStr(MCHANDLE Instance, MCPARAMID Param, PCHAR ValueStr, UINT32 MaxLength);
MCSTATUS DLL_API McGetParamNmStr(MCHANDLE Instance, PCCHAR ParamName, PCHAR ValueStr, UINT32 MaxLength);
MCSTATUS DLL_API McGetParamFloat(MCHANDLE Instance, MCPARAMID Param, PFLOAT64 ValueFloat);
MCSTATUS DLL_API McGetParamNmFloat(MCHANDLE Instance, PCCHAR ParamName, PFLOAT64 ValueFloat);
MCSTATUS DLL_API McGetParamInst(MCHANDLE Instance, MCPARAMID Param, PMCHANDLE ValueInst);
MCSTATUS DLL_API McGetParamNmInst(MCHANDLE Instance, PCCHAR ParamName, PMCHANDLE ValueInst);
MCSTATUS DLL_API McGetParamPtr(MCHANDLE Instance, MCPARAMID Param, PVOID *ValuePtr);
MCSTATUS DLL_API McGetParamNmPtr(MCHANDLE Instance, PCCHAR ParamName, PVOID *ValuePtr);
MCSTATUS DLL_API McGetParamInt64(MCHANDLE Instance, MCPARAMID Param, PINT64 ValueInt64);
MCSTATUS DLL_API McGetParamNmInt64(MCHANDLE Instance, PCCHAR ParamName, PINT64 ValueInt64);

// Callback registration for an object
MCSTATUS DLL_API McRegisterCallback(MCHANDLE Instance, PMCCALLBACK CallBackFunction, PVOID Context);
MCSTATUS DLL_API McRegisterCallbackEx(MCHANDLE Instance, PMCCALLBACK CallBackFunction, PMCCALLBACKEX ThreadStart, PMCCALLBACKEX ThreadEnd, PVOID Context);

// Event Management
MCSTATUS DLL_API McWaitSignal(MCHANDLE Instance, MCSIGNAL Signal, UINT32 Timeout, PMCSIGNALINFO Info);
MCSTATUS DLL_API McGetSignalInfo(MCHANDLE Instance, MCSIGNAL Signal, PMCSIGNALINFO Info);

#endif	// MC_DYNAMIC_BINDING

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _MULTICAM_
