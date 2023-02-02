// MCpp_Board.h - MULTICAM C++ API - Board
#if !defined(__MCPP_BOARD_H__)
#define __MCPP_BOARD_H__

// ********************************************************************************************
// Board class
// -----------
namespace Euresys
{
  namespace MultiCam
  {
    class Board : public MultiCamObjectWithSignaling
    {
      WRAPPING_MEMBERS

    public:
      Board(int index);
      ~Board();

      template <class T>
      void RegisterCallback(T *owner, void (T::*callbackMethod)(Board &, SignalInfo &), MCSIGNAL signal)
      {
        if (owner == NULL || callbackMethod == NULL)
        {
          ::Euresys::MultiCam::Exception e(MC_INTERNAL_ERROR, "Invalid callback object or function");
          throw e;
        }

        Callback *cb = new UserCallback<T, Board>(owner, callbackMethod);
        RegisterCallbackInternal(cb, signal);
      }
    };
  }
}
#endif
