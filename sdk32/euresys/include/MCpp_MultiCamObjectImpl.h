// MCpp_MulticamObjectImpl.h - MULTICAM C++ API - MulticamObjectImpl
#if !defined(__MCPP_MULTICAMOBJECTIMPL_H__)
#define __MCPP_MULTICAMOBJECTIMPL_H__

#include "MCpp_MultiCamObject.h"

namespace Euresys
{
  namespace MultiCam
  {
    inline MultiCamObject::MultiCamObject() : Handle(0)
    {
    }

    // Get Surface from handle
    inline Surface *MultiCamObject::GetSurface(MCHANDLE )
    {
      ThrowMultiCamException(MC_BAD_PARAMETER);
      return NULL;
    }

    // SetParam
    inline void MultiCamObject::SetParam(MCPARAMID Param, int Value)
    {
      MCSTATUS status;

      status = McSetParamInt(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Handle, Param, Value);
    }
    inline void MultiCamObject::SetParam(MCPARAMID Param, unsigned int Value)
    {
      MCSTATUS status;

      status = McSetParamInt(Handle, Param, (INT32)Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Handle, Param, (int)Value);
    }
    inline void MultiCamObject::SetParam(MCPARAMID Param, INT64 Value)
    {
      MCSTATUS status;

      status = McSetParamInt64(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Handle, Param, Value);
    }
    inline void MultiCamObject::SetParam(MCPARAMID Param, const char *Value)
    {
      MCSTATUS status;

      status = McSetParamStr(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Handle, Param, Value);
    }
    inline void MultiCamObject::SetParam(MCPARAMID Param, double Value)
    {
      MCSTATUS status;

      status = McSetParamFloat(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Handle, Param, Value);
    }
    inline void MultiCamObject::SetParam(MCPARAMID Param, Surface &Value)
    {
      MCSTATUS status;

      status = McSetParamInst(Handle, Param, Value.Handle);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Handle, Param, (int)Value.Handle);
    }
    inline void MultiCamObject::SetParam(MCPARAMID Param, void *Value)
    {
      MCSTATUS status;

      status = McSetParamPtr(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Handle, Param, Value);
    }
    inline void MultiCamObject::SetParam(const char *Param, int Value)
    {
      MCSTATUS status;

      status = McSetParamNmInt(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Param, Value);
    }
    inline void MultiCamObject::SetParam(const char *Param, unsigned int Value)
    {
      MCSTATUS status;

      status = McSetParamNmInt(Handle, Param, (INT32)Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Param, (int)Value);
    }
    inline void MultiCamObject::SetParam(const char *Param, INT64 Value)
    {
      MCSTATUS status;

      status = McSetParamNmInt64(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Param, Value);
    }
    inline void MultiCamObject::SetParam(const char *Param, const char *Value)
    {
      MCSTATUS status;

      status = McSetParamNmStr(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Param, Value);
    }
    inline void MultiCamObject::SetParam(const char *Param, double Value)
    {
      MCSTATUS status;

      status = McSetParamNmFloat(Handle, Param, Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Param, Value);
    }
    inline void MultiCamObject::SetParam(const char *Param, Surface &Value)
    {
      MCSTATUS status;

      status = McSetParamNmInst(Handle, Param, Value.Handle);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_SET, Param, (int)Value.Handle);
    }
    inline void MultiCamObject::SetParam(const char *Param, void *Value)
    {
      MCSTATUS status;

      status = McSetParamNmPtr(Handle, Param, Value);
      if (status != MC_OK)
          ThrowMultiCamException(status, TYPE_SET, Param, Value);
    }

    // GetParam
    inline void MultiCamObject::GetParam(MCPARAMID Param, int &Value)
    {
      MCSTATUS status;

      status = McGetParamInt(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Handle, Param);
    }
    inline void MultiCamObject::GetParam(MCPARAMID Param, unsigned int &Value)
    {
      MCSTATUS status;

      status = McGetParamInt(Handle, Param, (PINT32)&Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Handle, Param);
    }
    inline void MultiCamObject::GetParam(MCPARAMID Param, INT64 &Value)
    {
      MCSTATUS status;

      status = McGetParamInt64(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Handle, Param);
    }
    inline void MultiCamObject::GetParam(MCPARAMID Param, char *Value, int MaxLength)
    {
      MCSTATUS status;

      status = McGetParamStr(Handle, Param, Value, MaxLength);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Handle, Param);
    }
    inline void MultiCamObject::GetParam(MCPARAMID Param, double &Value)
    {
      MCSTATUS status;

      status = McGetParamFloat(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Handle, Param);
    }
    inline void MultiCamObject::GetParam(MCPARAMID Param, Surface *&Value)
    {
      MCSTATUS status;
      MCHANDLE sHandle;

      status = McGetParamInst(Handle, Param, &sHandle);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Handle, Param);

      Value = GetSurface(sHandle);
    }
    inline void MultiCamObject::GetParam(MCPARAMID Param, void *&Value)
    {
      MCSTATUS status;

      status = McGetParamPtr(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Handle, Param);
    }
    inline void MultiCamObject::GetParam(const char *Param, int &Value)
    {
      MCSTATUS status;

      status = McGetParamNmInt(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Param);
    }
    inline void MultiCamObject::GetParam(const char *Param, unsigned int &Value)
    {
      MCSTATUS status;

      status = McGetParamNmInt(Handle, Param, (PINT32)&Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Param);
    }
    inline void MultiCamObject::GetParam(const char *Param, INT64 &Value)
    {
      MCSTATUS status;

      status = McGetParamNmInt64(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Param);
    }
    inline void MultiCamObject::GetParam(const char *Param, char *Value, int MaxLength)
    {
      MCSTATUS status;

      status = McGetParamNmStr(Handle, Param, Value, MaxLength);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Param);
    }
    inline void MultiCamObject::GetParam(const char *Param, double &Value)
    {
      MCSTATUS status;

      status = McGetParamNmFloat(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Param);
    }
    inline void MultiCamObject::GetParam(const char *Param, Surface *&Value)
    {
      MCSTATUS status;
      MCHANDLE sHandle;

      status = McGetParamNmInst(Handle, Param, &sHandle);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Param);

      Value = GetSurface(sHandle);
    }
    inline void MultiCamObject::GetParam(const char *Param, void *&Value)
    {
      MCSTATUS status;

      status = McGetParamNmPtr(Handle, Param, &Value);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET, Param);
    }

    // GetHandle
    inline MCHANDLE MultiCamObject::GetHandle()
    {
      return Handle;
    }
  }
}

#endif
