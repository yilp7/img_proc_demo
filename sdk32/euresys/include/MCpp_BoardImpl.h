// MCpp_BoardImpl.h - MULTICAM C++ API - BoardImpl
#if !defined(__MCPP_BOARDIMPL_H__)
#define __MCPP_BOARDIMPL_H__

#include "MCpp_Board.h"

namespace Euresys
{
  namespace MultiCam
  {
    inline Board::Board(int Index)
    {
      WRAPPING_MEMBERS_INIT

      Handle = MC_BOARD + Index;

      // Make sure the handle is valid (i.e., this object corresponds to a MultiCam Board object)
      MCSTATUS status;
      int ix;
      status = McGetParamInt(Handle, MC_DriverIndex, &ix);
      if (status != MC_OK || ix != Index)
        ThrowMultiCamException(MC_NO_BOARD_FOUND);
    }

    inline Board::~Board()
    {
      WRAPPING_MEMBERS_UNINIT
    }

  }
}

#endif
