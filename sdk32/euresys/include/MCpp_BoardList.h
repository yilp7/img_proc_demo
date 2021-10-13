// MCpp_Board.h - MULTICAM C++ API - BoardList
#if !defined(__MCPP_BOARDLIST_H__)
#define __MCPP_BOARDLIST_H__

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // BoardList class
    // ---------------
    class BoardListImpl
    {
    private:
      Board **List;
      int Count;

    public:
      BoardListImpl();
      ~BoardListImpl();
      void Init();

      Board *operator[] (int driverIndex) { return GetBoardByDriverIndex(driverIndex); }
      Board *GetBoardByDriverIndex(int driverIndex);
      Board *GetBoardByPciPosition(int pciPosition);
      Board *GetBoardByBoardName(const char *boardName);
      Board *GetBoardByBoardIdentifier(const char *boardIdentifier);
      int GetCount();
      void Clear();
    };

    inline BoardListImpl* GetBoardListImpl()
    {
      static BoardListImpl impl;
      return &impl;
    }

    class BoardList
    {
    public:
      void Init() { GetBoardListImpl()->Init(); }
      Board *operator[] (int driverIndex) { return GetBoardListImpl()->GetBoardByDriverIndex(driverIndex); }
      Board *GetBoardByDriverIndex(int driverIndex) { return GetBoardListImpl()->GetBoardByDriverIndex(driverIndex); }
      Board *GetBoardByPciPosition(int pciPosition) { return GetBoardListImpl()->GetBoardByPciPosition(pciPosition); }
      Board *GetBoardByBoardName(const char *boardName) { return GetBoardListImpl()->GetBoardByBoardName(boardName); }
      Board *GetBoardByBoardIdentifier(const char *boardIdentifier) { return GetBoardListImpl()->GetBoardByBoardIdentifier(boardIdentifier); }
      int GetCount() { return GetBoardListImpl()->GetCount(); }
      void Clear() { GetBoardListImpl()->Clear(); }
    };
  }
}

#endif
