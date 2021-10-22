// MCpp_Callback.h - MULTICAM C++ API - Callback
#if !defined(__MCPP_CALLBACK_H__)
#define __MCPP_CALLBACK_H__

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // Callback class
    // --------------
    class Callback
    {
    public:
      virtual ~Callback() = 0;
      virtual void RunUntyped(MultiCamObjectWithSignaling *object, SignalInfo &info) = 0;
    };

    template <class T, class OwnerType> class UserCallback : public Callback
    {
    private:
      T *Object;
      void (T::*Function)(OwnerType &channel, SignalInfo &info);

    public:
      UserCallback(T *obj, void (T::*f)(OwnerType &channel, SignalInfo &info)) :
          Object(obj),
            Function(f)
          {
          }
          inline ~UserCallback()
          {
          }

          void Run(OwnerType &channel, SignalInfo &info)
          {
            (Object->*Function)(channel, info);
          }
          void RunUntyped(MultiCamObjectWithSignaling *caller, SignalInfo &info)
          {
            Run(*reinterpret_cast<OwnerType*>(caller), info);
          }
    };
  }
}

#endif
