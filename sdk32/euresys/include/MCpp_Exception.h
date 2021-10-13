// MCpp_Exception.h - MULTICAM C++ API - Exception
#if !defined(__MCPP_EXCEPTION_H__)
#define __MCPP_EXCEPTION_H__

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // Exceptions
    // ----------
#define MCPP_MAX_EXCEPTION_DESCRIPTION_SIZE	1024
    struct Exception
    {
    private:
      char McDescription[MCPP_MAX_EXCEPTION_DESCRIPTION_SIZE];

    public:
      Exception(int error, const char *desc);
      Exception(Exception &e);
      Exception& operator=(Exception &e);
      inline ~Exception();

      int Error;
      const char *What();
      MCSTATUS GetMcStatus() { return Error; }
    };

  }
}

#endif
