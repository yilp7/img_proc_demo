// CCALLBACKS.H - C CALLBACKS LAYER

#if !defined(__CCALLBACKS_H__)
#define __CCALLBACKS_H__

typedef void (MCAPI *PMCPP_C_CALLBACK)(void *Context, void *Caller, void *Info);

namespace Euresys
{
	namespace MultiCam
	{
		// ********************************************************************************************
		// Callback class (continued)
		// --------------------------

		class CFunctionCallback : public Callback
		{
		private:
			void *Context;
			PMCPP_C_CALLBACK Function;

		public:
			CFunctionCallback(void *aContext, PMCPP_C_CALLBACK aFunction) :
			Context(aContext),
			Function(aFunction)
			{
			}
			~CFunctionCallback()
			{
			}

			void Run(MultiCamObjectWithSignaling &caller, SignalInfo &info)
			{
				Function(Context, &caller, &info);
			}

            void RunUntyped(MultiCamObjectWithSignaling *caller, SignalInfo &info)
			{
				Function(Context, caller, &info);
			}
		};
	}
}


#endif
