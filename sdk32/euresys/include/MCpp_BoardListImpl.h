// MCpp_BoardImpl.h - MULTICAM C++ API - BoardListImpl
#if !defined(__MCPP_BOARDLISTIMPL_H__)
#define __MCPP_BOARDLISTIMPL_H__

#include <cstring>
#include "MCpp_BoardList.h"

namespace
{

int mc_stricmp(const char *s1, const char *s2)
{
#if defined(__GNUC__)
   return strcasecmp(s1, s2);
#elif defined(_MSC_VER) && _MSC_VER >= 1400
   // Visual Studio 8 or higher deprecate stricmp.
   return _stricmp(s1, s2);
#else
   return stricmp(s1, s2);
#endif
}
}

namespace Euresys
{
  namespace MultiCam
  {
//    BoardListImpl* GetBoardListImpl()

    inline BoardListImpl::BoardListImpl()
    {
      List = NULL;
      Count = 0;
    }

    inline BoardListImpl::~BoardListImpl()
    {
      Clear();
    }

    inline void BoardListImpl::Clear()
    {
      int i;

      if (List != NULL)
      {
        for (i = 0 ; i < Count ; i++)
        {
          if (List[i] != NULL)
            delete List[i];
        }
        delete[] List;
      }
      List = NULL;
      Count = 0;
    }

    // Initialization
    inline void BoardListImpl::Init()
    {
      int i;

      if (List != NULL) // Already initialized
        return;

      Config.GetParam(MC_BoardCount, Count);
      if (Count > 0)
      {
        List = new Board *[Count];
        for (i = 0 ; i < Count ; i++)
          List[i] = NULL;
        for (i = 0 ; i < Count ; i++)
          List[i] = new Board(i);
      }
    }

    // Accessors

    inline Board *BoardListImpl::GetBoardByDriverIndex(int DriverIndex)
    {
      if (DriverIndex >= 0 && DriverIndex < Count)
        return List[DriverIndex];
      else
      {
        ThrowMultiCamException(MC_NO_BOARD_FOUND);
        return NULL;
      }
    }

    inline Board *BoardListImpl::GetBoardByPciPosition(int PciPosition)
    {
      int i;
      int pos;

      for (i = 0 ; i < Count ; i++)
      {
        List[i]->GetParam(MC_PciPosition, pos);
        if (pos == PciPosition)
          return List[i];
      }

      ThrowMultiCamException(MC_NO_BOARD_FOUND);
      return NULL;
    }

    inline Board *BoardListImpl::GetBoardByBoardName(const char *BoardName)
    {
      int i;
      char name[64];

      for (i = 0 ; i < Count ; i++)
      {
        List[i]->GetParam(MC_BoardName, name, sizeof(name));
        if (mc_stricmp(name, BoardName) == 0)
          return List[i];
      }

      ThrowMultiCamException(MC_NO_BOARD_FOUND);
      return NULL;
    }

    inline Board *BoardListImpl::GetBoardByBoardIdentifier(const char *BoardIdentifier)
    {
      int i;
      char id[64];

      for (i = 0 ; i < Count ; i++)
      {
        List[i]->GetParam(MC_BoardIdentifier, id, sizeof(id));
        if (mc_stricmp(id, BoardIdentifier) == 0)
          return List[i];
      }

      ThrowMultiCamException(MC_NO_BOARD_FOUND);
      return NULL;
    }

    inline int BoardListImpl::GetCount()
    {
      return Count;
    }
  }
}

#endif
