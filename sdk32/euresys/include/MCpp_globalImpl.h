// MCpp_globalImpl.h - MULTICAM C++ API - GlobalsImpl
#if !defined(__MCPP_GLOBALIMPL_H__)
#define __MCPP_GLOBALIMPL_H__

#include "MCpp_global.h"
#include <cstdio>

// ****************************************************************************************************
// Global callback function
// ------------------------

inline void MCAPI GlobalCallbackFunction(PMCSIGNALINFO CbInfo)
{
  Euresys::MultiCam::MultiCamObjectWithSignaling *signaler = reinterpret_cast<Euresys::MultiCam::MultiCamObjectWithSignaling *>(CbInfo->Context);
  if (signaler != NULL)
    signaler->CbRoutine(CbInfo);
}


inline void ThrowMultiCamException(MCSTATUS reportedStatus, OperationType Type, const char *Name, const char *Value)
{
  int aStatus = -reportedStatus;
  char ErrorDesc[256];

  char Description[MCPP_MAX_EXCEPTION_DESCRIPTION_SIZE];

  if (McGetParamStr(MC_CONFIGURATION, MC_ErrorDesc+aStatus, ErrorDesc, 256) != MC_OK)
    sprintf(ErrorDesc, "Unknown error");

  if (Type == TYPE_NO_OP)
    sprintf(Description, "%s", ErrorDesc);

  else if (Type == TYPE_SET)
    sprintf(Description, "%s '%s' to value '%s': %s", OperationStr[Type], Name? Name:"Unknown parameter", Value? Value:"", ErrorDesc);

  else if (Type == TYPE_GET)
    sprintf(Description, "%s '%s': %s", OperationStr[Type], Name? Name:"unknown parameter", ErrorDesc);

  else if (Type == TYPE_CREATE || Type == TYPE_DELETE)
    sprintf(Description, "%s %s: %s", OperationStr[Type], Name? Name:"", ErrorDesc);

  else
    sprintf(Description, "%s: %s", OperationStr[Type], ErrorDesc);

  Euresys::MultiCam::Exception e(reportedStatus, Description);
  throw e;
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE , MCPARAMID ParamId, const char *Value)
{
  char paramStr[64];

  sprintf(paramStr, "%i", ParamId);
  ThrowMultiCamException(reportedStatus, Type, paramStr, Value);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, int Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "%d", Value);
  ThrowMultiCamException(reportedStatus, Type, Name, ValueStr);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, INT64 Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "0x%llx", Value);
  ThrowMultiCamException(reportedStatus, Type, Name, ValueStr);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, double Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "%f", Value);
  ThrowMultiCamException(reportedStatus, Type, Name, ValueStr);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, const char *Name, void *Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "%p", Value);
  ThrowMultiCamException(reportedStatus, Type, Name, ValueStr);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, int Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "%d", Value);
  ThrowMultiCamException(reportedStatus, Type, Instance, ParamId, ValueStr);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, double Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "%f", Value);
  ThrowMultiCamException(reportedStatus, Type, Instance, ParamId, ValueStr);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, INT64 Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "0x%llx", Value);
  ThrowMultiCamException(reportedStatus, Type, Instance, ParamId, ValueStr);
}

inline void ThrowMultiCamException (MCSTATUS reportedStatus, OperationType Type, MCHANDLE Instance, MCPARAMID ParamId, void *Value)
{
  char ValueStr[64];
  sprintf(ValueStr, "%p", Value);
  ThrowMultiCamException(reportedStatus, Type, Instance, ParamId, ValueStr);
}

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // MultiCam initialization and cleanup functions
    // ---------------------------------------------
    inline void Initialize()
    {
      MCSTATUS status;
      status = McOpenDriver(NULL);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_OPENDRIVER);
      Boards.Init();
    }

    inline void Terminate()
    {
      McCloseDriver();
    }
  }
}

#endif
