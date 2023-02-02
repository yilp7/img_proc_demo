// MCpp_SignalInfo.h - MULTICAM C++ API - SignalInfo
#if !defined(__MCPP_SIGNALINFO_H__)
#define __MCPP_SIGNALINFO_H__

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // SignalInfo class
    // ----------------
    class SignalInfo
    {
      WRAPPING_MEMBERS

    public:
      SignalInfo() 
      {
        WRAPPING_MEMBERS_INIT
      };
      ~SignalInfo();

    public:
      MCSIGNAL Signal;
      Surface *Surf;
    };
  }
}

#endif
