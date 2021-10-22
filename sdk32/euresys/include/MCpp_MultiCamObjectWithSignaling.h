// MCpp_MulticamObjectWithSignaling.h - MULTICAM C++ API - MulticamObjectWithSignaling
#if !defined(__MCPP_MULTICAMOBJECTWITHSIGNALING_H__)
#define __MCPP_MULTICAMOBJECTWITHSIGNALING_H__

namespace Euresys
{
  namespace MultiCam
  {

    namespace Internal 
    {
      template<typename T>
      class Container 
      {
      public:
        Container() {}
        virtual inline ~Container() {}
        virtual T At(int Index) const = 0;
        virtual int GetCount() const = 0;
        virtual int Add(T &item) = 0;
        virtual void Assign(int , T ) {}
        virtual void DeleteAll() {}
      };
    }

    class SignalInfo;
    class Callback;
    template <class T, class OwnerType> class UserCallback;

    class MultiCamObjectWithSignaling : public MultiCamObject
    {
    private:
      Internal::Container<Callback*> *Callbacks;
      bool CbReg;

      void GetSignalInfo(PMCSIGNALINFO mcInfo, SignalInfo *Info);

    public:
      void CbRoutine(PMCSIGNALINFO mcInfo);
      void RegisterCallbackInternal(Callback *cb, MCSIGNAL Signal);

      MultiCamObjectWithSignaling();
      virtual ~MultiCamObjectWithSignaling();

      void UnregisterCallback(MCSIGNAL signal);
      void RegisterRawCallback(void *, void *, int);

      void WaitSignal(MCSIGNAL signal, unsigned int timeout, SignalInfo &info) { WaitForSignal(signal, timeout, info); }
      void WaitForSignal(MCSIGNAL signal, unsigned int timeout, SignalInfo &info);
      void GetSignalInfo(MCSIGNAL signal, SignalInfo &info);

    };


  }
}

#endif
