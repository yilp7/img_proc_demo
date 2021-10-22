// MCpp_Surface.h - MULTICAM C++ API - Surface
#if !defined(__MCPP_SURFACE_H__)
#define __MCPP_SURFACE_H__

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // Surface class
    // -------------

    class Surface : public MultiCamObject
    {
      WRAPPING_MEMBERS

    protected:
      bool UserSurface;
    public:
      Surface(MCHANDLE aHandle);

    public:
      Surface();
      inline ~Surface();
      operator MCHANDLE() { return Handle; }

      void Reserve();
      void Free();
    };
  }
}

#endif
