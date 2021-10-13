// MCpp_Channel.h - MULTICAM C++ API - Channel
#if !defined(__MCPP_CHANNEL_H__)
#define __MCPP_CHANNEL_H__

namespace Euresys
{
  namespace MultiCam
  {

    // ********************************************************************************************
    // Channel class
    // -------------
    class Channel : public MultiCamObjectWithSignaling
    {
      WRAPPING_MEMBERS

    protected:
      virtual Surface *GetSurface(MCHANDLE Handle);
      Internal::Container<Surface*> *SurfaceList;
      void InitChannel(Board *board);

    public:
      Channel(Board *board, int connector);
      Channel(Board *board, const char *connector);
      inline ~Channel();
      operator MCHANDLE() { return Handle; }

      void SetActive();
      void SetIdle();
      void Prepare();

      template <class TT>
      void RegisterCallback(TT *owner, void (TT::*callbackMethod)(Channel &, SignalInfo &), MCSIGNAL signal)
      {
        if (owner == NULL || callbackMethod == NULL)
        {
          ::Euresys::MultiCam::Exception e(MC_INTERNAL_ERROR, "Invalid callback object or function");
          throw e;
        }


        Callback *cb = new UserCallback<TT, Channel>(owner, callbackMethod);
        RegisterCallbackInternal(cb, signal);
      }

      Surface *GetSurfaceByIndex(unsigned int index)
      {
        Surface *surf;
        GetParam(MC_Cluster + index, surf);
        return surf;
      }
    };
  }
}

#endif
