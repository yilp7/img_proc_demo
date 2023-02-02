// MCpp_ConfigurationImpl.h - MULTICAM C++ API - ConfigurationImpl
#if !defined(__MCPP_CONFIGURATIONIMPL_H__)
#define __MCPP_CONFIGURATIONIMPL_H__

#include "MCpp_Configuration.h"

namespace Euresys
{
  namespace MultiCam
  {
    inline ConfigurationImpl::ConfigurationImpl()
    {
      Handle = MC_CONFIGURATION;
    }

    inline ConfigurationImpl::~ConfigurationImpl()
    {
    }

    inline ConfigurationImpl* GetConfigurationImpl()
    {
      static ConfigurationImpl impl;
      return &impl;
    }
  }
}

#endif
