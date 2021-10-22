// MCpp_globalImpl.h - MULTICAM C++ API - GlobalsImpl
#if !defined(__MCPP_GLOBAL_H__)
#define __MCPP_GLOBAL_H__

// ****************************************************************************************************
// Exception description
//
//typedef
typedef enum
{
  TYPE_NO_OP,
  TYPE_SET,
  TYPE_GET,
  TYPE_CREATE,
  TYPE_DELETE,
  TYPE_WAIT_SIGNAL,
  TYPE_GET_SIGNALINFO,
  TYPE_REGISTER_CB,
  TYPE_UNREGISTER_CB,
  TYPE_OPENDRIVER,
  TYPE_PARAMINFO,
  TYPE_ENUMERATE
} OperationType;

static const char *OperationStr[] =
{
  "",
  "Cannot set param",
  "Cannot get param",
  "Cannot create",
  "Cannot delete",
  "Wait Signal Error",
  "Get Signal Information Error",
  "Register Callback Error",
  "Unregister Callback Error",
  "Cannot open MultiCam driver",
  "Cannot get parameter info",
  "Enumerate parameters Error"
};

void MCAPI GlobalCallbackFunction(PMCSIGNALINFO CbInfo);
void ThrowMultiCamException(MCSTATUS reportedStatus, OperationType Type = TYPE_NO_OP, const char *Name = NULL, const char *Value = NULL);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE , MCPARAMID ParamId, const char *Value = NULL);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, int Value);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, INT64 Value);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, double Value);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, void *Value);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, int Value);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, INT64 Value);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, double Value);
void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, void *Value);

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // MultiCam initialization and cleanup functions
    // ---------------------------------------------
    void Initialize();
    void Terminate();
  }
}

#endif
