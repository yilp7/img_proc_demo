// MCpp_Configuration.h - MULTICAM C++ API - Configuration
#if !defined(__MCPP_CONFIGURATION_H__)
#define __MCPP_CONFIGURATION_H__

namespace Euresys
{
  namespace MultiCam
  {
    // ********************************************************************************************
    // Configuration class
    // -------------------
    class ConfigurationImpl : public MultiCamObject
    {
    public:
      ConfigurationImpl();
      ~ConfigurationImpl();
    };

    ConfigurationImpl* GetConfigurationImpl();

    class Configuration
    {
    public:
      void SetParam(MCPARAMID param, int value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(MCPARAMID param, unsigned int value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(MCPARAMID param, INT64 value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(MCPARAMID param, const char *value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(MCPARAMID param, double value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(MCPARAMID param, Surface &value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(MCPARAMID param, void *value) { GetConfigurationImpl()->SetParam(param, value); }

      void SetParam(const char *param, int value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(const char *param, unsigned int value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(const char *param, INT64 value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(const char *param, const char *value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(const char *param, double value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(const char *param, Surface &value) { GetConfigurationImpl()->SetParam(param, value); }
      void SetParam(const char *param, void *value) { GetConfigurationImpl()->SetParam(param, value); }

      void GetParam(MCPARAMID param, int &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(MCPARAMID param, unsigned int &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(MCPARAMID param, INT64 &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(MCPARAMID param, char *value, int maxLength) { GetConfigurationImpl()->GetParam(param, value, maxLength); }
      void GetParam(MCPARAMID param, double &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(MCPARAMID param, Surface *&value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(MCPARAMID param, void *&value) { GetConfigurationImpl()->GetParam(param, value); }

      void GetParam(const char *param, int &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(const char *param, unsigned int &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(const char *param, INT64 &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(const char *param, char *value, int maxLength) { GetConfigurationImpl()->GetParam(param, value, maxLength); }
      void GetParam(const char *param, double &value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(const char *param, Surface *&value) { GetConfigurationImpl()->GetParam(param, value); }
      void GetParam(const char *param, void *&value) { GetConfigurationImpl()->GetParam(param, value); }

      MCHANDLE GetHandle() { return GetConfigurationImpl()->GetHandle(); }
    };
  }
}

#endif
