// MCpp_MulticamObject.h - MULTICAM C++ API - MulticamObject
#if !defined(__MCPP_MULTICAMOBJECT_H__)
#define __MCPP_MULTICAMOBJECT_H__

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // MultiCamObject: base class for Configuration, Board, Surface and Channel classes
    // --------------------------------------------------------------------------------
    struct Exception;
    class Surface;
    class MultiCamObject
    {
    protected:
      MCHANDLE Handle;
      virtual Surface *GetSurface(MCHANDLE handle);
    public:
      MultiCamObject();
      virtual ~MultiCamObject() {}

    public:
      void SetParam(MCPARAMID param, int value);
      void SetParam(MCPARAMID param, unsigned int value);
      void SetParam(MCPARAMID param, INT64 value);
      void SetParam(MCPARAMID param, const char *value);
      void SetParam(MCPARAMID param, double value);
      void SetParam(MCPARAMID param, Surface &value);
      void SetParam(MCPARAMID param, void *value);
      void SetParam(const char *param, int value);
      void SetParam(const char *param, unsigned int value);
      void SetParam(const char *param, INT64 value);
      void SetParam(const char *param, const char *value);
      void SetParam(const char *param, double value);
      void SetParam(const char *param, Surface &value);
      void SetParam(const char *param, void *value);

      void GetParam(MCPARAMID param, int &value);
      void GetParam(MCPARAMID param, unsigned int &value);
      void GetParam(MCPARAMID param, INT64 &value);
      void GetParam(MCPARAMID param, char *value, int maxLength);
      void GetParam(MCPARAMID param, double &value);
      void GetParam(MCPARAMID param, Surface *&value);
      void GetParam(MCPARAMID param, void *&value);
      void GetParam(const char *param, int &value);
      void GetParam(const char *param, unsigned int &value);
      void GetParam(const char *param, INT64 &value);
      void GetParam(const char *param, char *value, int maxLength);
      void GetParam(const char *param, double &value);
      void GetParam(const char *param, Surface *&value);
      void GetParam(const char *param, void *&value);

      MCHANDLE GetHandle();
    };
  }
}

#endif
