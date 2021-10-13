//
// Euresys DirectShow input filter
//
// public definitions & interfaces
//

#pragma once

#include "multicam.h"

typedef enum {
	ESF_STANDARD_PAL = 0,
	ESF_STANDARD_NTSC
} ESF_VIDEO_STANDARD;

typedef enum {
  ESF_RESOLUTION_FRAME = 0,
  ESF_RESOLUTION_FIELD,
  ESF_RESOLUTION_CIF,
  ESF_RESOLUTION_QCIF,
  ESF_RESOLUTION_CUSTOM
} ESF_RESOLUTION;

typedef enum {
  ESF_BITRATECONTROL_CBR = 0,
  ESF_BITRATECONTROL_VBR
} ESF_BITRATECONTROL;

typedef enum {
  ESF_GOPSTRUCTURE_IONLY = 0,
  ESF_GOPSTRUCTURE_IPONLY,
  ESF_GOPSTRUCTURE_IPB
} ESF_GOPSTRUCTURE;
// {C8D2D5D0-02DF-41d4-BD80-7681CD059B52}
DEFINE_GUID(CLSID_ESFilter, 
0xc8d2d5d0, 0x2df, 0x41d4, 0xbd, 0x80, 0x76, 0x81, 0xcd, 0x5, 0x9b, 0x52);

//
// IESFProperties Interface
//
// {EC67FA27-4176-4659-ADCF-944D0941667F}
DEFINE_GUID(IID_IESFProperties, 
0xec67fa27, 0x4176, 0x4659, 0xad, 0xcf, 0x94, 0x4d, 0x9, 0x41, 0x66, 0x7f);

interface IESFProperties : public IUnknown
{
  // Capture rate control
  STDMETHOD(GetCaptureRateRange)(double *plMinValue, double *plMaxValue, double *plDefaultValue = NULL) = 0;
  STDMETHOD(SetCaptureRate)(double lValue) = 0;
  STDMETHOD(GetCaptureRate)(double *plValue) = 0;
  // Resolution selection
  STDMETHOD(SetResolution)(ESF_RESOLUTION Resolution, long lWidth = 0, long lHeight = 0) = 0; // Width and height are ignored if Resolution is not ESF_RESOLUTION_CUSTOM.
  STDMETHOD(GetResolution)(ESF_RESOLUTION *pResolution, long *plWidth = NULL, long *plHeight = NULL) = 0;
  STDMETHOD(GetCustomResolutionRange)(long *plMinWidth, long *plMaxWidth, long *plMinHeight, long *plMaxHeight) = 0;
  // Brightness control
  STDMETHOD(GetBrightnessRange)(long *plMinValue, long *plMaxValue, long *plDefaultValue = NULL) = 0;
  STDMETHOD(SetBrightness)(long lValue) = 0;
  STDMETHOD(GetBrightness)(long *plValue) = 0;
  // Contrast control
  STDMETHOD(GetContrastRange)(long *plMinValue, long *plMaxValue, long *plDefaultValue = NULL) = 0;
  STDMETHOD(SetContrast)(long lValue) = 0;
  STDMETHOD(GetContrast)(long *plValue) = 0;
  // Saturation control
  STDMETHOD(GetSaturationRange)(long *plMinValue, long *plMaxValue, long *plDefaultValue = NULL) = 0;
  STDMETHOD(SetSaturation)(long lValue) = 0;
  STDMETHOD(GetSaturation)(long *plValue) = 0;
  // Camera specification
  STDMETHOD(GetVideoStandard)(ESF_VIDEO_STANDARD *pStandard, bool *pbMonochrome) = 0;
  STDMETHOD(SetVideoStandard)(ESF_VIDEO_STANDARD Standard, bool bMonochrome) = 0;
  // Source selection
  STDMETHOD(GetBoardRange)(long *plMinValue, long *plMaxValue) = 0;
  STDMETHOD(SetBoard)(long lBoardId) = 0;
  STDMETHOD(GetBoard)(long *plBoardId) = 0;
  STDMETHOD(GetConnectorRange)(long *plMinValue, long *plMaxValue) = 0;
  STDMETHOD(SetConnector)(long lConnectorId) = 0;
  STDMETHOD(GetConnector)(long *plConnectorId) = 0;
  // Capture Region selection
  STDMETHOD(GetCaptureRegionSizeRange)(long *plMinSizeX, long *plMaxSizeX, long *plMinSizeY, long *plMaxSizeY) = 0;
  STDMETHOD(SetCaptureRegionSize)(long lSizeX, long lSizeY) = 0;
  STDMETHOD(GetCaptureRegionSize)(long *plSizeX, long *plSizeY) = 0;
  STDMETHOD(GetCaptureRegionPosRange)(long *plMinPositionX, long *plMaxPositionX, long *plMinPositionY, long *plMaxPositionY) = 0;
  STDMETHOD(SetCaptureRegionPos)(long lPositionX, long lPositionY) = 0;
  STDMETHOD(GetCaptureRegionPos)(long *plPositionX, long *plPositionY) = 0;
  // Output Format selection
  STDMETHOD(GetOutputFormatsList)(ULONG *pcElems, GUID **ppElems) = 0;
  STDMETHOD(SetOutputFormat)(GUID Format) = 0;
  STDMETHOD(GetOutputFormat)(GUID *pFormat) = 0;
  // Individual Control
  STDMETHOD(SetIndividualControl)(bool bActive) = 0;
  STDMETHOD(GetIndividualControl)(bool *pbActive) = 0;
  // Video Signal detection
  STDMETHOD(IsVideoSignalPresent)(void) = 0;
  // Camera specification with broadcast support
  STDMETHOD(GetVideoStandardEx)(ESF_VIDEO_STANDARD *pStandard, bool *pbMonochrome, bool *pbBroadcast) = 0;
  STDMETHOD(SetVideoStandardEx)(ESF_VIDEO_STANDARD Standard, bool bMonochrome, bool bBroadcast) = 0;
};

//
// IESFCompression Interface
//
// {A41D0CDA-FE6D-46b7-BBF9-3FF677CDABB7}
DEFINE_GUID(IID_IESFCompression, 
0xa41d0cda, 0xfe6d, 0x46b7, 0xbb, 0xf9, 0x3f, 0xf6, 0x77, 0xcd, 0xab, 0xb7);

interface IESFCompression : public IUnknown
{
  // Bitrate Control
  STDMETHOD(SetBitrateControl)(ESF_BITRATECONTROL BitrateControl) = 0;
  STDMETHOD(GetBitrateControl)(ESF_BITRATECONTROL *pBitrateControl) = 0;
  // Average Bit Rate
  STDMETHOD(GetAverageBitrateRange)(long *plMinValue, long *plMaxValue, long *plDefaultValue = NULL) = 0;
  STDMETHOD(SetAverageBitrate)(long lValue) = 0;
  STDMETHOD(GetAverageBitrate)(long *plValue) = 0;
  // Video Quality 
  STDMETHOD(GetVideoQualityRange)(long *plMinValue, long *plMaxValue, long *plDefaultValue = NULL) = 0;
  STDMETHOD(SetVideoQuality)(long lValue) = 0;
  STDMETHOD(GetVideoQuality)(long *plValue) = 0;
  // Gop structure
  STDMETHOD(SetGopStructure)(ESF_GOPSTRUCTURE GopStructure) = 0;
  STDMETHOD(GetGopStructure)(ESF_GOPSTRUCTURE *pGopStructure) = 0;
  STDMETHOD(GetGopSizeRange)(long *plMinValue, long *plMaxValue, long *plDefaultValue = NULL) = 0;
  STDMETHOD(SetGopSize)(long lValue) = 0;
  STDMETHOD(GetGopSize)(long *plValue) = 0;
};

//
// IMCBord Interface
//
// {A97BB8BD-9ABE-44e0-9F5A-E055A6B6FA36}
DEFINE_GUID(IID_IMCBoard, 
0xa97bb8bd, 0x9abe, 0x44e0, 0x9f, 0x5a, 0xe0, 0x55, 0xa6, 0xb6, 0xfa, 0x36);

interface IMCBoard : public IUnknown
{
  STDMETHOD(GetParamFloat)(UINT32 unParamId, double *pdValue) = 0;
  STDMETHOD(GetParamInt)(UINT32 unParamId, long *plValue) = 0;
  STDMETHOD(GetParamStr)(UINT32 unParamId, PWSTR pszValue, long ccValue) = 0;
  STDMETHOD(GetParamNmFloat)(PCWSTR pszName, double *pdValue) = 0;
  STDMETHOD(GetParamNmInt)(PCWSTR pszName, long *plValue) = 0;
  STDMETHOD(GetParamNmStr)(PCWSTR pszName, PWSTR pszValue, long ccValue) = 0;
  STDMETHOD(SetParamFloat)(UINT32 unParamId, double dValue) = 0;
  STDMETHOD(SetParamInt)(UINT32 unParamId, long lValue) = 0;
  STDMETHOD(SetParamStr)(UINT32 unParamId, PCWSTR pszValue) = 0;
  STDMETHOD(SetParamNmFloat)(PCWSTR pszName, double dValue) = 0;
  STDMETHOD(SetParamNmInt)(PCWSTR pszName, long lValue) = 0;
  STDMETHOD(SetParamNmStr)(PCWSTR pszName, PCWSTR pszValue) = 0;
};

//
// IMCEnumBords Interface
//
// {C1A9226F-D740-439a-A8CC-FFB46977FBC3}
DEFINE_GUID(IID_IMCEnumBoards, 
0xc1a9226f, 0xd740, 0x439a, 0xa8, 0xcc, 0xff, 0xb4, 0x69, 0x77, 0xfb, 0xc3);

interface IMCEnumBoards : public IUnknown
{
	STDMETHOD(Next)(ULONG celt, IMCBoard **ppEureCard, ULONG *pceltFetched) = 0;
	STDMETHOD(Skip)(ULONG celt) = 0;
	STDMETHOD(Reset)() = 0;
	STDMETHOD(Clone)(IMCEnumBoards **ppEnumBoards) = 0;
};

//
// IMCConfig Interface
//
// {7377EB18-82DE-43ff-8DB2-01FEA57EEA66}
DEFINE_GUID(IID_IMCConfig, 
0x7377eb18, 0x82de, 0x43ff, 0x8d, 0xb2, 0x1, 0xfe, 0xa5, 0x7e, 0xea, 0x66);

interface IMCConfig : public IUnknown
{
  STDMETHOD(GetBoardEnum)(IMCEnumBoards **ppEnumBoards) = 0;
};


//
// IESFAllocatorNegociation
//
// {3475FA59-2661-4d91-BA60-9FE92E180395}
DEFINE_GUID(IID_IESFAllocatorNegociation, 
0x3475fa59, 0x2661, 0x4d91, 0xba, 0x60, 0x9f, 0xe9, 0x2e, 0x18, 0x3, 0x95);

interface IESFAllocatorNegociation : public IUnknown
{
  STDMETHOD(UseInputPinAllocator)(BOOL bUseInputPinAllocator) = 0;
};

//
// Property Pages GUIDS
//

// Configuration Property page
// {CE39B2A2-DF66-4D8B-93CD-DC6A1EADF09A}
DEFINE_GUID(CLSID_ConfigPropPage, 
0xCE39B2A2, 0xDF66, 0x4D8B, 0x93, 0xCD, 0xDC, 0x6A, 0x1E, 0xAD, 0xF0, 0x9A);

// Adjustment Property page
// {E873C3B6-950F-45CD-A184-95428036FB6A}
DEFINE_GUID(CLSID_AdjustPropPage, 
0xE873C3B6, 0x950F, 0x45CD, 0xA1, 0x84, 0x95, 0x42, 0x80, 0x36, 0xFB, 0x6A);

// Compression Property page
// {15CF19B6-1F28-4B06-A3D8-FC58B7A108C5}
DEFINE_GUID(CLSID_CompressionPropPage, 
0x15CF19B6, 0x1F28, 0x4B06, 0xA3, 0xD8, 0xFC, 0x58, 0xB7, 0xA1, 0x08, 0xC5);

//
// Filter Events
//

#define EC_ESF_SIGNAL_LOST            EC_USER + 0x00
#define EC_ESF_SIGNAL_RESTORED        EC_USER + 0x01

//
// Error Codes
//
#define MCSTATUS_TO_HRESULT(status) \
  MAKE_HRESULT((status == MC_OK)? SEVERITY_SUCCESS : SEVERITY_ERROR, FACILITY_ITF, (0x4000 - status))
#define HRESULT_TO_MCSTATUS(hr) (MCSTATUS)(0x4000 - HRESULT_CODE(hr))

#define E_ESF_BAD_PARAMETER   (MCSTATUS_TO_HRESULT(MC_BAD_PARAMETER))
#define E_ESF_OUT_OF_BOUND    (MCSTATUS_TO_HRESULT(MC_OUT_OF_BOUND))
#define E_ESF_INVALID_VALUE   (MCSTATUS_TO_HRESULT(MC_INVALID_VALUE))
#define E_ESF_FATAL_ERROR	  (MCSTATUS_TO_HRESULT(MC_FATAL_ERROR))
