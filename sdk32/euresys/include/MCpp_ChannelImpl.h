// MCpp_ChannelImpl.h - MULTICAM C++ API - ChannelImpl
#if !defined(__MCPP_CHANNELIMPL_H__)
#define __MCPP_CHANNELIMPL_H__

#include "MCpp_Channel.h"

namespace Euresys
{
  namespace MultiCam
  {
    // Constructors
    inline Channel::Channel(Board *b, int Connector) :
    SurfaceList(NULL)
    {
      try {
        InitChannel(b);
        SetParam(MC_Connector, Connector);
      } catch (...) {
        if (Handle != 0)
          McDelete(Handle);
        delete SurfaceList;
        throw;
      }
    }

    inline Channel::Channel(Board *b, const char *Connector) :
    SurfaceList(NULL)
    {
      try {
        InitChannel(b);
        SetParam(MC_Connector, Connector);
      } catch (...) {
        if (Handle != 0)
          McDelete(Handle);
        delete SurfaceList;
        throw;
      }
    }

    inline void Channel::InitChannel(Board *b)
    {
      WRAPPING_MEMBERS_INIT

      MCSTATUS status;
      MCHANDLE handle;
      int boardIx;

      status = McCreate(MC_CHANNEL, &handle);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_CREATE, "Channel");
      Handle = handle;

      b->GetParam(MC_DriverIndex, boardIx);
      SetParam(MC_DriverIndex, boardIx);

      SurfaceList = new Internal::List<Surface *, 16>();
    }

    // Destructor
    inline Channel::~Channel()
    {
      MCSTATUS status;
      int i;

      if (Handle != 0)
      {
        status = McDelete(Handle);
        if (status != MC_OK)
          ThrowMultiCamException(status, TYPE_DELETE, "Channel");
      }

      for (i = 0 ; i < SurfaceList->GetCount() ; i++)
      {
        if (SurfaceList->At(i) != NULL)
          delete SurfaceList->At(i);
      }
      delete SurfaceList;

      WRAPPING_MEMBERS_UNINIT
    }

    // Channel activation
    inline void Channel::SetActive()
    {
      SetParam(MC_ChannelState, MC_ChannelState_ACTIVE);
    }

    // Channel deactivation
    inline void Channel::SetIdle()
    {
      int state;

      GetParam(MC_ChannelState, state);
      if (state == MC_ChannelState_ACTIVE)
        SetParam(MC_ChannelState, MC_ChannelState_IDLE);
    }

    // Channel preparation
    inline void Channel::Prepare()
    {
      int state;

      GetParam(MC_ChannelState, state);
      if (state == MC_ChannelState_ORPHAN)
        SetParam(MC_ChannelState, MC_ChannelState_IDLE);
      else if (state != MC_ChannelState_ACTIVE)
        SetParam(MC_ChannelState, MC_ChannelState_READY);
    }

    // Get Surface from handle
    inline Surface *Channel::GetSurface(MCHANDLE sHandle)
    {
      MCSTATUS status;
      void *address;
      Surface *surface;

      if ((sHandle & 0xF0000000) != (MC_DEFAULT_SURFACE_HANDLE & 0xF0000000))
        ThrowMultiCamException(MC_INVALID_VALUE);
      status = McGetParamPtr(sHandle, MC_sctxt, &address);
      if (status != MC_OK)
      {
        Euresys::MultiCam::Exception e(status, "µC++ Internal Error");
        throw e;
      }

      if (address != NULL)
        surface = reinterpret_cast<Surface *>(address);
      else
      {
        surface = new Surface(sHandle);
        SurfaceList->Add(surface);
      }

      return surface;
    }

  }
}
#endif
