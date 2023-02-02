// MCpp_MulticamObjectWithSignalingImpl.h - MULTICAM C++ API - MulticamObjectWithSignalingImpl
#if !defined(__MCPP_MULTICAMOBJECTWITHSIGNALINGIMPL_H__)
#define __MCPP_MULTICAMOBJECTWITHSIGNALINGIMPL_H__

#include "MCpp_MultiCamObjectWithSignaling.h"
#include "containers.h"
#include "CCallbacks.h"

namespace Euresys
{
  namespace MultiCam
  {
    inline MultiCamObjectWithSignaling::MultiCamObjectWithSignaling() : CbReg(false)
    {
      Callbacks = new Euresys::MultiCam::Internal::AssociativeArray < Callback* > ();
    }

    inline MultiCamObjectWithSignaling::~MultiCamObjectWithSignaling()
    {
      if (Callbacks != NULL)
      {
        Callbacks->DeleteAll();
        delete Callbacks;
      }
    }

    // Callback dispatcher routine
    inline void MultiCamObjectWithSignaling::CbRoutine(PMCSIGNALINFO mcInfo)
    {
      SignalInfo info;
      Callback *cb = NULL;

      GetSignalInfo(mcInfo, &info);

      if (info.Signal <= 0 )
        return;

      cb = Callbacks->At(info.Signal);
      if (cb == NULL)
        cb = Callbacks->At(MC_SIG_ANY);

      if (cb != NULL)
        cb->RunUntyped(this, info);
    }

    // WaitSignal
    inline void MultiCamObjectWithSignaling::WaitForSignal(MCSIGNAL Signal, unsigned int Timeout, SignalInfo &Info)
    {
      MCSTATUS status;
      MCSIGNALINFO mcInfo;

      status = McWaitSignal(Handle, Signal, Timeout, &mcInfo);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_WAIT_SIGNAL);
      GetSignalInfo(&mcInfo, &Info);
    }

    // GetSignalInfo
    inline void MultiCamObjectWithSignaling::GetSignalInfo(MCSIGNAL Signal, SignalInfo &Info)
    {
      MCSTATUS status;
      MCSIGNALINFO mcInfo;

      status = McGetSignalInfo(Handle, Signal, &mcInfo);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_GET_SIGNALINFO);
      GetSignalInfo(&mcInfo, &Info);
    }

    // Convert MCSIGNALINFO to Euresys.MultiCam.SignalInfo
    inline void MultiCamObjectWithSignaling::GetSignalInfo(PMCSIGNALINFO mcInfo, SignalInfo *Info)
    {
      Info->Signal = mcInfo->Signal;

      if (Info->Signal == MC_SIG_SURFACE_FILLED ||
        Info->Signal == MC_SIG_SURFACE_PROCESSING)
        Info->Surf = GetSurface(mcInfo->SignalInfo);
      else
        Info->Surf = NULL;
    }

    inline void MultiCamObjectWithSignaling::RegisterCallbackInternal(Callback *cb, MCSIGNAL Signal)
    {
      if (Signal < 0)
        ThrowMultiCamException(MC_OUT_OF_BOUND, TYPE_REGISTER_CB);

      // Register the user callback
      if (Callbacks->At(Signal) != NULL)
        delete Callbacks->At(Signal);
      Callbacks->Assign(Signal, cb);

      // Register the global callback function (if necessary)
      if (!CbReg)
      {
        MCSTATUS status = McRegisterCallback(Handle, GlobalCallbackFunction, this);
        if (status != MC_OK)
          ThrowMultiCamException(status, TYPE_REGISTER_CB);
        CbReg = true;
      }
    }

    inline void MultiCamObjectWithSignaling::RegisterRawCallback(void *CbFunction, void *CbContext, int Signal)
    {
      if (CbFunction == NULL)
      {
        Euresys::MultiCam::Exception e(MC_INTERNAL_ERROR, "Invalid callback function");
        throw e;
      }

      Callback *cb = new Euresys::MultiCam::CFunctionCallback(CbContext, (PMCPP_C_CALLBACK)CbFunction);
      RegisterCallbackInternal(cb, Signal);
    }

    inline void MultiCamObjectWithSignaling::UnregisterCallback(MCSIGNAL Signal)
    {
      if (Signal < 0)
        ThrowMultiCamException(MC_OUT_OF_BOUND, TYPE_UNREGISTER_CB);

      // Unregister the user callback
      Callback *cb = Callbacks->At(Signal);
      Callbacks->Assign(Signal, NULL);
      delete cb;

      // Unregister the global callback function (if necessary)
      if (CbReg)
      {
        int i;
        int count = Callbacks->GetCount();
        for (i = 0 ; i <= count && Callbacks->At(i) == NULL ; i++);
        if (i > Callbacks->GetCount())
        {
          MCSTATUS status = McRegisterCallback(Handle, NULL, NULL);
          if (status != MC_OK)
            ThrowMultiCamException(status, TYPE_UNREGISTER_CB);
          CbReg = false;
        }
      }
    }
  }
}

#endif
