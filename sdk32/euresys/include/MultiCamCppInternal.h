// MultiCamCppInternal.H - MULTICAM C++ API
#if !defined(__MULTICAMCPPINTERNAL_H__)
#define __MULTICAMCPPINTERNAL_H__

#include "MCpp_MultiCamObject.h"
#include "MCpp_MultiCamObjectWithSignaling.h"
#include "MCpp_Board.h"
#include "MCpp_BoardList.h"
#include "MCpp_Surface.h"
#include "MCpp_SignalInfo.h"
#include "MCpp_Callback.h"
#include "MCpp_Channel.h"
#include "MCpp_Configuration.h"
#include "MCpp_Exception.h"
#include "MCpp_global.h"

namespace Euresys
{
  namespace MultiCam
  {
    static Euresys::MultiCam::Configuration Config;
    static Euresys::MultiCam::BoardList Boards;
  }
}

#include "MCpp_MultiCamObjectImpl.h"
#include "MCpp_MultiCamObjectWithSignalingImpl.h"
#include "MCpp_BoardImpl.h"
#include "MCpp_BoardListImpl.h"
#include "MCpp_SurfaceImpl.h"
#include "MCpp_SignalInfoImpl.h"
#include "MCpp_CallbackImpl.h"
#include "MCpp_ChannelImpl.h"
#include "MCpp_ConfigurationImpl.h"
#include "MCpp_ExceptionImpl.h"
#include "MCpp_globalImpl.h"

#endif
