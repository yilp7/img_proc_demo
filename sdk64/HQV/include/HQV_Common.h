#ifndef __HQV_COMMON_H__
#define __HQV_COMMON_H__

#define BUF_SIZE    256
#define MAX_ENUM	64

#include "HQV_Param.h"

typedef enum HQV_DATATYPE
{
	DATA_RAW   = 0,
	DATA_RGB   = 1,
	DATA_GRAY  = 2,
};

typedef enum HQV_FILETYPE
{
    FILE_BMP  = 0,
    FILE_RAW  = 1,
    FILE_JPG  = 2,
    FILE_TIF  = 3,
};

typedef enum HQV_DEVICETYPE
{
	DEVICE_UDEF  = 0,
	DEVICE_GEV   = 1,
	DEVICE_USS   = 2,
	DEVICE_USB   = 3,
	DEVICE_U3V   = 4,
	DEVICE_GRB   = 5,
	DEVICE_NCM   = 6,
};

typedef enum HQV_DEVICEACCESSMODE
{
    DEVICE_INDEX  = 0, //Device Access Identify by Index    
    DEVICE_MAC    = 1, //Device Access Identify by MAC address    
    DEVICE_SN     = 2  //Device Access Identify by Serial Number    
};

typedef enum HQV_COMPORT
{
	PORT_SERIAL0 = 0,
	PORT_SERIAL1 = 1,
	PORT_BULK0   = 2,
	PORT_BULK1   = 3,
	PORT_BULK2   = 4,
};

typedef enum HQV_RESULT
{
	RESULT_OK                       = 0x0000,
	RESULT_ABORTED                  = 0x1001,
	RESULT_NOT_INITIALIZED          = 0x1002,
    RESULT_INVALID_ARGUMENT         = 0x1003,
	RESULT_DEVICE_NO_OPEN			= 0x1004,
	RESULT_DEVICE_OPENED			= 0x1005,
	RESULT_DEVICE_ERROR				= 0x1006,
	RESULT_NOT_CONNECTED			= 0x1007,
	RESULT_UNEXPECTED_ERROR         = 0x1008,
	RESULT_DEVICE_LOST				= 0x1009,
	RESULT_BUFFER_TOO_SMALL			= 0x100A,
	RESULT_OTHER_ERROR				= 0x100B,
	RESULT_FILE_ERROR				= 0x100C,
	RESULT_NOT_SUPPORTED			= 0x100D,
	RESULT_INVALID_PARAMETER        = 0x100E,
	RESULT_NOT_ENOUGH_MEMORY		= 0x100F,
	RESULT_FILE_SAVEFAILED			= 0x1010,
	RESULT_NOT_IMPLEMENETED			= 0x1012,
	RESULT_FRAME_CAPTUREFAILED		= 0x1013,
	RESULT_TIMEOUT					= 0x1014,

	RESULT_DEVICE_ISRUNNING			= 0x2101,
	RESULT_DEVICE_NORUNNING			= 0x2102,

};

typedef enum HQV_CODEC
{
    CODEC_MPEG4 = 13,
    CODEC_RAW   = 14,
    CODEC_H264  = 28,
    CODEC_ASV1  = 32,
};

typedef enum HQV_DEVSTATUS
{
	DEVSTATUS_DISCONNECT = 0,
	DEVSTATUS_CONNECT    = 1,
	DEVSTATUS_RECONNECT  = 2,
};

typedef struct _HQV_FRAMEINFO_
{
	DWORD lFrameID;                       //Frame Buffer Index
	DWORD lBufSize;                       //Frame Buffer Image Size
	DWORD lWidth;                         //Frame Buffer Image Width
	DWORD lHeight;                        //Frame Buffer Image Height
	BYTE  lPixBits;                       //Frame Buffer Pixel Depth
	PBYTE pBufPtr;                        //Frame Buffer Pointer
	DWORD lFrameStatus;					  //Frame Status
	GVSP_PIXEL lPixType;                  //Frame Buffer Pixel Type
	DWORD lTimeStamp;                     //Frame Time Stamp
}HQV_FRAMEINFO, *PHQV_FRAMEINFO;

typedef struct _HQV_DEVINFO_GEV_
{
	BYTE iDevGlobalIndex;					//Device Global Index (RO)
	CHAR cDevVendorName[BUF_SIZE];			//Device Vendor Name (RO)
	CHAR cDevName[BUF_SIZE];				//Device Factory Name (RO)
	CHAR cDevVersion[BUF_SIZE];				//Device Version (RO)
	CHAR cDevUID[BUF_SIZE];					//Device User Define Name
	CHAR cDevSN[BUF_SIZE];					//Device Serial Number (RO)
	CHAR cDevMAC[BUF_SIZE];					//Device MAC address (RO)
	CHAR cDevIP[BUF_SIZE];					//Device IP address
	CHAR cDevSubnetMask[BUF_SIZE];			//Device Subnet Mask
	CHAR cDevGateway[BUF_SIZE];				//Device Gateway
	CHAR cNicName[BUF_SIZE];				//Adapter Factory Name (RO)
	CHAR cNicMAC[BUF_SIZE];					//Adapter MAC address (RO)
	CHAR cNicIP[BUF_SIZE];					//Adapter IP address (RO)
	CHAR cNicSubnetMask[BUF_SIZE];			//Adapter NIC Subnet Mask (RO)
	CHAR cNicGateway[BUF_SIZE];				//Adapter Gateway (RO)
	LONG iTimeoutHeartbeat;					//HeartBeatTimeout
	LONG iTimeoutFirstPacket;				//First Packet Timeout
	LONG iTimeoutInterPacket;				//Inter Packet Timeout
	LONG iTimeoutImageRequest;				//Image Request Timeout
	LONG iTimeoutImageReceived;				//Image Received Timeout
	LONG iPacketSize;						//Network Packet Size
	BOOL bNicFilterDriver;					//If Adapter has Filter Driver (RO)
	BOOL bPacketResend;						//If Enable Packet Resend
	BOOL bLinkRecovery;						//If Enable Link Recovery
	BOOL bUserIDConfig;						//If Enable User ID Setting
	BOOL bUserIPConfig;						//If Enable User IP Setting
}HQV_DEVINFO_GEV, *PHQV_DEVINFO_GEV;

typedef struct _HQV_DEVINFO_USX_
{
	BYTE iDevGlobalIndex;					//Device Global Index (RO)
	CHAR cDevVendorName[BUF_SIZE];			//Device Vendor Name (RO)
	CHAR cDevName[BUF_SIZE];				//Device Factory Name (RO)
	CHAR cDevUID[BUF_SIZE];					//Device User Define Name
	WORD iDevVersion;						//Device Version (RO)
	WORD iDevPID;							//Device Product ID (RO)
	WORD iDevVID;							//Device Vendor ID (RO)
	CHAR cDevSN[20];						//Device Serial Number (RO)
	BYTE iDevChipIndex;						//Chip Manage Position (RO)
	LONG iTimeoutImageRequest;				//Image Request Timeout
	LONG iTimeoutImageReceived;				//Image Received Timeout
}HQV_DEVINFO_USS, HQV_DEVINFO_USB, *PHQV_DEVINFO_USS, *PHQV_DEVINFO_USB;

typedef struct _HQV_DEVINFO_U3V_
{
	BYTE iDevGlobalIndex;					//Device Global Index (RO)
	CHAR cDevVendorName[BUF_SIZE];			//Device Vendor Name (RO)
	CHAR cDevName[BUF_SIZE];				//Device Factory Name (RO)
	CHAR cDevVersion[BUF_SIZE];				//Device Version (RO)
	CHAR cDevUID[BUF_SIZE];					//Device User Define Name
	CHAR cDevSN[BUF_SIZE];					//Device Serial Number (RO)
	CHAR cDevGUID[BUF_SIZE];				//Device GUID address (RO)
	CHAR cHostName[BUF_SIZE];				//Host Factory Name (RO)
	WORD iHostPID;							//Host Product ID (RO)
	WORD iHostVID;							//Host Vendor ID (RO)
	LONG iTimeoutImageRequest;				//Image Request Timeout
	LONG iTimeoutImageReceived;				//Image Received Timeout
	BOOL bHostFilterDriver;					//If Host has Filter Driver (RO)
	BOOL bPacketResend;						//If Enable Packet Resend
	BOOL bLinkRecovery;						//If Enable Link Recovery
	BOOL bUserIDConfig;						//If Enable User ID Setting
}HQV_DEVINFO_U3V, *PHQV_DEVINFO_U3V;

typedef struct _HQV_DEVINFO_GRB_
{
	BYTE iDevGlobalIndex;					//Device Global Index (RO)
	CHAR cDevVendorName[BUF_SIZE];			//Device Vendor Name (RO)
	CHAR cDevName[BUF_SIZE];				//Device Factory Name (RO)
}HQV_DEVINFO_GRB, *PHQV_DEVINFO_GRB;

typedef struct _HQV_DEVINFO_NCM_
{
	BYTE iDevGlobalIndex;					//Device Global Index (RO)
	CHAR cDevVendorName[BUF_SIZE];			//Device Vendor Name (RO)
	CHAR cDevName[BUF_SIZE];				//Device Factory Name (RO)
}HQV_DEVINFO_NCM, *PHQV_DEVINFO_NCM;

typedef struct _HQV_DEVINFO_
{
	BOOL bDevOnline;                       //Device Link Status
	BOOL bDevOpen;                         //Device Open Status
	HQV_DEVICETYPE eDevType;               //Device Type    
	union
	{
		PHQV_DEVINFO_GEV pDevInfoGEV;      //DeviceInfoGEV Pointer
		PHQV_DEVINFO_USS pDevInfoUSS;      //DeviceInfoUSS Pointer
		PHQV_DEVINFO_USB pDevInfoUSB;      //DeviceInfoUSB Pointer
		PHQV_DEVINFO_U3V pDevInfoU3V;      //DeviceInfoU3V Pointer
		PHQV_DEVINFO_GRB pDevInfoGRB;      //DeviceInfoGRB Pointer
		PHQV_DEVINFO_NCM pDevInfoNCM;      //DeviceInfoNCM Pointer
	};
}HQV_DEVINFO, *PHQV_DEVINFO;

typedef DWORD (WINAPI *FRAME_CALLBACK)(HANDLE hDev, HQV_FRAMEINFO lFrameInfo, PVOID pContex);
typedef DWORD (WINAPI *INFO_CALLBACK)(HANDLE hDev, HQV_DEVSTATUS lDevStatus, PVOID pContex);

#endif

