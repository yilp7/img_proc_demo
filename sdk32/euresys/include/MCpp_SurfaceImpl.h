// MCpp_SurfaceImpl.h - MULTICAM C++ API - SurfaceImpl
#if !defined(__MCPP_SURFACEIMPL_H__)
#define __MCPP_SURFACEIMPL_H__

#include "MCpp_Surface.h"

namespace Euresys
{
  namespace MultiCam
  {
    inline Surface::Surface()
    {
      WRAPPING_MEMBERS_INIT

      MCSTATUS status;
      MCHANDLE handle;

      status = McCreate(MC_DEFAULT_SURFACE_HANDLE, &handle);
      if (status != MC_OK)
        ThrowMultiCamException(status, TYPE_CREATE, "Surface");
      Handle = handle;
      UserSurface = true;

      // Link the MultiCam Surface to this object
      SetParam(MC_sctxt, this);
    }

    inline Surface::Surface(MCHANDLE aHandle)
    {
      WRAPPING_MEMBERS_INIT

      Handle = aHandle;
      UserSurface = false;

      // Link the MultiCam Surface to this object
      SetParam(MC_sctxt, this);
    }

    inline Surface::~Surface()
    {
      MCSTATUS status;

      if (Handle != 0)
      {
        if (UserSurface)
        {
          status = McDelete(Handle);
          if (status != MC_OK)
            ThrowMultiCamException(status, TYPE_DELETE, "Surface");
        }
        else
        {
          try {
            SetParam(MC_sctxt, (void *)NULL);
          }
          catch (Euresys::MultiCam::Exception &) // MultiCam may already have deleted the surface
          {
          }
        }
      }
      WRAPPING_MEMBERS_UNINIT
    }

    // Manual surface reservation
    inline void Surface::Reserve()
    {
      SetParam(MC_SurfaceState, MC_SurfaceState_RESERVED);
    }

    inline void Surface::Free()
    {
      SetParam(MC_SurfaceState, MC_SurfaceState_FREE);
    }
  }
}

#endif
