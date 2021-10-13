// MultiCamCppInternal.H - MULTICAM C++ API
#if !defined(__MULTICAMCPPEXTERNAL_H__)
#define __MULTICAMCPPEXTERNAL_H__

// For wrapping
#if defined __MULTICAM_WRAPPING__
  #ifdef __GNUC__
    typedef void (*FinalizeCallback)(int handle);
  #else
    typedef void (__stdcall *FinalizeCallback)(int handle);
  #endif
  #define WRAPPING_MEMBERS public: \
    FinalizeCallback callback_; \
    int handle_; \

  #define WRAPPING_MEMBERS_INIT handle_ = 0; \
    callback_ = 0; \

#define WRAPPING_MEMBERS_UNINIT if ((handle_ != NULL) && (callback_ != NULL)) { \
    MultiCam_LockDestructorCriticalSection(); \
    if ((handle_ != NULL) && (callback_ != NULL)) callback_(handle_); \
    MultiCam_UnlockDestructorCriticalSection(); }\

#else
#define WRAPPING_MEMBERS
#define WRAPPING_MEMBERS_INIT
#define WRAPPING_MEMBERS_UNINIT
#endif

#endif
