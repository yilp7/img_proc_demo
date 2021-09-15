#ifndef __HQV_PARAM_H__
#define __HQV_PARAM_H__

#define GVSP_PIXELMONO   (0x01000000)
#define GVSP_PIXELCOLOR  (0x02000000)
#define GVSP_PIXEL8BIT   (0x00080000)
#define GVSP_PIXEL12BIT  (0x000C0000)
#define GVSP_PIXEL16BIT  (0x00100000)
#define GVSP_PIXEL24BIT  (0x00180000)
#define GVSP_PIXEL32BIT  (0x00200000)
#define GVSP_PIXEL36BIT  (0x00240000)
#define GVSP_PIXEL48BIT  (0x00300000)
#define GVSP_PIXEL64BIT  (0x00400000)

#include <windows.h>

typedef enum GVSP_PIXEL
{
	GVSP_PIXEL_UDEF              = 0,
	GVSP_PIXEL_MONO8             = GVSP_PIXELMONO  | GVSP_PIXEL8BIT  | 0x0001,
	GVSP_PIXEL_MONO8SIGNED       = GVSP_PIXELMONO  | GVSP_PIXEL8BIT  | 0x0002,
	GVSP_PIXEL_MONO10            = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0003,
	GVSP_PIXEL_MONO10PACKED      = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x0004,
	GVSP_PIXEL_MONO12            = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0005,
	GVSP_PIXEL_MONO12PACKED      = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x0006,
	GVSP_PIXEL_MONO14            = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0025,
	GVSP_PIXEL_MONO16            = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0007,
	GVSP_PIXEL_BAYERGR8          = GVSP_PIXELMONO  | GVSP_PIXEL8BIT  | 0x0008,
	GVSP_PIXEL_BAYERRG8          = GVSP_PIXELMONO  | GVSP_PIXEL8BIT  | 0x0009,
	GVSP_PIXEL_BAYERGB8          = GVSP_PIXELMONO  | GVSP_PIXEL8BIT  | 0x000A,
	GVSP_PIXEL_BAYERBG8          = GVSP_PIXELMONO  | GVSP_PIXEL8BIT  | 0x000B,
	GVSP_PIXEL_BAYERGR10         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x000C,
	GVSP_PIXEL_BAYERRG10         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x000D,
	GVSP_PIXEL_BAYERGB10         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x000E,
	GVSP_PIXEL_BAYERBG10         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x000F,
	GVSP_PIXEL_BAYERGR12         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0010,
	GVSP_PIXEL_BAYERRG12         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0011,
	GVSP_PIXEL_BAYERGB12         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0012,
	GVSP_PIXEL_BAYERBG12         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0013,
	GVSP_PIXEL_BAYERGR10PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x0026,
	GVSP_PIXEL_BAYERRG10PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x0027,
	GVSP_PIXEL_BAYERGB10PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x0028,
	GVSP_PIXEL_BAYERBG10PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x0029,
	GVSP_PIXEL_BAYERGR12PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x002A,
	GVSP_PIXEL_BAYERRG12PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x002B,
	GVSP_PIXEL_BAYERGB12PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x002C,
	GVSP_PIXEL_BAYERBG12PACKED   = GVSP_PIXELMONO  | GVSP_PIXEL12BIT | 0x002D,
	GVSP_PIXEL_BAYERGR16         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x002E,
	GVSP_PIXEL_BAYERRG16         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x002F,
	GVSP_PIXEL_BAYERGB16         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0030,
	GVSP_PIXEL_BAYERBG16         = GVSP_PIXELMONO  | GVSP_PIXEL16BIT | 0x0031,
	GVSP_PIXEL_RGB8PACKED        = GVSP_PIXELCOLOR | GVSP_PIXEL24BIT | 0x0014,
	GVSP_PIXEL_BGR8PACKED        = GVSP_PIXELCOLOR | GVSP_PIXEL24BIT | 0x0015,
	GVSP_PIXEL_RGBA8PACKED       = GVSP_PIXELCOLOR | GVSP_PIXEL32BIT | 0x0016,
	GVSP_PIXEL_BGRA8PACKED       = GVSP_PIXELCOLOR | GVSP_PIXEL32BIT | 0x0017,
	GVSP_PIXEL_RGB10PACKED       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x0018,
	GVSP_PIXEL_BGR10PACKED       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x0019,
	GVSP_PIXEL_RGB12PACKED       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x001A,
	GVSP_PIXEL_BGR12PACKED       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x001B,
	GVSP_PIXEL_RGB16PACKED       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x0033,
	GVSP_PIXEL_BGR10V1PACKED     = GVSP_PIXELCOLOR | GVSP_PIXEL32BIT | 0x001C,
	GVSP_PIXEL_BGR10V2PACKED     = GVSP_PIXELCOLOR | GVSP_PIXEL32BIT | 0x001D,
	GVSP_PIXEL_RGB12V1PACKED     = GVSP_PIXELCOLOR | GVSP_PIXEL36BIT | 0x0034,
	GVSP_PIXEL_YUV411PACKED      = GVSP_PIXELCOLOR | GVSP_PIXEL12BIT | 0x001E,
	GVSP_PIXEL_YUV422PACKED      = GVSP_PIXELCOLOR | GVSP_PIXEL16BIT | 0x001F,
	GVSP_PIXEL_YUV422YUYVPACKED  = GVSP_PIXELCOLOR | GVSP_PIXEL16BIT | 0x0032,
	GVSP_PIXEL_YUV444PACKED      = GVSP_PIXELCOLOR | GVSP_PIXEL24BIT | 0x0020,
	GVSP_PIXEL_RGB8PLANAR        = GVSP_PIXELCOLOR | GVSP_PIXEL24BIT | 0x0021,
	GVSP_PIXEL_RGB10PLANAR       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x0022,
	GVSP_PIXEL_RGB12PLANAR       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x0023,
	GVSP_PIXEL_RGB16PLANAR       = GVSP_PIXELCOLOR | GVSP_PIXEL48BIT | 0x0024,
	GVSP_PIXEL_WINRGB16          = GVSP_PIXELCOLOR | GVSP_PIXEL16BIT | 0x8000,
	GVSP_PIXEL_WINRGB32          = GVSP_PIXELCOLOR | GVSP_PIXEL32BIT | 0x8001,
	GVSP_PIXEL_WINRGB24          = GVSP_PIXELCOLOR | GVSP_PIXEL24BIT | 0x8002,
	GVSP_PIXEL_WINBGR32          = GVSP_PIXELCOLOR | GVSP_PIXEL32BIT | 0x8003,
	GVSP_PIXEL_WINBGR24          = GVSP_PIXELCOLOR | GVSP_PIXEL24BIT | 0x8004
};


typedef enum HQV_DEVPARAM_ID
{
	//Common Param SFNC NO INCLUDE (Based On GenICam_SFNC_2_3_0) 
	PARAM_ID_UNDEFINE                                   = 0x00000000,

	PARAM_ID_COMMON_LOGENABLE                           = 0x00000001, //IBoolean R/W - B Log File Enable (0:Off 1:On)
	PARAM_ID_COMMON_TRACEENABLE                         = 0x00000002, //IBoolean R/W - B Trace Output Enable (0:Off 1:On)
	PARAM_ID_COMMON_CONFIGENABLE                        = 0x00000003, //IBoolean R/W - B Config File Enable (0:Off 1:On)
	PARAM_ID_COMMON_DEVICETYPE							= 0x00000004, //IInteger R - B Device Type(HQV_DEVICETYPE)
	PARAM_ID_COMMON_DEVICEINDEX							= 0x00000005, //IInteger R - B Device Index

	PARAM_ID_COMMON_SHOWDEVICECONTROLWINDOW             = 0x00000010, //[C][WO] Show Parameter Window
	PARAM_ID_COMMON_SHOWDEVICESERIALWINDOW              = 0x00000011, //[C][WO] Show Serial Window

	//Bayer Filter(Set the filter's values. The Gain* and Offset methods let you control the transformation(x' = kx + y) made by the filter, for the colors red, green, and blue.)
	PARAM_ID_COMMON_BAYERFILTERENABLE                   = 0x00000100,		//IBoolean R/W - B Bayer filtering for this converter enable (0:Off 1:On)
	PARAM_ID_COMMON_BAYERFILTERTYPE		                = 0x00000101,		//IBoolean R/W - B Bayer filtering for this converter 3x3 (0: 2x2 nearest neighbor 1: 3x3 kernel considered )
	PARAM_ID_COMMON_BAYERFILTERGAING                    = 0x00000102,		//IFloat R/W - B Set the Bayer filter's gain (green) 
	PARAM_ID_COMMON_BAYERFILTERGAINR                    = 0x00000103,		//IFloat R/W - B Set the Bayer filter's gain (red) 
	PARAM_ID_COMMON_BAYERFILTERGAINB                    = 0x00000104,		//IFloat R/W - B Set the Bayer filter's gain (blue) 
	PARAM_ID_COMMON_BAYERFILTEROFFSETG                  = 0x00000105,		//IInteger R/W - B Set the Bayer filter's offset (green) 
	PARAM_ID_COMMON_BAYERFILTEROFFSETR                  = 0x00000106,		//IInteger R/W - B Set the Bayer filter's offset (red) 
	PARAM_ID_COMMON_BAYERFILTEROFFSETB                  = 0x00000107,		//IInteger R/W - B Set the Bayer filter's offset (blue) 
	PARAM_ID_COMMON_BAYERFILTERWHITEBALANCE				= 0x00000108,		//ICommand W - B Calculate white-balance gains. (Max=max(Bavg,Gavg,Ravg). Bgain=Max/Bavg. Ggain=Max/Gavg. Rgain=Max/Ravg)
	PARAM_ID_COMMON_BAYERFILTERRESET                    = 0x00000109,		//ICommand W - B Reset the RGB filter configuration. (Gains are set to 1; offsets, to 0.)


	//Internal Display
	PARAM_ID_COMMON_DISPLAYENABLE                       = 0x00000200,		//IBoolean R/W - B Set Internal Display Enable (0:Off 1:On)
	PARAM_ID_COMMON_DISPLAYHWND							= 0x00000201,		//IInteger R/W - B Set Internal Display Window Handle (0:Modeless x:Window)
	PARAM_ID_COMMON_DISPLAYVSYNC                        = 0x00000202,		//IBoolean R/W - B Sets vertical synchronization flag. (0:Off 1:On)
	PARAM_ID_COMMON_DISPLAYFPS                          = 0x00000203,		//IFloat R - B Returns the average effective frame per second rate. 
	PARAM_ID_COMMON_DISPLAYWIDTH                        = 0x00000204,		//IInteger R/W - B Set the Display window's position and size(Width)
	PARAM_ID_COMMON_DISPLAYHEIGHT                       = 0x00000205,		//IInteger R/W - B Set the Display window's position and size(Height)
	PARAM_ID_COMMON_DISPLAYPOSX							= 0x00000206,		//IInteger R/W - B Set the Display window's position and size(X)
	PARAM_ID_COMMON_DISPLAYPOSY							= 0x00000207,		//IInteger R/W - B Set the Display window's position and size(Y)


	//2.1Device Control Summary(Contains the features related to the control and information of the device)
	PARAM_ID_SFNC_UNDEFINE								= 0x00001000,		//SFNC Param undefine 
	PARAM_ID_SFNC_DEVICETYPE							= 0x00001001,		//O IEnumeration R - G Returns the device type.
	PARAM_ID_SFNC_DEVICESCANTYPE						= 0x00001002,		//R IEnumeration R/(W) - E Scan type of the sensor of the device.
	PARAM_ID_SFNC_DEVICEVENDORNAME						= 0x00001003,		//R IString R - B Name of the manufacturer of the device.
	PARAM_ID_SFNC_DEVICEMODELNAME 						= 0x00001004,		//R IString R - B Model of the device.
	PARAM_ID_SFNC_DEVICEFAMILYNAME						= 0x00001005,		//O IString R - B Identifier of the product family of the device.
	PARAM_ID_SFNC_DEVICEMANUFACTURERINFO 				= 0x00001006,		//R IString R - B Manufacturer information about the device.
	PARAM_ID_SFNC_DEVICEVERSION 						= 0x00001007,		//R IString R - B Version of the device.
	PARAM_ID_SFNC_DEVICEFIRMWAREVERSION 				= 0x00001008,		//R IString R - B Version of the firmware in the device.
	PARAM_ID_SFNC_DEVICESERIALNUMBER 					= 0x00001009,		//R IString R - E Device's serial number.
	PARAM_ID_SFNC_DEVICEID  							= 0x0000100A,		//R IString R - I This feature is deprecated (See DeviceSerialNumber).
	PARAM_ID_SFNC_DEVICEUSERID 							= 0x0000100B,		//O IString R/W - B User-programmable device identifier.
	PARAM_ID_SFNC_DEVICESFNCVERSIONMAJOR 				= 0x0000100C,		//R IInteger R - B Major version of the Standard Features Naming Convention that was used to create the device's GenICam XML.
	PARAM_ID_SFNC_DEVICESFNCVERSIONMINOR 				= 0x0000100D,		//R IInteger R - B Minor version of the Standard Features Naming Convention that was used to create the device's GenICam XML.
	PARAM_ID_SFNC_DEVICESFNCVERSIONSUBMINOR 			= 0x0000100E,		//R IInteger R - B Sub minor version of Standard Features Naming Convention that was used to create the device's GenICam XML.
	PARAM_ID_SFNC_DEVICEMANIFESTENTRYSELECTOR			= 0x0000100F,		//O IInteger R/W - G Selects the manifest entry to reference.
	PARAM_ID_SFNC_DEVICEMANIFESTXMLMAJORVERSION			= 0x00001010,		//O IInteger R - G Indicates the major version number of the GenICam XML file of the selected manifest entry.
	PARAM_ID_SFNC_DEVICEMANIFESTXMLMINORVERSION			= 0x00001011,		//O IInteger R - G Indicates the minor version number of the GenICam XML file of the selected manifest entry.
	PARAM_ID_SFNC_DEVICEMANIFESTXMLSUBMINORVERSION		= 0x00001012,		//O IInteger R - G Indicates the subminor version number of the GenICam XML file of the selected manifest entry.
	PARAM_ID_SFNC_DEVICEMANIFESTSCHEMAMAJORVERSION		= 0x00001013,		//O IInteger R - G Indicates the major version number of the schema file of the selected manifest entry.
	PARAM_ID_SFNC_DEVICEMANIFESTSCHEMAMINORVERSION 		= 0x00001014,		//O IInteger R - G Indicates the minor version number of the schema file of the selected manifest entry.
	PARAM_ID_SFNC_DEVICEMANIFESTPRIMARYURL				= 0x00001015,		//O IString R - G Indicates the first URL to the GenICam XML device description file of the selected manifest entry.
	PARAM_ID_SFNC_DEVICEMANIFESTSECONDARYURL 			= 0x00001016,		//O IString R - G Indicates the second URL to the GenICam XML device description file of the selected manifest entry.
	PARAM_ID_SFNC_DEVICETLTYPE							= 0x00001017,		//R IEnumeration R - B Transport Layer type of the device.
	PARAM_ID_SFNC_DEVICETLVERSIONMAJOR 					= 0x00001018,		//R IInteger R - B Major version of the Transport Layer of the device.
	PARAM_ID_SFNC_DEVICETLVERSIONMINOR 					= 0x00001019,		//R IInteger R - B Minor version of the Transport Layer of the device.
	PARAM_ID_SFNC_DEVICETLVERSIONSUBMINOR 				= 0x0000101A,		//R IInteger R - B Sub minor version of the Transport Layer of the device.
	PARAM_ID_SFNC_DEVICEGENCPVERSIONMAJOR 				= 0x0000101B,		//R IInteger R - B Major version of the GenCP protocol supported by the device.
	PARAM_ID_SFNC_DEVICEGENCPVERSIONMINOR 				= 0x0000101C,		//R IInteger R - B Minor version of the GenCP protocol supported by the device.
	PARAM_ID_SFNC_DEVICEMAXTHROUGHPUT 					= 0x0000101D,		//O IInteger R Bps E Maximum bandwidth of the data that can be streamed out of the device.
	PARAM_ID_SFNC_DEVICECONNECTIONSELECTOR 				= 0x0000101E,		//R IInteger R/(W) - B Selects which Connection of the device to control.
	PARAM_ID_SFNC_DEVICECONNECTIONSPEED					= 0x0000101F,		//O IInteger R Bps E Indicates the speed of transmission of the specified Connection.
	PARAM_ID_SFNC_DEVICECONNECTIONSTATUS				= 0x00001020,		//O IEnumeration R - E Indicates the status of the specified Connection.
	PARAM_ID_SFNC_DEVICELINKSELECTOR 					= 0x00001021,		//R IInteger R/(W) - B Selects which Link of the device to control.
	PARAM_ID_SFNC_DEVICELINKSPEED						= 0x00001022,		//O IInteger R Bps E Indicates the speed of transmission negotiated on the specified Link.
	PARAM_ID_SFNC_DEVICELINKTHROUGHPUTLIMITMODE			= 0x00001023,		//R IEnumeration R/W - E Controls if the DeviceLinkThroughputLimit is active.
	PARAM_ID_SFNC_DEVICELINKTHROUGHPUTLIMIT				= 0x00001024,		//R IInteger R/(W) Bps E Limits the maximum bandwidth of the data that will be streamed out by the device on the selected Link.
	PARAM_ID_SFNC_DEVICELINKCONNECTIONCOUNT				= 0x00001025,		//O IInteger R - B Returns the number of physical connection of the device used by a particular Link.
	PARAM_ID_SFNC_DEVICELINKHEARTBEATMODE				= 0x00001026,		//O IEnumeration R/W - E Activate or deactivate the Link's heartbeat.
	PARAM_ID_SFNC_DEVICELINKHEARTBEATTIMEOUT			= 0x00001027,		//O IFloat R/W us G Controls the current heartbeat timeout of the specific Link.
	PARAM_ID_SFNC_DEVICELINKCOMMANDTIMEOUT				= 0x00001028,		//O IFloat R us G Indicates the command timeout of the specified Link.
	PARAM_ID_SFNC_DEVICESTREAMCHANNELCOUNT 				= 0x00001029,		//O IInteger R - E Indicates the number of streaming channels supported by the device.
	PARAM_ID_SFNC_DEVICESTREAMCHANNELSELECTOR 			= 0x0000102A,		//O IInteger R/W - E Selects the stream channel to control.
	PARAM_ID_SFNC_DEVICESTREAMCHANNELTYPE				= 0x0000102B,		//O IEnumeration R - G Reports the type of the stream channel.
	PARAM_ID_SFNC_DEVICESTREAMCHANNELLINK				= 0x0000102C,		//O IInteger R/(W) - G Index of device's Link to use for streaming the specifed stream channel.
	PARAM_ID_SFNC_DEVICESTREAMCHANNELENDIANNESS			= 0x0000102D,		//O IEnumeration R/(W) - G Endianness of multi-byte pixel data for this stream.
	PARAM_ID_SFNC_DEVICESTREAMCHANNELPACKETSIZE			= 0x0000102E,		//R IInteger R/(W) B E Specifies the stream packet size, in bytes, to send on the selected channel for a Transmitter or specifies the maximum packet size supported by a receiver.
	PARAM_ID_SFNC_DEVICEEVENTCHANNELCOUNT 				= 0x0000102F,		//O IInteger R - E Indicates the number of event channels supported by the device.
	PARAM_ID_SFNC_DEVICEMESSAGECHANNELCOUNT 			= 0x00001030,		//O IInteger R - E This feature is deprecated (See DeviceEventChannelCount).
	PARAM_ID_SFNC_DEVICECHARACTERSET 					= 0x00001031,		//O IEnumeration R - G Character set used by the strings of the device's bootstrap registers.
	PARAM_ID_SFNC_DEVICERESET 							= 0x00001032,		//R ICommand W - G Resets the device to its power up state.
	PARAM_ID_SFNC_DEVICEINDICATORMODE 					= 0x00001033,		//O IEnumeration R/W - E Controls the behavior of the indicators (such as LEDs) showing the status of the Device.
	PARAM_ID_SFNC_DEVICEFEATUREPERSISTENCESTART 		= 0x00001034,		//O ICommand (R)/W - G Indicate to the device and GenICam XML to get ready for persisting of all streamable features.
	PARAM_ID_SFNC_DEVICEFEATUREPERSISTENCEEND 			= 0x00001035,		//O ICommand (R)/W - G Indicate to the device the end of feature persistence.
	PARAM_ID_SFNC_DEVICEREGISTERSSTREAMINGSTART 		= 0x00001036,		//R ICommand (R)/W - G Prepare the device for registers streaming without checking for consistency.
	PARAM_ID_SFNC_DEVICEREGISTERSSTREAMINGEND 			= 0x00001037,		//R ICommand (R)/W - G Announce the end of registers streaming.
	PARAM_ID_SFNC_DEVICEREGISTERSCHECK 					= 0x00001038,		//R ICommand (R)/W - E Perform the validation of the current register set for consistency.
	PARAM_ID_SFNC_DEVICEREGISTERSVALID 					= 0x00001039,		//R IBoolean R - E Returns if the current register set is valid and consistent.
	PARAM_ID_SFNC_DEVICEREGISTERSENDIANNESS 			= 0x0000103A,		//O IEnumeration R/(W) - G Endianness of the registers of the device.
	PARAM_ID_SFNC_DEVICETEMPERATURESELECTOR 			= 0x0000103B,		//O IEnumeration R/W - E Selects the location within the device, where the temperature will be measured.
	PARAM_ID_SFNC_DEVICETEMPERATURE						= 0x0000103C,		//O IFloat R C E Device temperature in degrees Celsius (C).
	PARAM_ID_SFNC_DEVICECLOCKSELECTOR 					= 0x0000103D,		//O IEnumeration R/(W) - E Selects the clock frequency to access from the device.
	PARAM_ID_SFNC_DEVICECLOCKFREQUENCY					= 0x0000103E,		//O IFloat R/(W) Hz E Returns the frequency of the selected Clock.
	PARAM_ID_SFNC_DEVICESERIALPORTSELECTOR 				= 0x0000103F,		//R IEnumeration R/(W) - E Selects which serial port of the device to control.
	PARAM_ID_SFNC_DEVICESERIALPORTBAUDRATE				= 0x00001040,		//R IEnumeration R/(W) - E This feature controls the baud rate used by the selected serial port.
	PARAM_ID_SFNC_TIMESTAMP 							= 0x00001041,		//R IInteger R ns E Reports the current value of the device timestamp counter.
	PARAM_ID_SFNC_TIMESTAMPRESET 						= 0x00001042,		//O ICommand (R)/W - E Resets the current value of the device timestamp counter.
	PARAM_ID_SFNC_TIMESTAMPLATCH 						= 0x00001043,		//O ICommand (R)/W - E Latches the current timestamp counter into TimestampLatchValue.
	PARAM_ID_SFNC_TIMESTAMPLATCHVALUE 					= 0x00001044,		//O IInteger R ns E Returns the latched value of the timestamp counter.

	//2.2Image Format Control(Contains the features related to the format of the transmitted image)
	PARAM_ID_SFNC_SENSORWIDTH 							= 0x00001101,		//R IInteger R - E Effective width of the sensor in pixels.
	PARAM_ID_SFNC_SENSORHEIGHT 							= 0x00001102,		//R IInteger R - E Effective height of the sensor in pixels.
	PARAM_ID_SFNC_SENSORSHUTTERMODE 					= 0x00001103,		//O IEnumeration R/(W) - G Sets the shutter mode of the device.
	PARAM_ID_SFNC_SENSORTAPS 							= 0x00001104,		//O IEnumeration R/(W) - E Number of taps of the camera sensor.
	PARAM_ID_SFNC_SENSORDIGITIZATIONTAPS 				= 0x00001105,		//O IEnumeration R/(W) - E Number of digitized samples outputted simultaneously by the camera A/D conversion stage.
	PARAM_ID_SFNC_WIDTHMAX 								= 0x00001106,		//R IInteger R - E Maximum width of the image (in pixels).
	PARAM_ID_SFNC_HEIGHTMAX 							= 0x00001107,		//R IInteger R - E Maximum height of the image (in pixels).
	PARAM_ID_SFNC_REGIONSELECTOR 						= 0x00001108,		//O IEnumeration R/(W) - B Selects the Region of interest to control.
	PARAM_ID_SFNC_REGIONMODE							= 0x00001109,		//O IEnumeration R/W - B Controls if the selected Region of interest is active and streaming.
	PARAM_ID_SFNC_REGIONDESTINATION						= 0x0000110A,		//O IEnumeration R/(W) - E Control the destination of the selected region.
	PARAM_ID_SFNC_REGIONIDVALUE							= 0x0000110B,		//O IInteger R - E Returns a unique Identifier value that correspond to the selected Region.
	PARAM_ID_SFNC_COMPONENTSELECTOR 					= 0x0000110C,		//O IEnumeration R/W - B Selects a component to activate/deactivate its data streaming.
	PARAM_ID_SFNC_COMPONENTENABLE						= 0x0000110D,		//O IBoolean R/(W) - B Controls if the selected component streaming is active.
	PARAM_ID_SFNC_COMPONENTIDVALUE						= 0x0000110E,		//O IInteger R - E Returns a unique Identifier value that correspond to the selected Component type.
	PARAM_ID_SFNC_IMAGECOMPONENTSELECTOR 				= 0x0000110F,		//O IEnumeration R/W - B This feature is deprecated (See ComponentSelector).
	PARAM_ID_SFNC_IMAGECOMPONENTENABLE					= 0x00001110,		//O IBoolean R/(W) - B This feature is deprecated (See ComponentEnable).
	PARAM_ID_SFNC_WIDTH									= 0x00001111,		//R IInteger R/(W) - B Width of the image provided by the device (in pixels).
	PARAM_ID_SFNC_HEIGHT								= 0x00001112,		//R IInteger R/(W) - B Height of the image provided by the device (in pixels).
	PARAM_ID_SFNC_OFFSETX								= 0x00001113,		//R IInteger R/W - B Horizontal offset from the origin to the region of interest (in pixels).
	PARAM_ID_SFNC_OFFSETY								= 0x00001114,		//R IInteger R/W - B Vertical offset from the origin to the region of interest (in pixels).
	PARAM_ID_SFNC_LINEPITCHENABLE						= 0x00001115,		//R IBoolean R/W - E This feature controls whether the LinePitch feature is writable.
	PARAM_ID_SFNC_LINEPITCH								= 0x00001116,		//R IInteger R/W B E Total number of bytes between the starts of 2 consecutive lines.
	PARAM_ID_SFNC_BINNINGSELECTOR 						= 0x00001117,		//O IEnumeration R/(W) - E Selects which binning engine is controlled by the BinningHorizontal and BinningVertical features.
	PARAM_ID_SFNC_BINNINGHORIZONTALMODE					= 0x00001118,		//O IEnumeration R/(W) - E Sets the mode to use to combine horizontal photo-sensitive cells together when BinningHorizontal is used.
	PARAM_ID_SFNC_BINNINGHORIZONTAL						= 0x00001119,		//O IInteger R/W - E Number of horizontal photo-sensitive cells to combine together.
	PARAM_ID_SFNC_BINNINGVERTICALMODE					= 0x0000111A,		//O IEnumeration R/(W) - E Sets the mode used to combine horizontal photo-sensitive cells together when BinningVertical is used.
	PARAM_ID_SFNC_BINNINGVERTICAL						= 0x0000111B,		//O IInteger R/W - E Number of vertical photo-sensitive cells to combine together.
	PARAM_ID_SFNC_DECIMATIONHORIZONTALMODE 				= 0x0000111C,		//O IEnumeration R/(W) - E Sets the mode used to reduce the horizontal resolution when DecimationHorizontal is used.
	PARAM_ID_SFNC_DECIMATIONHORIZONTAL 					= 0x0000111D,		//O IInteger R/W - E Horizontal sub-sampling of the image.
	PARAM_ID_SFNC_DECIMATIONVERTICALMODE 				= 0x0000111E,		//O IEnumeration R/(W) - E Sets the mode used to reduce the Vertical resolution when DecimationVertical is used.
	PARAM_ID_SFNC_DECIMATIONVERTICAL 					= 0x0000111F,		//O IInteger R/W - E Vertical sub-sampling of the image.
	PARAM_ID_SFNC_REVERSEX 								= 0x00001120,		//R IBoolean R/W - E Flip horizontally the image sent by the device.
	PARAM_ID_SFNC_REVERSEY 								= 0x00001121,		//R IBoolean R/W - E Flip vertically the image sent by the device.
	PARAM_ID_SFNC_PIXELFORMAT							= 0x00001122,		//R IEnumeration R/(W) - B Format of the pixels provided by the device.
	PARAM_ID_SFNC_PIXELFORMATINFOSELECTOR 				= 0x00001123,		//R IEnumeration R/W - G Select the pixel format for which the information will be returned
	PARAM_ID_SFNC_PIXELFORMATINFOID						= 0x00001124,		//R IInteger R - G Returns the value used by the streaming channels to identify the selected pixel format.
	PARAM_ID_SFNC_PIXELCODING 							= 0x00001125,		//R IEnumeration R/(W) - E This feature is deprecated.
	PARAM_ID_SFNC_PIXELSIZE								= 0x00001126,		//R IEnumeration R/(W) - E Total size in bits of a pixel of the image.
	PARAM_ID_SFNC_PIXELCOLORFILTER						= 0x00001127,		//R IEnumeration R/(W) - E Type of color filter that is applied to the image.
	PARAM_ID_SFNC_PIXELDYNAMICRANGEMIN					= 0x00001128,		//O IInteger R/W - E Minimum value that can be returned during the digitization process.
	PARAM_ID_SFNC_PIXELDYNAMICRANGEMAX					= 0x00001129,		//O IInteger R/W - E Maximum value that will be returned during the digitization process.
	PARAM_ID_SFNC_TESTPATTERNGENERATORSELECTOR 			= 0x0000112A,		//O IEnumeration R/(W) - B Selects which test pattern generator is controlled by the TestPattern feature.
	PARAM_ID_SFNC_TESTPATTERN							= 0x0000112B,		//O IEnumeration R/W - B Selects the type of test pattern that is generated by the device as image source.
	PARAM_ID_SFNC_TESTIMAGESELECTOR 					= 0x0000112C,		//O IEnumeration R/W - I This feature is deprecated (See TestPattern).
	PARAM_ID_SFNC_DEINTERLACING 						= 0x0000112D,		//O IEnumeration R/W - B Controls how the device performs de-interlacing.
	PARAM_ID_SFNC_IMAGECOMPRESSIONMODE 					= 0x0000112E,		//O IEnumeration R/W - B Enable a specific image compression mode as the base mode for image transfer.
	PARAM_ID_SFNC_IMAGECOMPRESSIONRATEOPTION 			= 0x0000112F,		//O IEnumeration R/W - E Two rate controlling options are offered: fixed bit rate or fixed quality.
	PARAM_ID_SFNC_IMAGECOMPRESSIONQUALITY 				= 0x00001130,		//O IInteger R/(W) - E Control the quality of the produced compressed stream.
	PARAM_ID_SFNC_IMAGECOMPRESSIONBITRATE 				= 0x00001131,		//O IFloat R/(W) Mbps E Control the rate of the produced compressed stream.
	PARAM_ID_SFNC_IMAGECOMPRESSIONJPEGFORMATOPTION 		= 0x00001132,		//O IEnumeration R/W - E When JPEG is selected as the compression format, a device might optionally offer better control over JPEG-specific options through this feature.

	//2.3 Acquisition Control(Contains the features related to image acquisition, including trigger and exposure control )
	PARAM_ID_SFNC_ACQUISITIONMODE 						= 0x00001201,		//R IEnumeration R/(W) - B Sets the acquisition mode of the device.
	PARAM_ID_SFNC_ACQUISITIONSTART 						= 0x00001202,		//R ICommand (R)/W - B Starts the Acquisition of the device.
	PARAM_ID_SFNC_ACQUISITIONSTOP 						= 0x00001203,		//R ICommand (R)/W - B Stops the Acquisition of the device at the end of the current Frame.
	PARAM_ID_SFNC_ACQUISITIONABORT 						= 0x00001204,		//R ICommand (R)/W - E Aborts the Acquisition immediately.
	PARAM_ID_SFNC_ACQUISITIONARM 						= 0x00001205,		//O ICommand (R)/W - E Arms the device before an AcquisitionStart command.
	PARAM_ID_SFNC_ACQUISITIONFRAMECOUNT 				= 0x00001206,		//R IInteger R/W - B Number of frames to acquire in MultiFrame Acquisition mode.
	PARAM_ID_SFNC_ACQUISITIONBURSTFRAMECOUNT 			= 0x00001207,		//O IInteger R/W - B Number of frames to acquire for each FrameBurstStart trigger.
	PARAM_ID_SFNC_ACQUISITIONFRAMERATE 					= 0x00001208,		//R IFloat R/W Hz B Controls the acquisition rate (in Hertz) at which the frames are captured.
	PARAM_ID_SFNC_ACQUISITIONFRAMERATEENABLE 			= 0x00001209,		//R IBoolean R/W - E Controls if the AcquisitionFrameRate feature is writable and used to control the acquisition rate.
	PARAM_ID_SFNC_ACQUISITIONLINERATE 					= 0x0000120A,		//R IFloat R/W Hz B Controls the rate (in Hertz) at which the Lines in a Frame are captured.
	PARAM_ID_SFNC_ACQUISITIONLINERATEENABLE 			= 0x0000120B,		//R IBoolean R/W - E Controls if the AcquisitionLineRate feature is writable and used to control the acquisition rate.
	PARAM_ID_SFNC_ACQUISITIONSTATUSSELECTOR 			= 0x0000120C,		//R IEnumeration R/W - E Selects the internal acquisition signal to read using AcquisitionStatus.
	PARAM_ID_SFNC_ACQUISITIONSTATUS						= 0x0000120D,		//R IBoolean R - E Reads the state of the internal acquisition signal selected using AcquisitionStatusSelector.
	PARAM_ID_SFNC_TRIGGERSELECTOR 						= 0x0000120E,		//R IEnumeration R/W - B Selects the type of trigger to configure.
	PARAM_ID_SFNC_TRIGGERMODE							= 0x0000120F,		//R IEnumeration R/W - B Controls if the selected trigger is active.
	PARAM_ID_SFNC_TRIGGERSOFTWARE						= 0x00001210,		//R ICommand (R)/W - B Generates an internal trigger.
	PARAM_ID_SFNC_TRIGGERSOURCE							= 0x00001211,		//R IEnumeration R/W - B Specifies the internal signal or physical input Line to use as the trigger source.
	PARAM_ID_SFNC_TRIGGERACTIVATION						= 0x00001212,		//R IEnumeration R/W - B Specifies the activation mode of the trigger.
	PARAM_ID_SFNC_TRIGGEROVERLAP						= 0x00001213,		//R IEnumeration R/W - E Specifies the type trigger overlap permitted with the previous frame or line.
	PARAM_ID_SFNC_TRIGGERDELAY							= 0x00001214,		//R IFloat R/W us E Specifies the delay in microseconds (us) to apply after the trigger reception before activating it.
	PARAM_ID_SFNC_TRIGGERDIVIDER						= 0x00001215,		//R IInteger R/W - E Specifies a division factor for the incoming trigger pulses.
	PARAM_ID_SFNC_TRIGGERMULTIPLIER						= 0x00001216,		//R IInteger R/W - E Specifies a multiplication factor for the incoming trigger pulses.
	PARAM_ID_SFNC_EXPOSUREMODE 							= 0x00001217,		//R IEnumeration R/W - B Sets the operation mode of the Exposure.
	PARAM_ID_SFNC_EXPOSURETIMEMODE 						= 0x00001218,		//O IEnumeration R/W - B Sets the configuration mode of the ExposureTime feature.
	PARAM_ID_SFNC_EXPOSURETIMESELECTOR 					= 0x00001219,		//O IEnumeration R/W - B Selects which exposure time is controlled by the ExposureTime feature.
	PARAM_ID_SFNC_EXPOSURETIME							= 0x0000121A,		//R IFloat R/W us B Sets the Exposure time when ExposureMode is Timed and ExposureAuto is Off.
	PARAM_ID_SFNC_EXPOSUREAUTO 							= 0x0000121B,		//O IEnumeration R/W - B Sets the automatic exposure mode when ExposureMode is Timed.
	PARAM_ID_SFNC_MULTISLOPEMODE 						= 0x0000121C,		//O IEnumeration R/W - B Controls multi-slope exposure state.
	PARAM_ID_SFNC_MULTISLOPEKNEEPOINTCOUNT 				= 0x0000121D,		//O IInteger R/W - E The number of knee-points as well as the number of additional exposure slopes used for multi-slope exposure.
	PARAM_ID_SFNC_MULTISLOPEKNEEPOINTSELECTOR 			= 0x0000121E,		//O IInteger R/W - E Selects the parameters for controlling an additional slope in multi-slope exposure.
	PARAM_ID_SFNC_MULTISLOPEEXPOSURELIMIT				= 0x0000121F,		//O IFloat R/W % E Percent of the ExposureTime at a certain knee-point of multi-slope exposure.
	PARAM_ID_SFNC_MULTISLOPESATURATIONTHRESHOLD    		= 0x00001220,		//O IFloat R/(W) % E The percentage of the full saturation that is applied at a certain kneepoint of a multi-slope exposure.
	PARAM_ID_SFNC_MULTISLOPEINTENSITYLIMIT				= 0x00001221,		//O IFloat R/(W) % E The relative intensity which divides intensities influenced by different exposure slopes.
	PARAM_ID_SFNC_MULTISLOPEEXPOSUREGRADIENT			= 0x00001222,		//O IFloat R/(W) - E The gradient of the additional slope that is defined by this knee-point.

	//2.4 Analog Control(Contains the features related to the video signal conditioning in the analog domain)
	PARAM_ID_SFNC_GAINSELECTOR 							= 0x00001301,		//O IEnumeration R/W - B Selects which Gain is controlled by the various Gain features.
	PARAM_ID_SFNC_GAIN									= 0x00001302,		//O IFloat R/W - B Controls the selected gain as an absolute physical value.
	PARAM_ID_SFNC_GAINAUTO								= 0x00001303,		//O IEnumeration R/W - B Sets the automatic gain control (AGC) mode.
	PARAM_ID_SFNC_GAINAUTOBALANCE 						= 0x00001304,		//O IEnumeration R/W - B Sets the mode for automatic gain balancing between the sensor color channels or taps.
	PARAM_ID_SFNC_BLACKLEVELSELECTOR 					= 0x00001305,		//O IEnumeration R/W - E Selects which Black Level is controlled by the various Black Level features.
	PARAM_ID_SFNC_BLACKLEVEL							= 0x00001306,		//O IFloat R/W - E Controls the analog black level as an absolute physical value.
	PARAM_ID_SFNC_BLACKLEVELAUTO						= 0x00001307,		//O IEnumeration R/W - E Controls the mode for automatic black level adjustment.
	PARAM_ID_SFNC_BLACKLEVELAUTOBALANCE 				= 0x00001308,		//O IEnumeration R/W - E Controls the mode for automatic black level balancing between the sensor color channels or taps.
	PARAM_ID_SFNC_WHITECLIPSELECTOR 					= 0x00001309,		//O IEnumeration R/W - E Selects which White Clip to control.
	PARAM_ID_SFNC_WHITECLIP								= 0x0000130A,		//O IFloat R/W - E Controls the maximal intensity taken by the video signal before being clipped as an absolute physical value.
	PARAM_ID_SFNC_BALANCERATIOSELECTOR 					= 0x0000130B,		//O IEnumeration R/W - E Selects which Balance ratio to control.
	PARAM_ID_SFNC_BALANCERATIO							= 0x0000130C,		//O IFloat R/W - E Controls ratio of the selected color component to a reference color component.
	PARAM_ID_SFNC_BALANCEWHITEAUTO 						= 0x0000130D,		//O IEnumeration R/W - E Controls the mode for automatic white balancing between the color channels.
	PARAM_ID_SFNC_GAMMA 								= 0x0000130E,		//O IFloat R/W - B Controls the gamma correction of pixel intensity.
	
	//2.5 LUT Control(Contains the features related to the look-up table (LUT) control)
	PARAM_ID_SFNC_LUTSELECTOR 							= 0x00001401,		//O IEnumeration R/W - E Selects which LUT to control.
	PARAM_ID_SFNC_LUTENABLE								= 0x00001402,		//O IBoolean R/W - E Activates the selected LUT.
	PARAM_ID_SFNC_LUTINDEX								= 0x00001403,		//O IInteger R/W - G Control the index (offset) of the coefficient to access in the selected LUT.
	PARAM_ID_SFNC_LUTVALUE								= 0x00001404,		//O IInteger R/W - G Returns the Value at entry LUTIndex of the LUT selected by LUTSelector.
	PARAM_ID_SFNC_LUTVALUEALL							= 0x00001405,		//O IRegister R/W - G Accesses all the LUT coefficients in a single access without using individual LUTIndex.

	//2.6 Color Transformation Control(Contains the features related to the control of the color transformation)
	PARAM_ID_SFNC_COLORTRANSFORMATIONSELECTOR 			= 0x00001501,		//O IEnumeration R/W - E Selects which Color Transformation module is controlled by the various Color Transformation features.
	PARAM_ID_SFNC_COLORTRANSFORMATIONENABLE				= 0x00001502,		//O IBoolean R/W - E Activates the selected Color Transformation module.
	PARAM_ID_SFNC_COLORTRANSFORMATIONVALUESELECTOR		= 0x00001503,		//O IEnumeration R/W - E Selects the Gain factor or Offset of the Transformation matrix to access in the selected Color Transformation module.
	PARAM_ID_SFNC_COLORTRANSFORMATIONVALUE				= 0x00001504,		//O IFloat R/W - E Represents the value of the selected Gain factor or Offset inside the Transformation matrix.

	//2.7 Digital I/O Control(Contains the features related to the control of the input and output pins of the device )
	PARAM_ID_SFNC_LINESELECTOR 							= 0x00001601,		//R IEnumeration R/W - E Selects the physical line (or pin) of the external device connector or the virtual line of the Transport Layer to configure.
	PARAM_ID_SFNC_LINEMODE								= 0x00001602,		//O IEnumeration R/W - E Controls if the physical Line is used to Input or Output a signal.
	PARAM_ID_SFNC_LINEINVERTER							= 0x00001603,		//R IBoolean R/W - E Controls the inversion of the signal of the selected input or output Line.
	PARAM_ID_SFNC_LINESTATUS							= 0x00001604,		//R IBoolean R - E Returns the current status of the selected input or output Line.
	PARAM_ID_SFNC_LINESTATUSALL 						= 0x00001605,		//O IInteger R - E Returns the current status of all available Line signals at time of polling in a single bitfield.
	PARAM_ID_SFNC_LINESOURCE							= 0x00001606,		//R IEnumeration R/W - E Selects which internal acquisition or I/O source signal to output on the selected Line.
	PARAM_ID_SFNC_LINEFORMAT							= 0x00001607,		//O IEnumeration R/W - E Controls the current electrical format of the selected physical input or output Line.
	PARAM_ID_SFNC_USEROUTPUTSELECTOR 					= 0x00001608,		//R IEnumeration R/W - E Selects which bit of the User Output register will be set by UserOutputValue.
	PARAM_ID_SFNC_USEROUTPUTVALUE						= 0x00001609,		//R IBoolean R/W - E Sets the value of the bit selected by UserOutputSelector.
	PARAM_ID_SFNC_USEROUTPUTVALUEALL 					= 0x0000160A,		//O IInteger R/W - E Sets the value of all the bits of the User Output register.
	PARAM_ID_SFNC_USEROUTPUTVALUEALLMASK 				= 0x0000160B,		//O IInteger R/W - E Sets the write mask to apply to the value specified by UserOutputValueAll before writing it in the User Output register.

	//2.8 Counter and Timer Control(Contains the features related to the usage of programmable counters and timers)
	PARAM_ID_SFNC_COUNTERSELECTOR 						= 0x00001701,		//R IEnumeration R/W - E Selects which Counter to configure.
	PARAM_ID_SFNC_COUNTEREVENTSOURCE					= 0x00001702,		//R IEnumeration R/W - E Select the events that will be the source to increment the Counter.
	PARAM_ID_SFNC_COUNTEREVENTACTIVATION				= 0x00001703,		//R IEnumeration R/W - E Selects the Activation mode Event Source signal.
	PARAM_ID_SFNC_COUNTERRESETSOURCE					= 0x00001704,		//R IEnumeration R/W - E Selects the signals that will be the source to reset the Counter.
	PARAM_ID_SFNC_COUNTERRESETACTIVATION				= 0x00001705,		//R IEnumeration R/W - E Selects the Activation mode of the Counter Reset Source signal.
	PARAM_ID_SFNC_COUNTERRESET							= 0x00001706,		//R ICommand (R)/W - E Does a software reset of the selected Counter and starts it.
	PARAM_ID_SFNC_COUNTERVALUE							= 0x00001707,		//R IInteger R/W - E Reads or writes the current value of the selected Counter.
	PARAM_ID_SFNC_COUNTERVALUEATRESET					= 0x00001708,		//R IInteger R - E Reads the value of the selected Counter when it was reset by a trigger or by an explicit CounterReset command.
	PARAM_ID_SFNC_COUNTERDURATION						= 0x00001709,		//R IInteger R/W - E Sets the duration (or number of events) before the CounterEnd event is generated.
	PARAM_ID_SFNC_COUNTERSTATUS							= 0x0000170A,		//R IEnumeration R - E Returns the current status of the Counter.
	PARAM_ID_SFNC_COUNTERTRIGGERSOURCE					= 0x0000170B,		//R IEnumeration R/W - E Selects the source to start the Counter.
	PARAM_ID_SFNC_COUNTERTRIGGERACTIVATION				= 0x0000170C,		//R IEnumeration R/W - E Selects the activation mode of the trigger to start the Counter.
	PARAM_ID_SFNC_TIMERSELECTOR 						= 0x0000170D,		//R IEnumeration R/W - E Selects which Timer to configure.
	PARAM_ID_SFNC_TIMERDURATION							= 0x0000170E,		//R IFloat R/W us E Sets the duration (in microseconds) of the Timer pulse.
	PARAM_ID_SFNC_TIMERDELAY							= 0x0000170F,		//R IFloat R/W us E Sets the duration (in microseconds) of the delay to apply at the reception of a trigger before starting the Timer.
	PARAM_ID_SFNC_TIMERRESET							= 0x00001710,		//R ICommand (R)/W - E Does a software reset of the selected timer and starts it.
	PARAM_ID_SFNC_TIMERVALUE							= 0x00001711,		//R IFloat R/W us E Reads or writes the current value (in microseconds) of the selected Timer.
	PARAM_ID_SFNC_TIMERSTATUS							= 0x00001712,		//R IEnumeration R - E Returns the current status of the Timer.
	PARAM_ID_SFNC_TIMERTRIGGERSOURCE					= 0x00001713,		//R IEnumeration R/W - E Selects the source of the trigger to start the Timer.
	PARAM_ID_SFNC_TIMERTRIGGERACTIVATION				= 0x00001714,		//R IEnumeration R/W - E Selects the activation mode of the trigger to start the Timer.

	//2.9 Encoder Control(Contains the features related to the usage of quadrature encoders )
	PARAM_ID_SFNC_ENCODERSELECTOR 						= 0x00001801,		//O IEnumeration R/W - E Selects which Encoder to configure.
	PARAM_ID_SFNC_ENCODERSOURCEA						= 0x00001802,		//O IEnumeration R/W - E Selects the signal which will be the source of the A input of the Encoder.
	PARAM_ID_SFNC_ENCODERSOURCEB						= 0x00001803,		//O IEnumeration R/W - E Selects the signal which will be the source of the B input of the Encoder.
	PARAM_ID_SFNC_ENCODERMODE							= 0x00001804,		//O IEnumeration R/W - E Selects if the count of encoder uses FourPhase mode with jitter filtering or the HighResolution mode without jitter filtering.
	PARAM_ID_SFNC_ENCODERDIVIDER						= 0x00001805,		//O IInteger R/W - E Sets how many Encoder increment/decrements that are needed generate an Encoder output pulse signal.
	PARAM_ID_SFNC_ENCODEROUTPUTMODE						= 0x00001806,		//O IEnumeration R/W - E Selects the conditions for the Encoder interface to generate a valid Encoder output signal.
	PARAM_ID_SFNC_ENCODERSTATUS							= 0x00001807,		//O IEnumeration R - E Returns the motion status of the encoder.
	PARAM_ID_SFNC_ENCODERTIMEOUT						= 0x00001808,		//O IFloat R/W us E Sets the maximum time interval between encoder counter increments before the status turns to static.
	PARAM_ID_SFNC_ENCODERRESETSOURCE					= 0x00001809,		//R IEnumeration R/W - E Selects the signals that will be the source to reset the Encoder.
	PARAM_ID_SFNC_ENCODERRESETACTIVATION				= 0x0000180A,		//R IEnumeration R/W - E Selects the Activation mode of the Encoder Reset Source signal.
	PARAM_ID_SFNC_ENCODERRESET							= 0x0000180B,		//R ICommand (R)/W - E Does a software reset of the selected Encoder and starts it.
	PARAM_ID_SFNC_ENCODERVALUE							= 0x0000180C,		//R IInteger R/W - E Reads or writes the current value of the position counter of the selected Encoder.
	PARAM_ID_SFNC_ENCODERVALUEATRESET					= 0x0000180D,		//R IInteger R - E Reads the value of the of the position counter of the selected Encoder when it was reset by a signal or by an explicit EncoderReset command.

	//2.10 Logic Block Control(Contains the features related to the usage of the logic block)
	PARAM_ID_SFNC_LOGICBLOCKSELECTOR 					= 0x00001901,		//O IEnumeration R/W - G Specifies the Logic Block to configure.
	PARAM_ID_SFNC_LOGICBLOCKFUNCTION					= 0x00001902,		//O IEnumeration R/W - G Selects the combinational logic Function of the Logic Block to configure.
	PARAM_ID_SFNC_LOGICBLOCKINPUTNUMBER					= 0x00001903,		//O IInteger R/(W) - G Specifies the number of active signal inputs of the Logic Block.
	PARAM_ID_SFNC_LOGICBLOCKINPUTSELECTOR				= 0x00001904,		//O IInteger R/W - G Selects the Logic Block¡¯s input to configure.
	PARAM_ID_SFNC_LOGICBLOCKINPUTSOURCE					= 0x00001905,		//O IEnumeration R/W - G Selects the source signal for the input into the Logic Block.
	PARAM_ID_SFNC_LOGICBLOCKINPUTINVERTER				= 0x00001906,		//O IBoolean R/W - G Selects if the selected Logic Block Input source signal is inverted.
	PARAM_ID_SFNC_LOGICBLOCKLUTINDEX					= 0x00001907,		//O IInteger R/W - G Controls the index of the truth table to access in the selected LUT.
	PARAM_ID_SFNC_LOGICBLOCKLUTVALUE					= 0x00001908,		//O IBoolean R/W - G Read or Write the Value associated with the entry at index LogicBlockLUTIndex of the selected LUT.
	PARAM_ID_SFNC_LOGICBLOCKLUTVALUEALL					= 0x00001909,		//O IInteger R/W - G Sets the values of all the output bits of the selected LUT in one accessignoring LogicBlockLUTIndex.
	PARAM_ID_SFNC_LOGICBLOCKLUTSELECTOR					= 0x0000190A,		//O IEnumeration R/W - G Selects which of the two LUTs to configure when the selected Logic Block is a Latched dual LUTs (i.

	//2.11Software Signal Control(Contains the features related to the control of the Software Signal)
	PARAM_ID_SFNC_SOFTWARESIGNALCONTROL	 				= 0x00001A01,		//O ICategory R - B Category that contains the Software Signal Control features.
	PARAM_ID_SFNC_SOFTWARESIGNALSELECTOR 				= 0x00001A02,		//O IEnumeration R/W - B Selects which Software Signal features to control.
	PARAM_ID_SFNC_SOFTWARESIGNALPULSE					= 0x00001A03,		//O ICommand (R)/W - B Generates a pulse signal that can be used as a software trigger.

	//2.12Action Control(Contains the features related to the control of the Action command mechanism)
	PARAM_ID_SFNC_ACTIONUNCONDITIONALMODE 				= 0x00001B01,		//O IEnumeration R/W - G Enables the unconditional action command mode where action commands are processed even when the primary control channel is closed.
	PARAM_ID_SFNC_ACTIONDEVICEKEY 						= 0x00001B02,		//O IInteger W-O - G Provides the device key that allows the device to check the validity of action commands.
	PARAM_ID_SFNC_ACTIONQUEUESIZE 						= 0x00001B03,		//O IInteger R - G Indicates the size of the scheduled action commands queue.
	PARAM_ID_SFNC_ACTIONSELECTOR 						= 0x00001B04,		//O IInteger R/W - G Selects to which Action Signal further Action settings apply.
	PARAM_ID_SFNC_ACTIONGROUPMASK						= 0x00001B05,		//O IInteger R/W - G Provides the mask that the device will use to validate the action on reception of the action protocol message.
	PARAM_ID_SFNC_ACTIONGROUPKEY						= 0x00001B06,		//O IInteger R/W - G Provides the key that the device will use to validate the action on reception of the action protocol message.

	//2.13Event Control(Contains the features related to the generation of Event notifications by the device)
	PARAM_ID_SFNC_EVENTSELECTOR 						= 0x00001C01,		//R IEnumeration R/W - E Selects which Event to signal to the host application.
	PARAM_ID_SFNC_EVENTNOTIFICATION						= 0x00001C02,		//R IEnumeration R/W - E Activate or deactivate the notification to the host application of the occurrence of the selected Event.
	PARAM_ID_SFNC_EVENTFRAMETRIGGERDATA 				= 0x00001C03,		//R ICategory R - E Category that contains all the data features related to the FrameTrigger Event.
	PARAM_ID_SFNC_EVENTFRAMETRIGGER 					= 0x00001C04,		//R IInteger R - E Returns the unique Identifier of the FrameTrigger type of Event.
	PARAM_ID_SFNC_EVENTFRAMETRIGGERTIMESTAMP 			= 0x00001C05,		//R IInteger R - E Returns the Timestamp of the FrameTrigger Event.
	PARAM_ID_SFNC_EVENTFRAMETRIGGERFRAMEID 				= 0x00001C06,		//R IInteger R - E Returns the unique Identifier of the Frame (or image) that generated the FrameTrigger Event.
	PARAM_ID_SFNC_EVENTEXPOSUREENDDATA 					= 0x00001C07,		//R ICategory R - E Category that contains all the data features related to the ExposureEnd Event.
	PARAM_ID_SFNC_EVENTEXPOSUREEND 						= 0x00001C08,		//R IInteger R - E Returns the unique identifier of the ExposureEnd type of Event.
	PARAM_ID_SFNC_EVENTEXPOSUREENDTIMESTAMP 			= 0x00001C09,		//R IInteger R - E Returns the Timestamp of the ExposureEnd Event.
	PARAM_ID_SFNC_EVENTEXPOSUREENDFRAMEID 				= 0x00001C0A,		//R IInteger R - E Returns the unique Identifier of the Frame (or image) that generated the ExposureEnd Event.
	PARAM_ID_SFNC_EVENTERRORDATA 						= 0x00001C0B,		//R ICategory R - E Category that contains all the data features related to the Error Event.
	PARAM_ID_SFNC_EVENTERROR 							= 0x00001C0C,		//R IInteger R - E Returns the unique identifier of the Error type of Event.
	PARAM_ID_SFNC_EVENTERRORTIMESTAMP 					= 0x00001C0D,		//R IInteger R - E Returns the Timestamp of the Error Event.
	PARAM_ID_SFNC_EVENTERRORFRAMEID	 					= 0x00001C0E,		//R IInteger R - E If applicable, returns the unique Identifier of the Frame (or image) that generated the Error Event.
	PARAM_ID_SFNC_EVENTERRORCODE 						= 0x00001C0F,		//R IInteger R - E Returns an error code for the error(s) that happened.
	PARAM_ID_SFNC_EVENTTESTDATA 						= 0x00001C10,		//R ICategory R - E Category that contains all the data features related to the Event Test generated using the TestEventGenerate command.
	PARAM_ID_SFNC_EVENTTEST 							= 0x00001C11,		//R IInteger R - E Returns the unique identifier of the Event Test type of event generated using the TestEventGenerate command.
	PARAM_ID_SFNC_EVENTTESTTIMESTAMP 					= 0x00001C12,		//R IInteger R - E Returns the Timestamp of the Event Test event.

	//2.14User Set Control(Contains the features related to the User Set Control to save and load the user device settings)
	PARAM_ID_SFNC_USERSETSELECTOR 						= 0x00001D01,		//R IEnumeration R/W - B Selects the feature User Set to load, save or configure.
	PARAM_ID_SFNC_USERSETLOAD							= 0x00001D02,		//R ICommand (R)/W - B Loads the User Set specified by UserSetSelector to the device and makes it active.
	PARAM_ID_SFNC_USERSETSAVE							= 0x00001D03,		//R ICommand (R)/W - B Save the User Set specified by UserSetSelector to the non-volatile memory of the device.
	PARAM_ID_SFNC_USERSETDEFAULT 						= 0x00001D04,		//O IEnumeration R/W - B Selects the feature User Set to load and make active by default when the device is reset.
	PARAM_ID_SFNC_USERSETDEFAULTSELECTOR 				= 0x00001D05,		//O IEnumeration R/W - B This feature is deprecated (See UserSetDefault).
	PARAM_ID_SFNC_USERSETFEATURESELECTOR 				= 0x00001D06,		//R IEnumeration R/W - E Selects which individual UserSet feature to control.
	PARAM_ID_SFNC_USERSETFEATUREENABLE					= 0x00001D07,		//R IBoolean R/(W) - E Enables the selected feature and make it active in all the UserSets.

	//2.15 Sequencer Control(Contains the features related to the control of the Sequencer for features change)
	PARAM_ID_SFNC_SEQUENCERMODE 						= 0x00001E01,		//R IEnumeration R/W - E Controls if the sequencer mechanism is active.
	PARAM_ID_SFNC_SEQUENCERCONFIGURATIONMODE 			= 0x00001E02,		//R IEnumeration R/W - E Controls if the sequencer configuration mode is active.
	PARAM_ID_SFNC_SEQUENCERFEATURESELECTOR 				= 0x00001E03,		//R IEnumeration R/W - E Selects which sequencer features to control.
	PARAM_ID_SFNC_SEQUENCERFEATUREENABLE				= 0x00001E04,		//R IBoolean R/(W) - E Enables the selected feature and make it active in all the sequencer sets.
	PARAM_ID_SFNC_SEQUENCERSETSELECTOR 					= 0x00001E05,		//R IInteger R/W - E Selects the sequencer set to which further feature settings applies.
	PARAM_ID_SFNC_SEQUENCERSETSAVE						= 0x00001E06,		//R ICommand (R)/W - E Saves the current device state to the sequencer set selected by the SequencerSetSelector.
	PARAM_ID_SFNC_SEQUENCERSETLOAD						= 0x00001E07,		//R ICommand (R)/W - E Loads the sequencer set selected by SequencerSetSelector in the device.
	PARAM_ID_SFNC_SEQUENCERSETACTIVE 					= 0x00001E08,		//O IInteger R - E Contains the currently active sequencer set.
	PARAM_ID_SFNC_SEQUENCERSETSTART 					= 0x00001E09,		//R IInteger R/W - E Sets the initial/start sequencer set, which is the first set used within a sequencer.
	PARAM_ID_SFNC_SEQUENCERPATHSELECTOR					= 0x00001E0A,		//R IInteger R/W - E Selects to which branching path further path settings applies.
	PARAM_ID_SFNC_SEQUENCERSETNEXT 						= 0x00001E0B,		//R IInteger R/W - E Specifies the next sequencer set.
	PARAM_ID_SFNC_SEQUENCERTRIGGERSOURCE				= 0x00001E0C,		//R IEnumeration R/W - E Specifies the internal signal or physical input line to use as the sequencer trigger source.
	PARAM_ID_SFNC_SEQUENCERTRIGGERACTIVATION			= 0x00001E0D,		//R IEnumeration R/W - E Specifies the activation mode of the sequencer trigger.

	//2.16File Access Control(Contains the features related to the generic file access of a device)
	PARAM_ID_SFNC_FILESELECTOR 							= 0x00001F01,		//R IEnumeration R/(W) - G Selects the target file in the device.
	PARAM_ID_SFNC_FILEOPERATIONSELECTOR					= 0x00001F02,		//R IEnumeration R/W - G Selects the target operation for the selected file in the device.
	PARAM_ID_SFNC_FILEOPERATIONEXECUTE					= 0x00001F03,		//R ICommand (R)/W - G Executes the operation selected by FileOperationSelector on the selected file.
	PARAM_ID_SFNC_FILEOPENMODE							= 0x00001F04,		//R IEnumeration R/(W) - G Selects the access mode in which a file is opened in the device.
	PARAM_ID_SFNC_FILEACCESSBUFFER 						= 0x00001F05,		//R IRegister R/(W) - G Defines the intermediate access buffer that allows the exchange of data between the device file storage and the application.
	PARAM_ID_SFNC_FILEACCESSOFFSET						= 0x00001F06,		//R IInteger R/(W) B G Controls the Offset of the mapping between the device file storage and the FileAccessBuffer.
	PARAM_ID_SFNC_FILEACCESSLENGTH						= 0x00001F07,		//R IInteger R/W B G Controls the Length of the mapping between the device file storage and the FileAccessBuffer.
	PARAM_ID_SFNC_FILEOPERATIONSTATUS					= 0x00001F08,		//R IEnumeration R - G Represents the file operation execution status.
	PARAM_ID_SFNC_FILEOPERATIONRESULT					= 0x00001F09,		//R IInteger R - G Represents the file operation result.
	PARAM_ID_SFNC_FILESIZE								= 0x00001F0A,		//R IInteger R B G Represents the size of the selected file in bytes.
	
	//2.17Source Control(Contains the features related to the control of the multiple Source devices)
	PARAM_ID_SFNC_SOURCECOUNT 							= 0x00002001,		//O IInteger R/(W) - B Controls or returns the number of sources supported by the device.
	PARAM_ID_SFNC_SOURCESELECTOR						= 0x00002002,		//O IEnumeration R/W - B Selects the source to control.
	PARAM_ID_SFNC_SOURCEIDVALUE							= 0x00002003,		//O IInteger R - E Retuns a unique Identifier value that correspond to the selected Source.

	//2.18Transfer Control(Contains the features related to the control of the Transfers )
	PARAM_ID_SFNC_TRANSFERSELECTOR 						= 0x00002101,		//O IEnumeration R/(W) - E Selects which stream transfers are currently controlled by the selected Transfer features.
	PARAM_ID_SFNC_TRANSFERCONTROLMODE					= 0x00002102,		//R IEnumeration R/(W) - E Selects the control method for the transfers.
	PARAM_ID_SFNC_TRANSFEROPERATIONMODE					= 0x00002103,		//O IEnumeration R/(W) - E Selects the operation mode of the transfer.
	PARAM_ID_SFNC_TRANSFERBLOCKCOUNT					= 0x00002104,		//O IInteger R/(W) - E Specifies the number of data Blocks that the device should stream before stopping.
	PARAM_ID_SFNC_TRANSFERBURSTCOUNT 					= 0x00002105,		//O IInteger R/W - E Number of Block(s) to transfer for each TransferBurstStart trigger.
	PARAM_ID_SFNC_TRANSFERQUEUEMAXBLOCKCOUNT			= 0x00002106,		//O IInteger R/(W) - E Controls the maximum number of data blocks that can be stored in the block queue of the selected stream.
	PARAM_ID_SFNC_TRANSFERQUEUECURRENTBLOCKCOUNT		= 0x00002107,		//O IInteger R - E Returns the number of Block(s) currently in the transfer queue.
	PARAM_ID_SFNC_TRANSFERQUEUEMODE						= 0x00002108,		//O IEnumeration R/(W) - E Specifies the operation mode of the transfer queue.
	PARAM_ID_SFNC_TRANSFERSTART							= 0x00002109,		//O ICommand (R)/W - E Starts the streaming of data blocks out of the device.
	PARAM_ID_SFNC_TRANSFERSTOP							= 0x0000210A,		//O ICommand (R)/W - E Stops the streaming of data Block(s).
	PARAM_ID_SFNC_TRANSFERABORT							= 0x0000210B,		//O ICommand (R)/W - E Aborts immediately the streaming of data block(s).
	PARAM_ID_SFNC_TRANSFERPAUSE							= 0x0000210C,		//O ICommand (R)/W - G Pauses the streaming of data Block(s).
	PARAM_ID_SFNC_TRANSFERRESUME						= 0x0000210D,		//O ICommand (R)/W - G Resumes a data Blocks streaming that was previously paused by a TransferPause command.
	PARAM_ID_SFNC_TRANSFERTRIGGERSELECTOR				= 0x0000210E,		//O IEnumeration R/W - G Selects the type of transfer trigger to configure.
	PARAM_ID_SFNC_TRANSFERTRIGGERMODE					= 0x0000210F,		//R IEnumeration R/W - G Controls if the selected trigger is active.
	PARAM_ID_SFNC_TRANSFERTRIGGERSOURCE					= 0x00002110,		//O IEnumeration R/W - G Specifies the signal to use as the trigger source for transfers.
	PARAM_ID_SFNC_TRANSFERTRIGGERACTIVATION				= 0x00002111,		//O IEnumeration R/W - G Specifies the activation mode of the transfer control trigger.
	PARAM_ID_SFNC_TRANSFERSTATUSSELECTOR				= 0x00002112,		//R IEnumeration R/W - G Selects which status of the transfer module to read.
	PARAM_ID_SFNC_TRANSFERSTATUS						= 0x00002113,		//R IBool R - G Reads the status of the Transfer module signal selected by TransferStatusSelector.
	PARAM_ID_SFNC_TRANSFERCOMPONENTSELECTOR				= 0x00002114,		//O IEnumeration R/W - G Selects the color component for the control of the	TransferStreamChannel feature.
	PARAM_ID_SFNC_TRANSFERSTREAMCHANNEL					= 0x00002115,		//O IInteger R/W - G Selects the streaming channel that will be used to transfer the selected stream of data.

	//2.19Scan 3D Control(Contains the features related to the control of the 3D scan features)
	PARAM_ID_SFNC_SCAN3DEXTRACTIONSELECTOR 				= 0x00002201,		//O IEnumeration R/W - E Selects the 3DExtraction processing module to control (if multiple ones are present).
	PARAM_ID_SFNC_SCAN3DEXTRACTIONSOURCE 				= 0x00002202,		//O IEnumeration R/W - E Selects the sensor¡¯s data source region for 3D Extraction module.
	PARAM_ID_SFNC_SCAN3DEXTRACTIONMETHOD				= 0x00002203,		//O IEnumeration R/W - E Selects the method for extracting 3D from the input sensor data.
	PARAM_ID_SFNC_SCAN3DDISTANCEUNIT					= 0x00002204,		//O IEnumeration R/(W) - E Specifies the unit used when delivering (calibrated) distance data.
	PARAM_ID_SFNC_SCAN3DCOORDINATESYSTEM				= 0x00002205,		//O IEnumeration R/(W) - E Specifies the Coordinate system to use for the device.
	PARAM_ID_SFNC_SCAN3DOUTPUTMODE						= 0x00002206,		//O IEnumeration R/(W) - E Controls the Calibration and data organization of the device and the coordinates transmitted.
	PARAM_ID_SFNC_SCAN3DCOORDINATESYSTEMREFERENCE		= 0x00002207,		//O IEnumeration R/W - E Defines coordinate system reference location.
	PARAM_ID_SFNC_SCAN3DCOORDINATESELECTOR         		= 0x00002208,		//O IEnumeration R/W - E Selects the individual coordinates in the vectors for 3D information/transformation.
	PARAM_ID_SFNC_SCAN3DCOORDINATESCALE					= 0x00002209,		//O IFloat R/(W) - E Scale factor when transforming a pixel from relative coordinates to world coordinates.
	PARAM_ID_SFNC_SCAN3DCOORDINATEOFFSET				= 0x0000220A,		//O IFloat R/(W) - E Offset when transforming a pixel from relative coordinates to world coordinates.
	PARAM_ID_SFNC_SCAN3DINVALIDDATAFLAG					= 0x0000220B,		//O IBoolean R/(W) - E Enables the definition of a non-valid flag value in the data stream.
	PARAM_ID_SFNC_SCAN3DINVALIDDATAVALUE				= 0x0000220C,		//O IFloat R/(W) - E Value which identifies a non-valid pixel if Scan3dInvalidDataFlag is enabled.
	PARAM_ID_SFNC_SCAN3DAXISMIN							= 0x0000220D,		//O IFloat R - E Minimum valid transmitted coordinate value of the selected Axis.
	PARAM_ID_SFNC_SCAN3DAXISMAX							= 0x0000220E,		//O IFloat R - E Maximum valid transmitted coordinate value of the selected Axis.
	PARAM_ID_SFNC_SCAN3DCOORDINATETRANSFORMSELECTOR		= 0x0000220F,		//O IEnumeration R/W - E Sets the index to read/write a coordinate transform value.
	PARAM_ID_SFNC_SCAN3DTRANSFORMVALUE 					= 0x00002210,		//O IFloat R/W - E Specifies the transform value selected.
	PARAM_ID_SFNC_SCAN3DCOORDINATEREFERENCESELECTOR		= 0x00002211,		//O IEnumeration R/W - E Sets the index to read a coordinate system reference value defining the transform of a point from the current (Anchor or Transformed) system to the reference system.
	PARAM_ID_SFNC_SCAN3DCOORDINATEREFERENCEVALUE		= 0x00002212,		//O IFloat R - E Returns the reference value selected.

	//2.20Chunk Data Control(Contains the features related to the Chunk Data Control)
	PARAM_ID_SFNC_CHUNKMODEACTIVE 						= 0x00002301,		//R IBoolean R/W - E Activates the inclusion of Chunk data in the payload of the image.
	PARAM_ID_SFNC_CHUNKSELECTOR 						= 0x00002302,		//R IEnumeration R/W - E Selects which Chunk to enable or control.
	PARAM_ID_SFNC_CHUNKENABLE							= 0x00002303,		//R IBoolean R/W - E Enables the inclusion of the selected Chunk data in the payload of the image.
	PARAM_ID_SFNC_CHUNKCOMPONENTSELECTOR 				= 0x00002304,		//O IEnumeration R - E Selects the Component from which to retreive data from.
	PARAM_ID_SFNC_CHUNKCOMPONENTID 						= 0x00002305,		//O IEnumeration R - E Returns the Component Identifier of the selected Component image.
	PARAM_ID_SFNC_CHUNKCOMPONENTIDVALUE					= 0x00002306,		//O IInteger R - E Returns a unique Identifier value that correspond to the selected chunk Component.
	PARAM_ID_SFNC_CHUNKIMAGECOMPONENT 					= 0x00002307,		//O IEnumeration R - E This feature is deprecated (See ChunkComponentID).
	PARAM_ID_SFNC_CHUNKPARTSELECTOR 					= 0x00002308,		//O IInteger R/W - E This feature is deprecated (See ChunkComponentSelector).
	PARAM_ID_SFNC_CHUNKIMAGE 							= 0x00002309,		//R IRegister R - G Returns the entire image data included in the payload.
	PARAM_ID_SFNC_CHUNKOFFSETX 							= 0x0000230A,		//R IInteger R - E Returns the OffsetX of the image included in the payload.
	PARAM_ID_SFNC_CHUNKOFFSETY 							= 0x0000230B,		//R IInteger R - E Returns the OffsetY of the image included in the payload.
	PARAM_ID_SFNC_CHUNKWIDTH 							= 0x0000230C,		//R IInteger R - E Returns the Width of the image included in the payload.
	PARAM_ID_SFNC_CHUNKHEIGHT 							= 0x0000230D,		//R IInteger R - E Returns the Height of the image included in the payload.
	PARAM_ID_SFNC_CHUNKPIXELFORMAT 						= 0x0000230E,		//R IEnumeration R - E Returns the PixelFormat of the image included in the payload.
	PARAM_ID_SFNC_CHUNKPIXELDYNAMICRANGEMIN 			= 0x0000230F,		//R IInteger R - E Returns the minimum value of dynamic range of the image included in the payload.
	PARAM_ID_SFNC_CHUNKPIXELDYNAMICRANGEMAX 			= 0x00002310,		//R IInteger R - E Returns the maximum value of dynamic range of the image included in the payload.
	PARAM_ID_SFNC_CHUNKTIMESTAMP 						= 0x00002311,		//R IInteger R - E Returns the Timestamp of the image included in the payload at the time of the FrameStart internal event.
	PARAM_ID_SFNC_CHUNKTIMESTAMPLATCHVALUE 				= 0x00002312,		//R IInteger R ns E Returns the last Timestamp latched with the TimestampLatch command.
	PARAM_ID_SFNC_CHUNKLINESTATUSALL 					= 0x00002313,		//R IInteger R - E Returns the status of all the I/O lines at the time of the FrameStart internal event.
	PARAM_ID_SFNC_CHUNKCOUNTERSELECTOR 					= 0x00002314,		//R IEnumeration R/W - E Selects which counter to retrieve data from.
	PARAM_ID_SFNC_CHUNKCOUNTERVALUE						= 0x00002315,		//R IInteger R - E Returns the value of the selected Chunk counter at the time of the FrameStart event.
	PARAM_ID_SFNC_CHUNKTIMERSELECTOR 					= 0x00002316,		//R IEnumeration R/W - E Selects which Timer to retrieve data from.
	PARAM_ID_SFNC_CHUNKTIMERVALUE						= 0x00002317,		//R IFloat R us E Returns the value of the selected Timer at the time of the FrameStart internal event.
	PARAM_ID_SFNC_CHUNKSCANLINESELECTOR 				= 0x00002318,		//O IInteger R - E Index for vector representation of one chunk value per line in an image.
	PARAM_ID_SFNC_CHUNKENCODERSELECTOR 					= 0x00002319,		//R IEnumeration R/W - E Selects which Encoder to retrieve data from.
	PARAM_ID_SFNC_CHUNKENCODERVALUE						= 0x0000231A,		//R IInteger R - E Returns the counter¡¯s value of the selected Encoder at the time of the FrameStart in area scan mode or the counter¡¯s value at the time of the LineStart selected by ChunkScanLineSelector in Linescan mode.
	PARAM_ID_SFNC_CHUNKENCODERSTATUS					= 0x0000231B,		//O IEnumeration R - E Returns the motion status of the selected encoder.
	PARAM_ID_SFNC_CHUNKEXPOSURETIMESELECTOR 			= 0x0000231C,		//O IEnumeration R/W - E Selects which exposure time is read by the ChunkExposureTime feature.
	PARAM_ID_SFNC_CHUNKEXPOSURETIME						= 0x0000231D,		//R IFloat R us E Returns the exposure time used to capture the image.
	PARAM_ID_SFNC_CHUNKGAINSELECTOR 					= 0x0000231E,		//R IEnumeration R/W - E Selects which Gain to return.
	PARAM_ID_SFNC_CHUNKGAIN								= 0x0000231F,		//R IFloat R - E Returns the gain used to capture the image.
	PARAM_ID_SFNC_CHUNKBLACKLEVELSELECTOR 				= 0x00002320,		//R IEnumeration R/W - E Selects which Black Level to return.
	PARAM_ID_SFNC_CHUNKBLACKLEVEL						= 0x00002321,		//R IFloat R - E Returns the black level used to capture the image included in the payload.
	PARAM_ID_SFNC_CHUNKLINEPITCH 						= 0x00002322,		//R IInteger R B E Returns the LinePitch of the image included in the payload.
	PARAM_ID_SFNC_CHUNKFRAMEID 							= 0x00002323,		//R IInteger R - E Returns the unique Identifier of the frame (or image) included in the payload.
	PARAM_ID_SFNC_CHUNKSOURCESELECTOR 					= 0x00002324,		//O IEnumeration R - E Selects which Source to retrieve data from.
	PARAM_ID_SFNC_CHUNKSOURCEID 						= 0x00002325,		//O IEnumeration R - E Returns the Identifier of Source that the image comes from.
	PARAM_ID_SFNC_CHUNKSOURCEIDVALUE 					= 0x00002326,		//R IInteger R - E Returns the unique integer Identifier value of the Source that the image comes from.
	PARAM_ID_SFNC_CHUNKREGIONSELECTOR 					= 0x00002327,		//O IEnumeration R - E Selects which Region to retrieve data from.
	PARAM_ID_SFNC_CHUNKREGIONID 						= 0x00002328,		//O IEnumeration R - E Returns the Identifier of Region that the image comes from.
	PARAM_ID_SFNC_CHUNKREGIONIDVALUE 					= 0x00002329,		//R IInteger R - E Returns the unique integer Identifier value of the Region that the image comes from.
	PARAM_ID_SFNC_CHUNKTRANSFERBLOCKID 					= 0x0000232A,		//R IInteger R - E Returns the unique identifier of the transfer block used to transport the payload.
	PARAM_ID_SFNC_CHUNKTRANSFERSTREAMID 				= 0x0000232B,		//R IEnumeration R - E Returns identifier of the stream that generated this block.
	PARAM_ID_SFNC_CHUNKTRANSFERQUEUECURRENTBLOCKCOUNT	= 0x0000232C,		//O IInteger R - E Returns the current number of blocks in the transfer queue.
	PARAM_ID_SFNC_CHUNKSTREAMCHANNELID 					= 0x0000232D,		//R IInteger R - E Returns identifier of the stream channel used to carry the block.
	PARAM_ID_SFNC_CHUNKSEQUENCERSETACTIVE 				= 0x0000232E,		//R IInteger R - E Return the index of the active set of the running sequencer included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DDISTANCEUNIT	 			= 0x0000232F,		//O IEnumeration R - E Returns the Distance Unit of the payload image.
	PARAM_ID_SFNC_CHUNKSCAN3DOUTPUTMODE 				= 0x00002330,		//O IEnumeration R - E Returns the Calibrated Mode of the payload image.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATESYSTEM 			= 0x00002331,		//O IEnumeration R - E Returns the Coordinate System of the image included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATESYSTEMREFERENCE	= 0x00002332,		//O IEnumeration R - E Returns the Coordinate System Position of the image included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATESELECTOR 		= 0x00002333,		//O IEnumeration R - E Selects which Coordinate to retrieve data from.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATESCALE			= 0x00002334,		//O IFloat R - E Returns the Scale for the selected coordinate axis of the image included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATEOFFSET 			= 0x00002335,		//O IFloat R - E Returns the Offset for the selected coordinate axis of the image included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DINVALIDDATAFLAG     		= 0x00002336,		//O IBoolean R - E Returns if a specific non-valid data flag is used in the data stream.
	PARAM_ID_SFNC_CHUNKSCAN3DINVALIDDATAVALUE			= 0x00002337,		//O IFloat R - E Returns the Invalid Data Value used for the image included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DAXISMIN					= 0x00002338,		//O IFloat R - E Returns the Minimum Axis value for the selected coordinate axis of the image included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DAXISMAX					= 0x00002339,		//O IFloat R - E Returns the Maximum Axis value for the selected coordinate axis of the image included in the payload.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATETRANSFORMSELECTOR= 0x0000233A,		//O IEnumeration R/W - E Selector for transform values.
	PARAM_ID_SFNC_CHUNKSCAN3DTRANSFORMVALUE				= 0x0000233B,		//O IFloat R - E Returns the transform value.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATEREFERENCESELECTOR= 0x0000233C,		//O IEnumeration R/W - E Selector to read a coordinate system reference value defining the transform of a point from one system to the other.
	PARAM_ID_SFNC_CHUNKSCAN3DCOORDINATEREFERENCEVALUE	= 0x0000233D,		//O IFloat R - E Reads the value of a position or pose coordinate for the anchor or transformed coordinate systems relative to the reference point.


	//2.21Test Control(Contains the features related to the control of the test features )
	PARAM_ID_SFNC_TESTPENDINGACK 						= 0x00002401,		//O IInteger R/W ms G Tests the device's pending acknowledge feature.
	PARAM_ID_SFNC_TESTEVENTGENERATE 					= 0x00002402,		//O ICommand (R)/W - G Generates a Test Event.

	//2.22GenICam Control(Contains the features related to GenICam control and access)
	PARAM_ID_SFNC_TLPARAMSLOCKED 						= 0x00002501,		//M IInteger R/W - I Used by the Transport Layer to prevent critical features from changing during acquisition.

	//2.23Transport Layer Control(Contains the features related to the Transport Layer Control)
	PARAM_ID_SFNC_PAYLOADSIZE 							= 0x00002601,		//R IInteger R B E Provides the number of bytes transferred for each image or chunk on the stream channel.
	PARAM_ID_SFNC_DEVICETAPGEOMETRY 					= 0x00002602,		//R IEnumeration R/(W) - E This device tap geometry feature describes the geometrical properties characterizing the taps of a camera as presented at the output of the device.
	PARAM_ID_SFNC_GEVVERSIONMAJOR 						= 0x00002603,		//R IInteger R - E This feature is deprecated (See DeviceTLVersionMajor).
	PARAM_ID_SFNC_GEVVERSIONMINOR 						= 0x00002604,		//R IInteger R - E This feature is deprecated (See DeviceTLVersionMinor).
	PARAM_ID_SFNC_GEVDEVICEMODEISBIGENDIAN 				= 0x00002605,		//O IBoolean R - G This feature is deprecated (See DeviceRegistersEndianness).
	PARAM_ID_SFNC_GEVDEVICECLASS 						= 0x00002606,		//O IEnumeration R - G This feature is deprecated (See DeviceType).
	PARAM_ID_SFNC_GEVDEVICEMODECHARACTERSET 			= 0x00002607,		//O IEnumeration R - G This feature is deprecated (See DeviceCharacterSet).
	PARAM_ID_SFNC_GEVPHYSICALLINKCONFIGURATION 			= 0x00002608,		//O IEnumeration R/W - E Controls the principal physical link configuration to use on next restart/power-up of the device.
	PARAM_ID_SFNC_GEVCURRENTPHYSICALLINKCONFIGURATION	= 0x00002609,		//O IEnumeration R - E Indicates the current physical link configuration of the device.
	PARAM_ID_SFNC_GEVACTIVELINKCOUNT 					= 0x0000260A,		//O IInteger R - E Indicates the current number of active logical links.
	PARAM_ID_SFNC_GEVSUPPORTEDOPTIONSELECTOR 			= 0x0000260B,		//O IEnumeration R/W - E Selects the GEV option to interrogate for existing support.
	PARAM_ID_SFNC_GEVSUPPORTEDOPTION					= 0x0000260C,		//O IBoolean R - E Returns if the selected GEV option is supported.
	PARAM_ID_SFNC_GEVINTERFACESELECTOR 					= 0x0000260D,		//O IInteger R/W - B Selects which logical link to control.
	PARAM_ID_SFNC_GEVLINKSPEED							= 0x0000260E,		//O IInteger R Mbs E This feature is deprecated (See DeviceLinkSpeed).
	PARAM_ID_SFNC_GEVMACADDRESS							= 0x0000260F,		//O IInteger R - B MAC address of the logical link.
	PARAM_ID_SFNC_GEVPAUSEFRAMERECEPTION				= 0x00002610,		//O IBoolean R/(W) - E Controls whether incoming PAUSE Frames are handled on the given logical link.
	PARAM_ID_SFNC_GEVPAUSEFRAMETRANSMISSION				= 0x00002611,		//O IBoolean R/(W) - E Controls whether PAUSE Frames can be generated on the given logical link.
	PARAM_ID_SFNC_GEVCURRENTIPCONFIGURATIONLLA			= 0x00002612,		//O IBoolean R/W - B Controls whether the Link Local Address IP configuration scheme is activated on the given logical link.
	PARAM_ID_SFNC_GEVCURRENTIPCONFIGURATIONDHCP			= 0x00002613,		//O IBoolean R/W - B Controls whether the DHCP IP configuration scheme is activated on the given logical link.
	PARAM_ID_SFNC_GEVCURRENTIPCONFIGURATIONPERSISTENTIP	= 0x00002614,		//O IBoolean R/W - B Controls whether the PersistentIP configuration scheme is activated on the given logical link.
	PARAM_ID_SFNC_GEVCURRENTIPADDRESS					= 0x00002615,		//O IInteger R - B Reports the IP address for the given logical link.
	PARAM_ID_SFNC_GEVCURRENTSUBNETMASK					= 0x00002616,		//O IInteger R - B Reports the subnet mask of the given logical link.
	PARAM_ID_SFNC_GEVCURRENTDEFAULTGATEWAY				= 0x00002617,		//O IInteger R - B Reports the default gateway IP address to be used on the given logical link.
	PARAM_ID_SFNC_GEVIPCONFIGURATIONSTATUS				= 0x00002618,		//O IEnumeration R - B Reports the current IP configuration status.
	PARAM_ID_SFNC_GEVFIRSTURL 							= 0x00002619,		//O IString R - G Indicates the first URL to the GenICam XML device description file.
	PARAM_ID_SFNC_GEVSECONDURL 							= 0x0000261A,		//O IString R - G Indicates the second URL to the GenICam XML device description file.
	PARAM_ID_SFNC_GEVNUMBEROFINTERFACES 				= 0x0000261B,		//O IInteger R - E This feature is deprecated (See DeviceLinkSelector).
	PARAM_ID_SFNC_GEVPERSISTENTIPADDRESS				= 0x0000261C,		//O IInteger R/W - B Controls the Persistent IP address for this logical link.
	PARAM_ID_SFNC_GEVPERSISTENTSUBNETMASK				= 0x0000261D,		//O IInteger R/W - B Controls the Persistent subnet mask associated with the Persistent IP address on this logical link.
	PARAM_ID_SFNC_GEVPERSISTENTDEFAULTGATEWAY			= 0x0000261E,		//O IInteger R/W - B Controls the persistent default gateway for this logical link.
	PARAM_ID_SFNC_GEVMESSAGECHANNELCOUNT 				= 0x0000261F,		//O IInteger R - E This feature is deprecated (See DeviceEventChannelCount).
	PARAM_ID_SFNC_GEVSTREAMCHANNELCOUNT 				= 0x00002620,		//O IInteger R - E This feature is deprecated (See DeviceStreamChannelCount).
	PARAM_ID_SFNC_GEVHEARTBEATTIMEOUT 					= 0x00002621,		//O IInteger R/W ms G This feature is deprecated (See DeviceLinkHeartbeatTimeout).
	PARAM_ID_SFNC_GEVTIMESTAMPTICKFREQUENCY 			= 0x00002622,		//O IInteger R Hz E This feature is deprecated (See the increment of the TimestampLatchValue feature).
	PARAM_ID_SFNC_GEVTIMESTAMPCONTROLLATCH 				= 0x00002623,		//O ICommand W - E This feature is deprecated (See TimestampLatch).
	PARAM_ID_SFNC_GEVTIMESTAMPCONTROLRESET 				= 0x00002624,		//O ICommand W - E This feature is deprecated (See TimestampReset).
	PARAM_ID_SFNC_GEVTIMESTAMPVALUE 					= 0x00002625,		//O IInteger R E This feature is deprecated (See TimestampLatchValue).
	PARAM_ID_SFNC_GEVDISCOVERYACKDELAY 					= 0x00002626,		//O IInteger R/(W) ms E Indicates the maximum randomized delay the device will wait to acknowledge a discovery command.
	PARAM_ID_SFNC_GEVIEEE1588 							= 0x00002627,		//O IBoolean R/W - E Enables the IEEE 1588 Precision Time Protocol to control the timestamp register.
	PARAM_ID_SFNC_GEVIEEE1588CLOCKACCURACY 				= 0x00002628,		//O IEnumeration R/(W) - E Indicates the expected accuracy of the device clock when it is the grandmaster, or in the event it becomes the grandmaster.
	PARAM_ID_SFNC_GEVIEEE1588STATUS 					= 0x00002629,		//O IEnumeration R - E Provides the status of the IEEE 1588 clock.
	PARAM_ID_SFNC_GEVGVCPEXTENDEDSTATUSCODESSELECTOR	= 0x0000262A,		//O IEnumeration R/W - G Selects the GigE Vision version to control extended status codes for.
	PARAM_ID_SFNC_GEVGVCPEXTENDEDSTATUSCODES			= 0x0000262B,		//O IBoolean R/W - G Enables the generation of extended status codes.
	PARAM_ID_SFNC_GEVGVCPPENDINGACK						= 0x0000262C,		//O IBoolean R/W - G Enables the generation of PENDING_ACK.
	PARAM_ID_SFNC_GEVGVCPHEARTBEATDISABLE 				= 0x0000262D,		//O IBoolean R/W - E This feature is deprecated (See DeviceHeartbeatMode).
	PARAM_ID_SFNC_GEVGVCPPENDINGTIMEOUT 				= 0x0000262E,		//O IInteger R - G This feature is deprecated (See DeviceLinkCommandTimeout).
	PARAM_ID_SFNC_GEVPRIMARYAPPLICATIONSWITCHOVERKEY	= 0x0000262F,		//O IInteger W-O - G Controls the key to use to authenticate primary application switchover requests.
	PARAM_ID_SFNC_GEVGVSPEXTENDEDIDMODE 				= 0x00002630,		//O IEnumeration R/(W) - E Enables the extended IDs mode.
	PARAM_ID_SFNC_GEVCCP 								= 0x00002631,		//O IEnumeration R/W - G Controls the device access privilege of an application.
	PARAM_ID_SFNC_GEVPRIMARYAPPLICATIONSOCKET 			= 0x00002632,		//O IInteger R - G Returns the UDP source port of the primary application.
	PARAM_ID_SFNC_GEVPRIMARYAPPLICATIONIPADDRESS 		= 0x00002633,		//O IInteger R - G Returns the address of the primary application.
	PARAM_ID_SFNC_GEVMCPHOSTPORT 						= 0x00002634,		//O IInteger R/W - G Controls the port to which the device must send messages.
	PARAM_ID_SFNC_GEVMCDA 								= 0x00002635,		//O IInteger R/W - G Controls the destination IP address for the message channel.
	PARAM_ID_SFNC_GEVMCTT 								= 0x00002636,		//O IInteger R/W ms G Provides the transmission timeout value in milliseconds.
	PARAM_ID_SFNC_GEVMCRC 								= 0x00002637,		//O IInteger R/W - G Controls the number of retransmissions allowed when a message channel message times out.
	PARAM_ID_SFNC_GEVMCSP 								= 0x00002638,		//O IInteger R - G This feature indicates the source port for the message channel.
	PARAM_ID_SFNC_GEVSTREAMCHANNELSELECTOR 				= 0x0000263A,		//O IInteger R/W - E Selects the stream channel to control.
	PARAM_ID_SFNC_GEVSCCFGPACKETRESENDDESTINATION		= 0x0000263B,		//O IBoolean R/W - G Enables the alternate IP destination for stream packets resent due to a packet resend request.
	PARAM_ID_SFNC_GEVSCCFGALLINTRANSMISSION				= 0x0000263C,		//O IBoolean R/W - G Enables the selected GVSP transmitter to use the single packet per data block All-in Transmission mode.
	PARAM_ID_SFNC_GEVSCCFGUNCONDITIONALSTREAMING		= 0x0000263D,		//O IBoolean R/W - G Enables the camera to continue to stream, for this stream channel, if its control channel is closed or regardless of the reception of any ICMP messages (such as destination unreachable messages).
	PARAM_ID_SFNC_GEVSCCFGEXTENDEDCHUNKDATA				= 0x0000263E,		//O IBoolean R/W - G Enables cameras to use the extended chunk data payload type for this stream channel.
	PARAM_ID_SFNC_GEVSCPDIRECTION						= 0x0000263F,		//O IEnumeration R - G This feature is deprecated (See DeviceStreamChannelType).
	PARAM_ID_SFNC_GEVSCPINTERFACEINDEX					= 0x00002640,		//O IInteger R/(W) - G Index of the logical link to use.
	PARAM_ID_SFNC_GEVSCPHOSTPORT						= 0x00002641,		//O IInteger R/W - G Controls the port of the selected channel to which a GVSP transmitter must send data stream or the port from which a GVSP receiver may receive data stream.
	PARAM_ID_SFNC_GEVSCPSFIRETESTPACKET					= 0x00002642,		//O IBoolean R/W - G Sends a test packet.
	PARAM_ID_SFNC_GEVSCPSDONOTFRAGMENT					= 0x00002643,		//O IBoolean R/W - G The state of this feature is copied into the "do not fragment" bit of IP header of each stream packet.
	PARAM_ID_SFNC_GEVSCPSBIGENDIAN						= 0x00002644,		//O IBoolean R/W - G This feature is deprecated (See DeviceStreamChannelEndianness).
	PARAM_ID_SFNC_GEVSCPSPACKETSIZE						= 0x00002645,		//R IInteger R/(W) B E This GigE Vision specific feature corresponds to DeviceStreamChannelPacketSize and should be kept in sync with it.
	PARAM_ID_SFNC_GEVSCPD								= 0x00002646,		//R IInteger R/W E Controls the delay (in GEV timestamp counter unit) to insert between each packet for this stream channel.
	PARAM_ID_SFNC_GEVSCDA								= 0x00002647,		//O IInteger R/W - G Controls the destination IP address of the selected stream channel to which a GVSP transmitter must send data stream or the destination IP address from which a GVSP receiver may receive data stream.
	PARAM_ID_SFNC_GEVSCSP								= 0x00002648,		//O IInteger R - G Indicates the source port of the stream channel.
	PARAM_ID_SFNC_GEVSCZONECOUNT						= 0x00002649,		//O IInteger R - G Reports the number of zones per block transmitted on the selected stream channel.
	PARAM_ID_SFNC_GEVSCZONEDIRECTIONALL					= 0x0000264A,		//O IInteger R - G Reports the transmission direction of each zone transmitted on the selected stream channel.
	PARAM_ID_SFNC_GEVSCZONECONFIGURATIONLOCK			= 0x0000264B,		//O IBoolean R/W - G Controls whether the selected stream channel multi-zone configuration is locked.

	//Sensor Param
	PARAM_ID_SENSOR_OFFSETH								= 0x00003000,		//R IInteger R/W - E First column of visible pixels to be read out(starting Horizontal value).
	PARAM_ID_SENSOR_OFFSETV								= 0x00003001,		//R IInteger R/W - E First row of visible pixels to be read out(starting Vertical value).
	PARAM_ID_SENSOR_WIDTH								= 0x00003002,		//R IInteger R/W - E Visible pixels to be read out(Window Width).
	PARAM_ID_SENSOR_HEIGHT								= 0x00003003,		//R IInteger R/W - E Visible pixels to be read out(Window Height).
	PARAM_ID_SENSOR_BLANKINGH							= 0x00003004,		//R IInteger R/W - E Horizontal Blanking.
	PARAM_ID_SENSOR_BLANKINGV							= 0x00003005,		//R IInteger R/W - E Vertical Blanking.
	
	PARAM_ID_SENSOR_EXPOSURETIMERAW						= 0x00003010,		//R IInteger R/W - E Total integration time in number of rows.
	PARAM_ID_SENSOR_EXPOSUREAUTOENABLE					= 0x00003011,		//R IBoolean R/W - E Automatic Exposure Control(AEC).

	PARAM_ID_SENSOR_GAINANALOGRAW						= 0x00003020,		//R IInteger R/W - E Analog Gain
	PARAM_ID_SENSOR_GAINANALOGAUTOENABLE				= 0x00003021,		//R IBoolean R/W - E Automatic gain control (AGC).
	PARAM_ID_SENSOR_GAINANALOGAGCMAX					= 0x00003022,		//R IInteger R/W - E Automatic gain control (AGC) as the upper threshold of gain(Maximum Analog Gain).

	PARAM_ID_SENSOR_GAINDIGITRAW						= 0x0000302A,		//R IInteger R/W - E Digital Gain
	PARAM_ID_SENSOR_GAINDIGITAUTOENABLE					= 0x0000302B,		//R IBoolean R/W - E Automatic Exposure Control.
	PARAM_ID_SENSOR_GAINDIGITAGCMAX						= 0x0000302C,		//R IInteger R/W - E Automatic gain control (AGC) as the upper threshold of gain(Maximum Digital Gain).
	 
	PARAM_ID_SENSOR_BITLUT								= 0x00003040,		//R IEnumeration R/W - E Sensor Bit Output Mode

	//I/O Control Param
	PARAM_ID_TRIGGER_DEBOUNCE							= 0x00003103,		//R IInteger R/W - E Trigger Debounce pulse width
	PARAM_ID_TRIGGER_WIDTH								= 0x00003104,		//R IInteger R/W - E Trigger Pulse Width
	PARAM_ID_TRIGGER_INTERVAL							= 0x00003106,		//R IInteger R/W - E Specifies the interval in microseconds (us) to apply between twice trigger
	PARAM_ID_TRIGGER_POLARITY							= 0x00003107,		//R IBoolean R/W - E Trigger Polarity

	PARAM_ID_STROBE_ENABLE								= 0x00003110,		//R IBoolean R/W - E STROBE Signal Output Enable
	PARAM_ID_STROBE_WIDTH								= 0x00003111,		//R IInteger R/W - E STROBE Signal Pulse Width
	PARAM_ID_STROBE_DELAY								= 0x00003112,		//R IInteger R/W - E STROBE Signal Output Delay
	PARAM_ID_STROBE_POLARITY							= 0x00003113,		//R IBoolean R/W - E STROBE Signal Polarity

	//Image Process Unit
	PARAM_ID_IPU_BRIGHTNESS								= 0x00003100,		//R IInteger R/W - E HW/SW Brightness Image Processing
	PARAM_ID_IPU_CONTRAST								= 0x00003101,		//R IInteger R/W - E HW/SW Contrast Image Processing


	//Image Stream Control Param
	PARAM_ID_GRAB_UNDEFINE								= 0x00005000,		//UNDEFINE 
	PARAM_ID_GRAB_FRAMERATE								= 0x00005001,
	PARAM_ID_GRAB_BANDWIDTH								= 0x00005002,
	PARAM_ID_GRAB_FRAMECOUNT							= 0x00005003,

	PARAM_ID_GRAB_PACKETRESENT							= 0x00005010,
	PARAM_ID_GRAB_PACKETLOST							= 0x00005011,
	PARAM_ID_GRAB_IMAGEERROR							= 0x00005012,
	PARAM_ID_GRAB_IMAGEDROPPED							= 0x00005013,

	PARAM_ID_GRAB_BUFFERNUM								= 0x00005100,
	PARAM_ID_GRAB_BUFFERMODE							= 0x00005101,
	PARAM_ID_GRAB_BUFFERRESET							= 0x00005102,

};


typedef enum PARAM_VALUETYPE
{
	VALUE_INT      = 0, //Integer Type (INT,BOOL,ENUM)
	VALUE_FLOAT    = 1, //Float Type (FLOAT)
	VALUE_STRING   = 2  //String Type (STRING,ENUM)
};

typedef enum PARAM_CLASSTYPE
{
	CLASS_UDEF     = 0, //Param Undefined
	CLASS_INT      = 1, //Param Integer Type    
	CLASS_ENUM     = 2, //Param Enumeration Type    
	CLASS_BOOL     = 3, //Param Boolean Type    
	CLASS_FLOAT    = 4, //Param Float Type (100.0)    
	CLASS_STRING   = 5, //Param String Type
	CLASS_COMMAND  = 6, //Param Command Type
	CLASS_REGISTER = 7, //Param Register Type
};

typedef enum PARAM_VALIDTYPE
{
	VALID_NA     = 0, //Cannot R/W
	VALID_RO     = 1, //Read Only
	VALID_WO     = 2, //Write Only
	VALID_RW     = 3  //Read & Write
};

typedef enum PARAM_VISIBLETYPE
{
	VISIBLE_NA        = 0, //Not Open to public
	VISIBLE_BIGINNER  = 1, //Open to Beginner
	VISIBLE_EXPERT    = 2, //Open to Expert
	VISIBLE_GURU      = 3  //Open to Guru
};

typedef struct _HQV_PARAM_INFO_INT_
{
	INT lMin;								//Param Minimum Value
	INT lMax;								//Param Maximum Value
	INT lInc;								//Param Increment Value
	CHAR lUnit[BUF_SIZE];					//Param Unit
	INT lValInt;							//Param Integer Value
}HQV_PARAM_INFO_INT,*PHQV_PARAM_INFO_INT;

typedef struct _HQV_PARAM_INFO_ENUM_UNIT_
{
	INT lParamIndex;						//Param Index
	INT lParamValue;						//Param Value
	CHAR lParamName[BUF_SIZE];				//Param Name
	CHAR lDescription[BUF_SIZE];			//Param Description
	PARAM_VALIDTYPE lValid;					//Param IsValid
	PARAM_VISIBLETYPE lVisible;				//Param IsVisible
}HQV_PARAM_INFO_ENUM_UNIT,*PHQV_PARAM_INFO_ENUM_UNIT;

typedef struct _HQV_PARAM_INFO_ENUM_
{
	INT lParamEnumUnitNumber;							//Param count
	HQV_PARAM_INFO_ENUM_UNIT lParamEnumUnit[MAX_ENUM];	//Param Group
	INT lParamValEnum;									//Enum Value by int
	CHAR lParamValEnumStr[BUF_SIZE];					//Enum Value by string
}HQV_PARAM_INFO_ENUM,*PHQV_PARAM_INFO_ENUM;

typedef struct _HQV_PARAM_INFO_FLOAT_
{
	FLOAT lMin;								//Param Minimum Value
	FLOAT lMax;								//Param Maximum Value
	FLOAT lInc;								//Param Increment
	CHAR lUnit[BUF_SIZE];					//Param Unit
	FLOAT lValFloat;						//Param Float Value
}HQV_PARAM_INFO_FLOAT,*PHQV_PARAM_INFO_FLOAT;

typedef struct _HQV_PARAM_INFO_BOOL_
{
	BOOL lValBool;							//Param Bool Value
}HQV_PARAM_INFO_BOOL,*PHQV_PARAM_INFO_BOOL;

typedef struct _HQV_PARAM_INFO_STRING_
{
	CHAR lValString[BUF_SIZE];				//Param String Value
}HQV_PARAM_INFO_STRING,*PHQV_PARAM_INFO_STRING;

typedef struct _HQV_PARAM_INFO_COMMAND_
{
	BOOL lIsDone;
}HQV_PARAM_INFO_COMMAND,*PHQV_PARAM_INFO_COMMAND;

typedef struct _HQV_PARAM_INFO_REGISTER_
{
	DWORD lLength;							//Param Length
	DWORD lValReg;							//Register Value by unsigned int
}HQV_PARAM_INFO_REGISTER,*PHQV_PARAM_INFO_REGISTER;

typedef struct _HQV_DEVPARAM_
{
	BOOL bUseName;
	HQV_DEVPARAM_ID lParamByID;
	CHAR lParamByName[BUF_SIZE];
	_HQV_DEVPARAM_()
	{
		bUseName = FALSE;
		lParamByID = PARAM_ID_UNDEFINE;
		strcpy_s(lParamByName,"");
	}
	_HQV_DEVPARAM_(HQV_DEVPARAM_ID y0)
	{
		bUseName = FALSE;
		lParamByID = y0;
		strcpy_s(lParamByName,"");
	}
	_HQV_DEVPARAM_(PCHAR y0)
	{
		bUseName = TRUE;
		strcpy_s(lParamByName,y0);
		lParamByID = PARAM_ID_UNDEFINE;
	}
}HQV_DEVPARAM,*PHQV_DEVPARAM;

typedef struct _HQV_DEVPARAMINFO_
{
	CHAR lParamName[BUF_SIZE];
	CHAR lCategory[BUF_SIZE];
	CHAR lDescription[BUF_SIZE];
	HQV_DEVPARAM_ID lParamByID;
	PARAM_CLASSTYPE lClass;
	PARAM_VALIDTYPE lValid;
	PARAM_VISIBLETYPE lVisible;
	union
	{
		PHQV_PARAM_INFO_INT pParamINT;
		PHQV_PARAM_INFO_ENUM pParamENUM;
		PHQV_PARAM_INFO_BOOL pParamBOOL;
		PHQV_PARAM_INFO_FLOAT pParamFLOAT;
		PHQV_PARAM_INFO_STRING pParamSTRING;
		PHQV_PARAM_INFO_COMMAND pParamCOMMAND;
		PHQV_PARAM_INFO_REGISTER pParamREGISTER;
	};
}HQV_DEVPARAMINFO,*PHQV_DEVPARAMINFO;


//2.1Device Control Summary(Contains the features related to the control and information of the device)
#define PARAM_NAME_SFNC_DEVICETYPE 								"DeviceType"									//O IEnumeration R - G Returns the device type.
#define PARAM_NAME_SFNC_DEVICESCANTYPE							"DeviceScanType"								//R IEnumeration R/(W) - E Scan type of the sensor of the device.
#define PARAM_NAME_SFNC_DEVICEVENDORNAME						"DeviceVendorName"								//R IString R - B Name of the manufacturer of the device.
#define PARAM_NAME_SFNC_DEVICEMODELNAME 						"DeviceModelName" 								//R IString R - B Model of the device.
#define PARAM_NAME_SFNC_DEVICEFAMILYNAME						"DeviceFamilyName"								//O IString R - B Identifier of the product family of the device.
#define PARAM_NAME_SFNC_DEVICEMANUFACTURERINFO 					"DeviceManufacturerInfo" 						//R IString R - B Manufacturer information about the device.
#define PARAM_NAME_SFNC_DEVICEVERSION 							"DeviceVersion" 								//R IString R - B Version of the device.
#define PARAM_NAME_SFNC_DEVICEFIRMWAREVERSION 					"DeviceFirmwareVersion" 						//R IString R - B Version of the firmware in the device.
#define PARAM_NAME_SFNC_DEVICESERIALNUMBER 						"DeviceSerialNumber" 							//R IString R - E Device's serial number.
#define PARAM_NAME_SFNC_DEVICEID  								"DeviceID"  									//R IString R - I This feature is deprecated (See DeviceSerialNumber).
#define PARAM_NAME_SFNC_DEVICEUSERID 							"DeviceUserID" 									//O IString R/W - B User-programmable device identifier.
#define PARAM_NAME_SFNC_DEVICESFNCVERSIONMAJOR 					"DeviceSFNCVersionMajor" 						//R IInteger R - B Major version of the Standard Features Naming Convention that was used to create the device's GenICam XML.
#define PARAM_NAME_SFNC_DEVICESFNCVERSIONMINOR 					"DeviceSFNCVersionMinor" 						//R IInteger R - B Minor version of the Standard Features Naming Convention that was used to create the device's GenICam XML.
#define PARAM_NAME_SFNC_DEVICESFNCVERSIONSUBMINOR 				"DeviceSFNCVersionSubMinor" 					//R IInteger R - B Sub minor version of Standard Features Naming Convention that was used to create the device's GenICam XML.
#define PARAM_NAME_SFNC_DEVICEMANIFESTENTRYSELECTOR			    "DeviceManifestEntrySelector"					//O IInteger R/W - G Selects the manifest entry to reference.
#define PARAM_NAME_SFNC_DEVICEMANIFESTXMLMAJORVERSION		    "DeviceManifestXMLMajorVersion"					//O IInteger R - G Indicates the major version number of the GenICam XML file of the selected manifest entry.
#define PARAM_NAME_SFNC_DEVICEMANIFESTXMLMINORVERSION		    "DeviceManifestXMLMinorVersion"					//O IInteger R - G Indicates the minor version number of the GenICam XML file of the selected manifest entry.
#define PARAM_NAME_SFNC_DEVICEMANIFESTXMLSUBMINORVERSION		"DeviceManifestXMLSubMinorVersion"				//O IInteger R - G Indicates the subminor version number of the GenICam XML file of the selected manifest entry.
#define PARAM_NAME_SFNC_DEVICEMANIFESTSCHEMAMAJORVERSION		"DeviceManifestSchemaMajorVersion"				//O IInteger R - G Indicates the major version number of the schema file of the selected manifest entry.
#define PARAM_NAME_SFNC_DEVICEMANIFESTSCHEMAMINORVERSION		"DeviceManifestSchemaMinorVersion" 				//O IInteger R - G Indicates the minor version number of the schema file of the selected manifest entry.
#define PARAM_NAME_SFNC_DEVICEMANIFESTPRIMARYURL				"DeviceManifestPrimaryURL"						//O IString R - G Indicates the first URL to the GenICam XML device description file of the selected manifest entry.
#define PARAM_NAME_SFNC_DEVICEMANIFESTSECONDARYURL 			    "DeviceManifestSecondaryURL" 					//O IString R - G Indicates the second URL to the GenICam XML device description file of the selected manifest entry.
#define PARAM_NAME_SFNC_DEVICETLTYPE							"DeviceTLType"									//R IEnumeration R - B Transport Layer type of the device.
#define PARAM_NAME_SFNC_DEVICETLVERSIONMAJOR 					"DeviceTLVersionMajor" 							//R IInteger R - B Major version of the Transport Layer of the device.
#define PARAM_NAME_SFNC_DEVICETLVERSIONMINOR 					"DeviceTLVersionMinor" 							//R IInteger R - B Minor version of the Transport Layer of the device.
#define PARAM_NAME_SFNC_DEVICETLVERSIONSUBMINOR 				"DeviceTLVersionSubMinor" 						//R IInteger R - B Sub minor version of the Transport Layer of the device.
#define PARAM_NAME_SFNC_DEVICEGENCPVERSIONMAJOR 				"DeviceGenCPVersionMajor" 						//R IInteger R - B Major version of the GenCP protocol supported by the device.
#define PARAM_NAME_SFNC_DEVICEGENCPVERSIONMINOR 				"DeviceGenCPVersionMinor" 						//R IInteger R - B Minor version of the GenCP protocol supported by the device.
#define PARAM_NAME_SFNC_DEVICEMAXTHROUGHPUT 					"DeviceMaxThroughput" 							//O IInteger R Bps E Maximum bandwidth of the data that can be streamed out of the device.
#define PARAM_NAME_SFNC_DEVICECONNECTIONSELECTOR 				"DeviceConnectionSelector" 						//R IInteger R/(W) - B Selects which Connection of the device to control.
#define PARAM_NAME_SFNC_DEVICECONNECTIONSPEED					"DeviceConnectionSpeed"							//O IInteger R Bps E Indicates the speed of transmission of the specified Connection.
#define PARAM_NAME_SFNC_DEVICECONNECTIONSTATUS					"DeviceConnectionStatus"						//O IEnumeration R - E Indicates the status of the specified Connection.
#define PARAM_NAME_SFNC_DEVICELINKSELECTOR 						"DeviceLinkSelector" 							//R IInteger R/(W) - B Selects which Link of the device to control.
#define PARAM_NAME_SFNC_DEVICELINKSPEED							"DeviceLinkSpeed"								//O IInteger R Bps E Indicates the speed of transmission negotiated on the specified Link.
#define PARAM_NAME_SFNC_DEVICELINKTHROUGHPUTLIMITMODE		    "DeviceLinkThroughputLimitMode"					//R IEnumeration R/W - E Controls if the DeviceLinkThroughputLimit is active.
#define PARAM_NAME_SFNC_DEVICELINKTHROUGHPUTLIMIT				"DeviceLinkThroughputLimit"						//R IInteger R/(W) Bps E Limits the maximum bandwidth of the data that will be streamed out by the device on the selected Link.
#define PARAM_NAME_SFNC_DEVICELINKCONNECTIONCOUNT				"DeviceLinkConnectionCount"						//O IInteger R - B Returns the number of physical connection of the device used by a particular Link.
#define PARAM_NAME_SFNC_DEVICELINKHEARTBEATMODE					"DeviceLinkHeartbeatMode"						//O IEnumeration R/W - E Activate or deactivate the Link's heartbeat.
#define PARAM_NAME_SFNC_DEVICELINKHEARTBEATTIMEOUT				"DeviceLinkHeartbeatTimeout"					//O IFloat R/W us G Controls the current heartbeat timeout of the specific Link.
#define PARAM_NAME_SFNC_DEVICELINKCOMMANDTIMEOUT				"DeviceLinkCommandTimeout"						//O IFloat R us G Indicates the command timeout of the specified Link.
#define PARAM_NAME_SFNC_DEVICESTREAMCHANNELCOUNT 				"DeviceStreamChannelCount" 						//O IInteger R - E Indicates the number of streaming channels supported by the device.
#define PARAM_NAME_SFNC_DEVICESTREAMCHANNELSELECTOR 		    "DeviceStreamChannelSelector" 					//O IInteger R/W - E Selects the stream channel to control.
#define PARAM_NAME_SFNC_DEVICESTREAMCHANNELTYPE					"DeviceStreamChannelType"						//O IEnumeration R - G Reports the type of the stream channel.
#define PARAM_NAME_SFNC_DEVICESTREAMCHANNELLINK					"DeviceStreamChannelLink"						//O IInteger R/(W) - G Index of device's Link to use for streaming the specifed stream channel.
#define PARAM_NAME_SFNC_DEVICESTREAMCHANNELENDIANNESS		    "DeviceStreamChannelEndianness"					//O IEnumeration R/(W) - G Endianness of multi-byte pixel data for this stream.
#define PARAM_NAME_SFNC_DEVICESTREAMCHANNELPACKETSIZE		    "DeviceStreamChannelPacketSize"					//R IInteger R/(W) B E Specifies the stream packet size, in bytes, to send on the selected channel for a Transmitter or specifies the maximum packet size supported by a receiver.
#define PARAM_NAME_SFNC_DEVICEEVENTCHANNELCOUNT 				"DeviceEventChannelCount" 						//O IInteger R - E Indicates the number of event channels supported by the device.
#define PARAM_NAME_SFNC_DEVICEMESSAGECHANNELCOUNT 				"DeviceMessageChannelCount" 					//O IInteger R - E This feature is deprecated (See DeviceEventChannelCount).
#define PARAM_NAME_SFNC_DEVICECHARACTERSET 						"DeviceCharacterSet" 							//O IEnumeration R - G Character set used by the strings of the device's bootstrap registers.
#define PARAM_NAME_SFNC_DEVICERESET 							"DeviceReset" 									//R ICommand W - G Resets the device to its power up state.
#define PARAM_NAME_SFNC_DEVICEINDICATORMODE 					"DeviceIndicatorMode" 							//O IEnumeration R/W - E Controls the behavior of the indicators (such as LEDs) showing the status of the Device.
#define PARAM_NAME_SFNC_DEVICEFEATUREPERSISTENCESTART 		    "DeviceFeaturePersistenceStart" 				//O ICommand (R)/W - G Indicate to the device and GenICam XML to get ready for persisting of all streamable features.
#define PARAM_NAME_SFNC_DEVICEFEATUREPERSISTENCEEND 		    "DeviceFeaturePersistenceEnd" 					//O ICommand (R)/W - G Indicate to the device the end of feature persistence.
#define PARAM_NAME_SFNC_DEVICEREGISTERSSTREAMINGSTART 		    "DeviceRegistersStreamingStart" 				//R ICommand (R)/W - G Prepare the device for registers streaming without checking for consistency.
#define PARAM_NAME_SFNC_DEVICEREGISTERSSTREAMINGEND 		    "DeviceRegistersStreamingEnd" 					//R ICommand (R)/W - G Announce the end of registers streaming.
#define PARAM_NAME_SFNC_DEVICEREGISTERSCHECK 					"DeviceRegistersCheck" 							//R ICommand (R)/W - E Perform the validation of the current register set for consistency.
#define PARAM_NAME_SFNC_DEVICEREGISTERSVALID 					"DeviceRegistersValid" 							//R IBoolean R - E Returns if the current register set is valid and consistent.
#define PARAM_NAME_SFNC_DEVICEREGISTERSENDIANNESS 				"DeviceRegistersEndianness" 					//O IEnumeration R/(W) - G Endianness of the registers of the device.
#define PARAM_NAME_SFNC_DEVICETEMPERATURESELECTOR 				"DeviceTemperatureSelector" 					//O IEnumeration R/W - E Selects the location within the device, where the temperature will be measured.
#define PARAM_NAME_SFNC_DEVICETEMPERATURE						"DeviceTemperature"								//O IFloat R C E Device temperature in degrees Celsius (C).
#define PARAM_NAME_SFNC_DEVICECLOCKSELECTOR 					"DeviceClockSelector" 							//O IEnumeration R/(W) - E Selects the clock frequency to access from the device.
#define PARAM_NAME_SFNC_DEVICECLOCKFREQUENCY					"DeviceClockFrequency"							//O IFloat R/(W) Hz E Returns the frequency of the selected Clock.
#define PARAM_NAME_SFNC_DEVICESERIALPORTSELECTOR 				"DeviceSerialPortSelector" 						//R IEnumeration R/(W) - E Selects which serial port of the device to control.
#define PARAM_NAME_SFNC_DEVICESERIALPORTBAUDRATE				"DeviceSerialPortBaudRate"						//R IEnumeration R/(W) - E This feature controls the baud rate used by the selected serial port.
#define PARAM_NAME_SFNC_TIMESTAMP 								"Timestamp" 									//R IInteger R ns E Reports the current value of the device timestamp counter.
#define PARAM_NAME_SFNC_TIMESTAMPRESET 							"TimestampReset" 								//O ICommand (R)/W - E Resets the current value of the device timestamp counter.
#define PARAM_NAME_SFNC_TIMESTAMPLATCH 							"TimestampLatch" 								//O ICommand (R)/W - E Latches the current timestamp counter into TimestampLatchValue.
#define PARAM_NAME_SFNC_TIMESTAMPLATCHVALUE 					"TimestampLatchValue" 							//O IInteger R ns E Returns the latched value of the timestamp counter.

//2.2Image Format Control(Contains the features related to the format of the transmitted image)
#define PARAM_NAME_SFNC_SENSORWIDTH 							"SensorWidth" 									//R IInteger R - E Effective width of the sensor in pixels.
#define PARAM_NAME_SFNC_SENSORHEIGHT 							"SensorHeight" 									//R IInteger R - E Effective height of the sensor in pixels.
#define PARAM_NAME_SFNC_SENSORSHUTTERMODE 						"SensorShutterMode" 							//O IEnumeration R/(W) - G Sets the shutter mode of the device.
#define PARAM_NAME_SFNC_SENSORTAPS 								"SensorTaps" 									//O IEnumeration R/(W) - E Number of taps of the camera sensor.
#define PARAM_NAME_SFNC_SENSORDIGITIZATIONTAPS 					"SensorDigitizationTaps" 						//O IEnumeration R/(W) - E Number of digitized samples outputted simultaneously by the camera A/D conversion stage.
#define PARAM_NAME_SFNC_WIDTHMAX 								"WidthMax" 										//R IInteger R - E Maximum width of the image (in pixels).
#define PARAM_NAME_SFNC_HEIGHTMAX 								"HeightMax" 									//R IInteger R - E Maximum height of the image (in pixels).
#define PARAM_NAME_SFNC_REGIONSELECTOR 							"RegionSelector" 								//O IEnumeration R/(W) - B Selects the Region of interest to control.
#define PARAM_NAME_SFNC_REGIONMODE								"RegionMode"									//O IEnumeration R/W - B Controls if the selected Region of interest is active and streaming.
#define PARAM_NAME_SFNC_REGIONDESTINATION						"RegionDestination"								//O IEnumeration R/(W) - E Control the destination of the selected region.
#define PARAM_NAME_SFNC_REGIONIDVALUE							"RegionIDValue"									//O IInteger R - E Returns a unique Identifier value that correspond to the selected Region.
#define PARAM_NAME_SFNC_COMPONENTSELECTOR 						"ComponentSelector" 							//O IEnumeration R/W - B Selects a component to activate/deactivate its data streaming.
#define PARAM_NAME_SFNC_COMPONENTENABLE							"ComponentEnable"								//O IBoolean R/(W) - B Controls if the selected component streaming is active.
#define PARAM_NAME_SFNC_COMPONENTIDVALUE						"ComponentIDValue"								//O IInteger R - E Returns a unique Identifier value that correspond to the selected Component type.
#define PARAM_NAME_SFNC_IMAGECOMPONENTSELECTOR 					"ImageComponentSelector" 						//O IEnumeration R/W - B This feature is deprecated (See ComponentSelector).
#define PARAM_NAME_SFNC_IMAGECOMPONENTENABLE					"ImageComponentEnable"							//O IBoolean R/(W) - B This feature is deprecated (See ComponentEnable).
#define PARAM_NAME_SFNC_WIDTH									"Width"											//R IInteger R/(W) - B Width of the image provided by the device (in pixels).
#define PARAM_NAME_SFNC_HEIGHT									"Height"										//R IInteger R/(W) - B Height of the image provided by the device (in pixels).
#define PARAM_NAME_SFNC_OFFSETX									"OffsetX"										//R IInteger R/W - B Horizontal offset from the origin to the region of interest (in pixels).
#define PARAM_NAME_SFNC_OFFSETY									"OffsetY"										//R IInteger R/W - B Vertical offset from the origin to the region of interest (in pixels).
#define PARAM_NAME_SFNC_LINEPITCHENABLE							"LinePitchEnable"								//R IBoolean R/W - E This feature controls whether the LinePitch feature is writable.
#define PARAM_NAME_SFNC_LINEPITCH								"LinePitch"										//R IInteger R/W B E Total number of bytes between the starts of 2 consecutive lines.
#define PARAM_NAME_SFNC_BINNINGSELECTOR 						"BinningSelector" 								//O IEnumeration R/(W) - E Selects which binning engine is controlled by the BinningHorizontal and BinningVertical features.
#define PARAM_NAME_SFNC_BINNINGHORIZONTALMODE					"BinningHorizontalMode"							//O IEnumeration R/(W) - E Sets the mode to use to combine horizontal photo-sensitive cells together when BinningHorizontal is used.
#define PARAM_NAME_SFNC_BINNINGHORIZONTAL						"BinningHorizontal"								//O IInteger R/W - E Number of horizontal photo-sensitive cells to combine together.
#define PARAM_NAME_SFNC_BINNINGVERTICALMODE						"BinningVerticalMode"							//O IEnumeration R/(W) - E Sets the mode used to combine horizontal photo-sensitive cells together when BinningVertical is used.
#define PARAM_NAME_SFNC_BINNINGVERTICAL							"BinningVertical"								//O IInteger R/W - E Number of vertical photo-sensitive cells to combine together.
#define PARAM_NAME_SFNC_DECIMATIONHORIZONTALMODE 				"DecimationHorizontalMode" 						//O IEnumeration R/(W) - E Sets the mode used to reduce the horizontal resolution when DecimationHorizontal is used.
#define PARAM_NAME_SFNC_DECIMATIONHORIZONTAL 					"DecimationHorizontal" 							//O IInteger R/W - E Horizontal sub-sampling of the image.
#define PARAM_NAME_SFNC_DECIMATIONVERTICALMODE 					"DecimationVerticalMode" 						//O IEnumeration R/(W) - E Sets the mode used to reduce the Vertical resolution when DecimationVertical is used.
#define PARAM_NAME_SFNC_DECIMATIONVERTICAL 						"DecimationVertical" 							//O IInteger R/W - E Vertical sub-sampling of the image.
#define PARAM_NAME_SFNC_REVERSEX 								"ReverseX" 										//R IBoolean R/W - E Flip horizontally the image sent by the device.
#define PARAM_NAME_SFNC_REVERSEY 								"ReverseY" 										//R IBoolean R/W - E Flip vertically the image sent by the device.
#define PARAM_NAME_SFNC_PIXELFORMAT								"PixelFormat"									//R IEnumeration R/(W) - B Format of the pixels provided by the device.
#define PARAM_NAME_SFNC_PIXELFORMATINFOSELECTOR 				"PixelFormatInfoSelector" 						//R IEnumeration R/W - G Select the pixel format for which the information will be returned
#define PARAM_NAME_SFNC_PIXELFORMATINFOID						"PixelFormatInfoID"								//R IInteger R - G Returns the value used by the streaming channels to identify the selected pixel format.
#define PARAM_NAME_SFNC_PIXELCODING 							"PixelCoding" 									//R IEnumeration R/(W) - E This feature is deprecated.
#define PARAM_NAME_SFNC_PIXELSIZE								"PixelSize"										//R IEnumeration R/(W) - E Total size in bits of a pixel of the image.
#define PARAM_NAME_SFNC_PIXELCOLORFILTER						"PixelColorFilter"								//R IEnumeration R/(W) - E Type of color filter that is applied to the image.
#define PARAM_NAME_SFNC_PIXELDYNAMICRANGEMIN					"PixelDynamicRangeMin"							//O IInteger R/W - E Minimum value that can be returned during the digitization process.
#define PARAM_NAME_SFNC_PIXELDYNAMICRANGEMAX					"PixelDynamicRangeMax"							//O IInteger R/W - E Maximum value that will be returned during the digitization process.
#define PARAM_NAME_SFNC_TESTPATTERNGENERATORSELECTOR 			"TestPatternGeneratorSelector" 					//O IEnumeration R/(W) - B Selects which test pattern generator is controlled by the TestPattern feature.
#define PARAM_NAME_SFNC_TESTPATTERN								"TestPattern"									//O IEnumeration R/W - B Selects the type of test pattern that is generated by the device as image source.
#define PARAM_NAME_SFNC_TESTIMAGESELECTOR 						"TestImageSelector" 							//O IEnumeration R/W - I This feature is deprecated (See TestPattern).
#define PARAM_NAME_SFNC_DEINTERLACING 							"Deinterlacing" 								//O IEnumeration R/W - B Controls how the device performs de-interlacing.
#define PARAM_NAME_SFNC_IMAGECOMPRESSIONMODE 					"ImageCompressionMode" 							//O IEnumeration R/W - B Enable a specific image compression mode as the base mode for image transfer.
#define PARAM_NAME_SFNC_IMAGECOMPRESSIONRATEOPTION 				"ImageCompressionRateOption" 					//O IEnumeration R/W - E Two rate controlling options are offered: fixed bit rate or fixed quality.
#define PARAM_NAME_SFNC_IMAGECOMPRESSIONQUALITY 				"ImageCompressionQuality" 						//O IInteger R/(W) - E Control the quality of the produced compressed stream.
#define PARAM_NAME_SFNC_IMAGECOMPRESSIONBITRATE 				"ImageCompressionBitrate" 						//O IFloat R/(W) Mbps E Control the rate of the produced compressed stream.
#define PARAM_NAME_SFNC_IMAGECOMPRESSIONJPEGFORMATOPTION		"ImageCompressionJPEGFormatOption" 				//O IEnumeration R/W - E When JPEG is selected as the compression format, a device might optionally offer better control over JPEG-specific options through this feature.
             
//2.3 Acquisition Control(Contains the features related to image acquisition, including trigger and exposure control )
#define PARAM_NAME_SFNC_ACQUISITIONMODE 						"AcquisitionMode" 								//R IEnumeration R/(W) - B Sets the acquisition mode of the device.
#define PARAM_NAME_SFNC_ACQUISITIONSTART 						"AcquisitionStart" 								//R ICommand (R)/W - B Starts the Acquisition of the device.
#define PARAM_NAME_SFNC_ACQUISITIONSTOP 						"AcquisitionStop" 								//R ICommand (R)/W - B Stops the Acquisition of the device at the end of the current Frame.
#define PARAM_NAME_SFNC_ACQUISITIONABORT 						"AcquisitionAbort" 								//R ICommand (R)/W - E Aborts the Acquisition immediately.
#define PARAM_NAME_SFNC_ACQUISITIONARM 							"AcquisitionArm" 								//O ICommand (R)/W - E Arms the device before an AcquisitionStart command.
#define PARAM_NAME_SFNC_ACQUISITIONFRAMECOUNT 					"AcquisitionFrameCount" 						//R IInteger R/W - B Number of frames to acquire in MultiFrame Acquisition mode.
#define PARAM_NAME_SFNC_ACQUISITIONBURSTFRAMECOUNT 				"AcquisitionBurstFrameCount" 					//O IInteger R/W - B Number of frames to acquire for each FrameBurstStart trigger.
#define PARAM_NAME_SFNC_ACQUISITIONFRAMERATE 					"AcquisitionFrameRate" 							//R IFloat R/W Hz B Controls the acquisition rate (in Hertz) at which the frames are captured.
#define PARAM_NAME_SFNC_ACQUISITIONFRAMERATEENABLE 				"AcquisitionFrameRateEnable" 					//R IBoolean R/W - E Controls if the AcquisitionFrameRate feature is writable and used to control the acquisition rate.
#define PARAM_NAME_SFNC_ACQUISITIONLINERATE 					"AcquisitionLineRate" 							//R IFloat R/W Hz B Controls the rate (in Hertz) at which the Lines in a Frame are captured.
#define PARAM_NAME_SFNC_ACQUISITIONLINERATEENABLE 				"AcquisitionLineRateEnable" 					//R IBoolean R/W - E Controls if the AcquisitionLineRate feature is writable and used to control the acquisition rate.
#define PARAM_NAME_SFNC_ACQUISITIONSTATUSSELECTOR 				"AcquisitionStatusSelector" 					//R IEnumeration R/W - E Selects the internal acquisition signal to read using AcquisitionStatus.
#define PARAM_NAME_SFNC_ACQUISITIONSTATUS						"AcquisitionStatus"								//R IBoolean R - E Reads the state of the internal acquisition signal selected using AcquisitionStatusSelector.
#define PARAM_NAME_SFNC_TRIGGERSELECTOR 						"TriggerSelector" 								//R IEnumeration R/W - B Selects the type of trigger to configure.
#define PARAM_NAME_SFNC_TRIGGERMODE								"TriggerMode"									//R IEnumeration R/W - B Controls if the selected trigger is active.
#define PARAM_NAME_SFNC_TRIGGERSOFTWARE							"TriggerSoftware"								//R ICommand (R)/W - B Generates an internal trigger.
#define PARAM_NAME_SFNC_TRIGGERSOURCE							"TriggerSource"									//R IEnumeration R/W - B Specifies the internal signal or physical input Line to use as the trigger source.
#define PARAM_NAME_SFNC_TRIGGERACTIVATION						"TriggerActivation"								//R IEnumeration R/W - B Specifies the activation mode of the trigger.
#define PARAM_NAME_SFNC_TRIGGEROVERLAP							"TriggerOverlap"								//R IEnumeration R/W - E Specifies the type trigger overlap permitted with the previous frame or line.
#define PARAM_NAME_SFNC_TRIGGERDELAY							"TriggerDelay"									//R IFloat R/W us E Specifies the delay in microseconds (us) to apply after the trigger reception before activating it.
#define PARAM_NAME_SFNC_TRIGGERDIVIDER							"TriggerDivider"								//R IInteger R/W - E Specifies a division factor for the incoming trigger pulses.
#define PARAM_NAME_SFNC_TRIGGERMULTIPLIER						"TriggerMultiplier"								//R IInteger R/W - E Specifies a multiplication factor for the incoming trigger pulses.
#define PARAM_NAME_SFNC_EXPOSUREMODE 							"ExposureMode" 									//R IEnumeration R/W - B Sets the operation mode of the Exposure.
#define PARAM_NAME_SFNC_EXPOSURETIMEMODE 						"ExposureTimeMode" 								//O IEnumeration R/W - B Sets the configuration mode of the ExposureTime feature.
#define PARAM_NAME_SFNC_EXPOSURETIMESELECTOR 					"ExposureTimeSelector" 							//O IEnumeration R/W - B Selects which exposure time is controlled by the ExposureTime feature.
#define PARAM_NAME_SFNC_EXPOSURETIME							"ExposureTime"									//R IFloat R/W us B Sets the Exposure time when ExposureMode is Timed and ExposureAuto is Off.
#define PARAM_NAME_SFNC_EXPOSUREAUTO 							"ExposureAuto" 									//O IEnumeration R/W - B Sets the automatic exposure mode when ExposureMode is Timed.
#define PARAM_NAME_SFNC_MULTISLOPEMODE 							"MultiSlopeMode" 								//O IEnumeration R/W - B Controls multi-slope exposure state.
#define PARAM_NAME_SFNC_MULTISLOPEKNEEPOINTCOUNT 				"MultiSlopeKneePointCount" 						//O IInteger R/W - E The number of knee-points as well as the number of additional exposure slopes used for multi-slope exposure.
#define PARAM_NAME_SFNC_MULTISLOPEKNEEPOINTSELECTOR 			"MultiSlopeKneePointSelector" 					//O IInteger R/W - E Selects the parameters for controlling an additional slope in multi-slope exposure.
#define PARAM_NAME_SFNC_MULTISLOPEEXPOSURELIMIT					"MultiSlopeExposureLimit"						//O IFloat R/W % E Percent of the ExposureTime at a certain knee-point of multi-slope exposure.
#define PARAM_NAME_SFNC_MULTISLOPESATURATIONTHRESHOLD			"MultiSlopeSaturationThreshold"    				//O IFloat R/(W) % E The percentage of the full saturation that is applied at a certain kneepoint of a multi-slope exposure.
#define PARAM_NAME_SFNC_MULTISLOPEINTENSITYLIMIT				"MultiSlopeIntensityLimit"						//O IFloat R/(W) % E The relative intensity which divides intensities influenced by different exposure slopes.
#define PARAM_NAME_SFNC_MULTISLOPEEXPOSUREGRADIENT				"MultiSlopeExposureGradient"					//O IFloat R/(W) - E The gradient of the additional slope that is defined by this knee-point.

//2.4 Analog Control(Contains the features related to the video signal conditioning in the analog domain)
#define PARAM_NAME_SFNC_GAINSELECTOR 							"GainSelector" 									//O IEnumeration R/W - B Selects which Gain is controlled by the various Gain features.
#define PARAM_NAME_SFNC_GAIN									"Gain"											//O IFloat R/W - B Controls the selected gain as an absolute physical value.
#define PARAM_NAME_SFNC_GAINAUTO								"GainAuto"										//O IEnumeration R/W - B Sets the automatic gain control (AGC) mode.
#define PARAM_NAME_SFNC_GAINAUTOBALANCE 						"GainAutoBalance" 								//O IEnumeration R/W - B Sets the mode for automatic gain balancing between the sensor color channels or taps.
#define PARAM_NAME_SFNC_BLACKLEVELSELECTOR 						"BlackLevelSelector" 							//O IEnumeration R/W - E Selects which Black Level is controlled by the various Black Level features.
#define PARAM_NAME_SFNC_BLACKLEVEL								"BlackLevel"									//O IFloat R/W - E Controls the analog black level as an absolute physical value.
#define PARAM_NAME_SFNC_BLACKLEVELAUTO							"BlackLevelAuto"								//O IEnumeration R/W - E Controls the mode for automatic black level adjustment.
#define PARAM_NAME_SFNC_BLACKLEVELAUTOBALANCE 					"BlackLevelAutoBalance" 						//O IEnumeration R/W - E Controls the mode for automatic black level balancing between the sensor color channels or taps.
#define PARAM_NAME_SFNC_WHITECLIPSELECTOR 						"WhiteClipSelector" 							//O IEnumeration R/W - E Selects which White Clip to control.
#define PARAM_NAME_SFNC_WHITECLIP								"WhiteClip"										//O IFloat R/W - E Controls the maximal intensity taken by the video signal before being clipped as an absolute physical value.
#define PARAM_NAME_SFNC_BALANCERATIOSELECTOR 					"BalanceRatioSelector" 							//O IEnumeration R/W - E Selects which Balance ratio to control.
#define PARAM_NAME_SFNC_BALANCERATIO							"BalanceRatio"									//O IFloat R/W - E Controls ratio of the selected color component to a reference color component.
#define PARAM_NAME_SFNC_BALANCEWHITEAUTO 						"BalanceWhiteAuto" 								//O IEnumeration R/W - E Controls the mode for automatic white balancing between the color channels.
#define PARAM_NAME_SFNC_GAMMA 									"Gamma" 										//O IFloat R/W - B Controls the gamma correction of pixel intensity.
             
//2.5 LUT Control(Contains the features related to the look-up table (LUT) control)
#define PARAM_NAME_SFNC_LUTSELECTOR 							"LUTSelector" 									//O IEnumeration R/W - E Selects which LUT to control.
#define PARAM_NAME_SFNC_LUTENABLE								"LUTEnable"										//O IBoolean R/W - E Activates the selected LUT.
#define PARAM_NAME_SFNC_LUTINDEX								"LUTIndex"										//O IInteger R/W - G Control the index (offset) of the coefficient to access in the selected LUT.
#define PARAM_NAME_SFNC_LUTVALUE								"LUTValue"										//O IInteger R/W - G Returns the Value at entry LUTIndex of the LUT selected by LUTSelector.
#define PARAM_NAME_SFNC_LUTVALUEALL								"LUTValueAll"									//O IRegister R/W - G Accesses all the LUT coefficients in a single access without using individual LUTIndex.

//2.6 Color Transformation Control(Contains the features related to the control of the color transformation)
#define PARAM_NAME_SFNC_COLORTRANSFORMATIONSELECTOR 			"ColorTransformationSelector" 					//O IEnumeration R/W - E Selects which Color Transformation module is controlled by the various Color Transformation features.
#define PARAM_NAME_SFNC_COLORTRANSFORMATIONENABLE				"ColorTransformationEnable"						//O IBoolean R/W - E Activates the selected Color Transformation module.
#define PARAM_NAME_SFNC_COLORTRANSFORMATIONVALUESELECTOR		"ColorTransformationValueSelector"				//O IEnumeration R/W - E Selects the Gain factor or Offset of the Transformation matrix to access in the selected Color Transformation module.
#define PARAM_NAME_SFNC_COLORTRANSFORMATIONVALUE				"ColorTransformationValue"						//O IFloat R/W - E Represents the value of the selected Gain factor or Offset inside the Transformation matrix.
             
//2.7 Digital I/O Control(Contains the features related to the control of the input and output pins of the device )
#define PARAM_NAME_SFNC_LINESELECTOR 							"LineSelector" 									//R IEnumeration R/W - E Selects the physical line (or pin) of the external device connector or the virtual line of the Transport Layer to configure.
#define PARAM_NAME_SFNC_LINEMODE								"LineMode"										//O IEnumeration R/W - E Controls if the physical Line is used to Input or Output a signal.
#define PARAM_NAME_SFNC_LINEINVERTER							"LineInverter"									//R IBoolean R/W - E Controls the inversion of the signal of the selected input or output Line.
#define PARAM_NAME_SFNC_LINESTATUS								"LineStatus"									//R IBoolean R - E Returns the current status of the selected input or output Line.
#define PARAM_NAME_SFNC_LINESTATUSALL 							"LineStatusAll" 								//O IInteger R - E Returns the current status of all available Line signals at time of polling in a single bitfield.
#define PARAM_NAME_SFNC_LINESOURCE								"LineSource"									//R IEnumeration R/W - E Selects which internal acquisition or I/O source signal to output on the selected Line.
#define PARAM_NAME_SFNC_LINEFORMAT								"LineFormat"									//O IEnumeration R/W - E Controls the current electrical format of the selected physical input or output Line.
#define PARAM_NAME_SFNC_USEROUTPUTSELECTOR 						"UserOutputSelector" 							//R IEnumeration R/W - E Selects which bit of the User Output register will be set by UserOutputValue.
#define PARAM_NAME_SFNC_USEROUTPUTVALUE							"UserOutputValue"								//R IBoolean R/W - E Sets the value of the bit selected by UserOutputSelector.
#define PARAM_NAME_SFNC_USEROUTPUTVALUEALL 						"UserOutputValueAll" 							//O IInteger R/W - E Sets the value of all the bits of the User Output register.
#define PARAM_NAME_SFNC_USEROUTPUTVALUEALLMASK 					"UserOutputValueAllMask" 						//O IInteger R/W - E Sets the write mask to apply to the value specified by UserOutputValueAll before writing it in the User Output register.

//2.8 Counter and Timer Control(Contains the features related to the usage of programmable counters and timers)
#define PARAM_NAME_SFNC_COUNTERSELECTOR 						"CounterSelector" 								//R IEnumeration R/W - E Selects which Counter to configure.
#define PARAM_NAME_SFNC_COUNTEREVENTSOURCE						"CounterEventSource"							//R IEnumeration R/W - E Select the events that will be the source to increment the Counter.
#define PARAM_NAME_SFNC_COUNTEREVENTACTIVATION					"CounterEventActivation"						//R IEnumeration R/W - E Selects the Activation mode Event Source signal.
#define PARAM_NAME_SFNC_COUNTERRESETSOURCE						"CounterResetSource"							//R IEnumeration R/W - E Selects the signals that will be the source to reset the Counter.
#define PARAM_NAME_SFNC_COUNTERRESETACTIVATION					"CounterResetActivation"						//R IEnumeration R/W - E Selects the Activation mode of the Counter Reset Source signal.
#define PARAM_NAME_SFNC_COUNTERRESET							"CounterReset"									//R ICommand (R)/W - E Does a software reset of the selected Counter and starts it.
#define PARAM_NAME_SFNC_COUNTERVALUE							"CounterValue"									//R IInteger R/W - E Reads or writes the current value of the selected Counter.
#define PARAM_NAME_SFNC_COUNTERVALUEATRESET						"CounterValueAtReset"							//R IInteger R - E Reads the value of the selected Counter when it was reset by a trigger or by an explicit CounterReset command.
#define PARAM_NAME_SFNC_COUNTERDURATION							"CounterDuration"								//R IInteger R/W - E Sets the duration (or number of events) before the CounterEnd event is generated.
#define PARAM_NAME_SFNC_COUNTERSTATUS							"CounterStatus"									//R IEnumeration R - E Returns the current status of the Counter.
#define PARAM_NAME_SFNC_COUNTERTRIGGERSOURCE					"CounterTriggerSource"							//R IEnumeration R/W - E Selects the source to start the Counter.
#define PARAM_NAME_SFNC_COUNTERTRIGGERACTIVATION				"CounterTriggerActivation"						//R IEnumeration R/W - E Selects the activation mode of the trigger to start the Counter.
#define PARAM_NAME_SFNC_TIMERSELECTOR 							"TimerSelector" 								//R IEnumeration R/W - E Selects which Timer to configure.
#define PARAM_NAME_SFNC_TIMERDURATION							"TimerDuration"									//R IFloat R/W us E Sets the duration (in microseconds) of the Timer pulse.
#define PARAM_NAME_SFNC_TIMERDELAY								"TimerDelay"									//R IFloat R/W us E Sets the duration (in microseconds) of the delay to apply at the reception of a trigger before starting the Timer.
#define PARAM_NAME_SFNC_TIMERRESET								"TimerReset"									//R ICommand (R)/W - E Does a software reset of the selected timer and starts it.
#define PARAM_NAME_SFNC_TIMERVALUE								"TimerValue"									//R IFloat R/W us E Reads or writes the current value (in microseconds) of the selected Timer.
#define PARAM_NAME_SFNC_TIMERSTATUS								"TimerStatus"									//R IEnumeration R - E Returns the current status of the Timer.
#define PARAM_NAME_SFNC_TIMERTRIGGERSOURCE						"TimerTriggerSource"							//R IEnumeration R/W - E Selects the source of the trigger to start the Timer.
#define PARAM_NAME_SFNC_TIMERTRIGGERACTIVATION					"TimerTriggerActivation"						//R IEnumeration R/W - E Selects the activation mode of the trigger to start the Timer.

//2.9 Encoder Control(Contains the features related to the usage of quadrature encoders )
#define PARAM_NAME_SFNC_ENCODERSELECTOR 						"EncoderSelector" 								//O IEnumeration R/W - E Selects which Encoder to configure.
#define PARAM_NAME_SFNC_ENCODERSOURCEA							"EncoderSourceA"								//O IEnumeration R/W - E Selects the signal which will be the source of the A input of the Encoder.
#define PARAM_NAME_SFNC_ENCODERSOURCEB							"EncoderSourceB"								//O IEnumeration R/W - E Selects the signal which will be the source of the B input of the Encoder.
#define PARAM_NAME_SFNC_ENCODERMODE								"EncoderMode"									//O IEnumeration R/W - E Selects if the count of encoder uses FourPhase mode with jitter filtering or the HighResolution mode without jitter filtering.
#define PARAM_NAME_SFNC_ENCODERDIVIDER							"EncoderDivider"								//O IInteger R/W - E Sets how many Encoder increment/decrements that are needed generate an Encoder output pulse signal.
#define PARAM_NAME_SFNC_ENCODEROUTPUTMODE						"EncoderOutputMode"								//O IEnumeration R/W - E Selects the conditions for the Encoder interface to generate a valid Encoder output signal.
#define PARAM_NAME_SFNC_ENCODERSTATUS							"EncoderStatus"									//O IEnumeration R - E Returns the motion status of the encoder.
#define PARAM_NAME_SFNC_ENCODERTIMEOUT							"EncoderTimeout"								//O IFloat R/W us E Sets the maximum time interval between encoder counter increments before the status turns to static.
#define PARAM_NAME_SFNC_ENCODERRESETSOURCE						"EncoderResetSource"							//R IEnumeration R/W - E Selects the signals that will be the source to reset the Encoder.
#define PARAM_NAME_SFNC_ENCODERRESETACTIVATION					"EncoderResetActivation"						//R IEnumeration R/W - E Selects the Activation mode of the Encoder Reset Source signal.
#define PARAM_NAME_SFNC_ENCODERRESET							"EncoderReset"									//R ICommand (R)/W - E Does a software reset of the selected Encoder and starts it.
#define PARAM_NAME_SFNC_ENCODERVALUE							"EncoderValue"									//R IInteger R/W - E Reads or writes the current value of the position counter of the selected Encoder.
#define PARAM_NAME_SFNC_ENCODERVALUEATRESET						"EncoderValueAtReset"							//R IInteger R - E Reads the value of the of the position counter of the selected Encoder when it was reset by a signal or by an explicit EncoderReset command.

//2.10 Logic Block Control(Contains the features related to the usage of the logic block)
#define PARAM_NAME_SFNC_LOGICBLOCKSELECTOR 						"LogicBlockSelector" 							//O IEnumeration R/W - G Specifies the Logic Block to configure.
#define PARAM_NAME_SFNC_LOGICBLOCKFUNCTION						"LogicBlockFunction"							//O IEnumeration R/W - G Selects the combinational logic Function of the Logic Block to configure.
#define PARAM_NAME_SFNC_LOGICBLOCKINPUTNUMBER					"LogicBlockInputNumber"							//O IInteger R/(W) - G Specifies the number of active signal inputs of the Logic Block.
#define PARAM_NAME_SFNC_LOGICBLOCKINPUTSELECTOR					"LogicBlockInputSelector"						//O IInteger R/W - G Selects the Logic Block¡¯s input to configure.
#define PARAM_NAME_SFNC_LOGICBLOCKINPUTSOURCE					"LogicBlockInputSource"							//O IEnumeration R/W - G Selects the source signal for the input into the Logic Block.
#define PARAM_NAME_SFNC_LOGICBLOCKINPUTINVERTER					"LogicBlockInputInverter"						//O IBoolean R/W - G Selects if the selected Logic Block Input source signal is inverted.
#define PARAM_NAME_SFNC_LOGICBLOCKLUTINDEX						"LogicBlockLUTIndex"							//O IInteger R/W - G Controls the index of the truth table to access in the selected LUT.
#define PARAM_NAME_SFNC_LOGICBLOCKLUTVALUE						"LogicBlockLUTValue"							//O IBoolean R/W - G Read or Write the Value associated with the entry at index LogicBlockLUTIndex of the selected LUT.
#define PARAM_NAME_SFNC_LOGICBLOCKLUTVALUEALL					"LogicBlockLUTValueAll"							//O IInteger R/W - G Sets the values of all the output bits of the selected LUT in one accessignoring LogicBlockLUTIndex.
#define PARAM_NAME_SFNC_LOGICBLOCKLUTSELECTOR					"LogicBlockLUTSelector"							//O IEnumeration R/W - G Selects which of the two LUTs to configure when the selected Logic Block is a Latched dual LUTs (i.
             
//2.11Software Signal Control(Contains the features related to the control of the Software Signal)
#define PARAM_NAME_SFNC_SOFTWARESIGNALCONTROL	 				"SoftwareSignalControl"	 						//O ICategory R - B Category that contains the Software Signal Control features.
#define PARAM_NAME_SFNC_SOFTWARESIGNALSELECTOR 					"SoftwareSignalSelector" 						//O IEnumeration R/W - B Selects which Software Signal features to control.
#define PARAM_NAME_SFNC_SOFTWARESIGNALPULSE						"SoftwareSignalPulse"							//O ICommand (R)/W - B Generates a pulse signal that can be used as a software trigger.

//2.12Action Control(Contains the features related to the control of the Action command mechanism)
#define PARAM_NAME_SFNC_ACTIONUNCONDITIONALMODE 				"ActionUnconditionalMode" 						//O IEnumeration R/W - G Enables the unconditional action command mode where action commands are processed even when the primary control channel is closed.
#define PARAM_NAME_SFNC_ACTIONDEVICEKEY 						"ActionDeviceKey" 								//O IInteger W-O - G Provides the device key that allows the device to check the validity of action commands.
#define PARAM_NAME_SFNC_ACTIONQUEUESIZE 						"ActionQueueSize" 								//O IInteger R - G Indicates the size of the scheduled action commands queue.
#define PARAM_NAME_SFNC_ACTIONSELECTOR 							"ActionSelector" 								//O IInteger R/W - G Selects to which Action Signal further Action settings apply.
#define PARAM_NAME_SFNC_ACTIONGROUPMASK							"ActionGroupMask"								//O IInteger R/W - G Provides the mask that the device will use to validate the action on reception of the action protocol message.
#define PARAM_NAME_SFNC_ACTIONGROUPKEY							"ActionGroupKey"								//O IInteger R/W - G Provides the key that the device will use to validate the action on reception of the action protocol message.

//2.13Event Control(Contains the features related to the generation of Event notifications by the device)
#define PARAM_NAME_SFNC_EVENTSELECTOR 							"EventSelector" 								//R IEnumeration R/W - E Selects which Event to signal to the host application.
#define PARAM_NAME_SFNC_EVENTNOTIFICATION						"EventNotification"								//R IEnumeration R/W - E Activate or deactivate the notification to the host application of the occurrence of the selected Event.
#define PARAM_NAME_SFNC_EVENTFRAMETRIGGERDATA 					"EventFrameTriggerData" 						//R ICategory R - E Category that contains all the data features related to the FrameTrigger Event.
#define PARAM_NAME_SFNC_EVENTFRAMETRIGGER 						"EventFrameTrigger" 							//R IInteger R - E Returns the unique Identifier of the FrameTrigger type of Event.
#define PARAM_NAME_SFNC_EVENTFRAMETRIGGERTIMESTAMP 				"EventFrameTriggerTimestamp" 					//R IInteger R - E Returns the Timestamp of the FrameTrigger Event.
#define PARAM_NAME_SFNC_EVENTFRAMETRIGGERFRAMEID 				"EventFrameTriggerFrameID" 						//R IInteger R - E Returns the unique Identifier of the Frame (or image) that generated the FrameTrigger Event.
#define PARAM_NAME_SFNC_EVENTEXPOSUREENDDATA 					"EventExposureEndData" 							//R ICategory R - E Category that contains all the data features related to the ExposureEnd Event.
#define PARAM_NAME_SFNC_EVENTEXPOSUREEND 						"EventExposureEnd" 								//R IInteger R - E Returns the unique identifier of the ExposureEnd type of Event.
#define PARAM_NAME_SFNC_EVENTEXPOSUREENDTIMESTAMP 				"EventExposureEndTimestamp" 					//R IInteger R - E Returns the Timestamp of the ExposureEnd Event.
#define PARAM_NAME_SFNC_EVENTEXPOSUREENDFRAMEID 				"EventExposureEndFrameID" 						//R IInteger R - E Returns the unique Identifier of the Frame (or image) that generated the ExposureEnd Event.
#define PARAM_NAME_SFNC_EVENTERRORDATA 							"EventErrorData" 								//R ICategory R - E Category that contains all the data features related to the Error Event.
#define PARAM_NAME_SFNC_EVENTERROR 								"EventError" 									//R IInteger R - E Returns the unique identifier of the Error type of Event.
#define PARAM_NAME_SFNC_EVENTERRORTIMESTAMP 					"EventErrorTimestamp" 							//R IInteger R - E Returns the Timestamp of the Error Event.
#define PARAM_NAME_SFNC_EVENTERRORFRAMEID	 					"EventErrorFrameID"	 							//R IInteger R - E If applicable, returns the unique Identifier of the Frame (or image) that generated the Error Event.
#define PARAM_NAME_SFNC_EVENTERRORCODE 							"EventErrorCode" 								//R IInteger R - E Returns an error code for the error(s) that happened.
#define PARAM_NAME_SFNC_EVENTTESTDATA 							"EventTestData" 								//R ICategory R - E Category that contains all the data features related to the Event Test generated using the TestEventGenerate command.
#define PARAM_NAME_SFNC_EVENTTEST 								"EventTest" 									//R IInteger R - E Returns the unique identifier of the Event Test type of event generated using the TestEventGenerate command.
#define PARAM_NAME_SFNC_EVENTTESTTIMESTAMP 						"EventTestTimestamp" 							//R IInteger R - E Returns the Timestamp of the Event Test event.

//2.14User Set Control(Contains the features related to the User Set Control to save and load the user device settings)
#define PARAM_NAME_SFNC_USERSETSELECTOR 						"UserSetSelector" 								//R IEnumeration R/W - B Selects the feature User Set to load, save or configure.
#define PARAM_NAME_SFNC_USERSETLOAD								"UserSetLoad"									//R ICommand (R)/W - B Loads the User Set specified by UserSetSelector to the device and makes it active.
#define PARAM_NAME_SFNC_USERSETSAVE								"UserSetSave"									//R ICommand (R)/W - B Save the User Set specified by UserSetSelector to the non-volatile memory of the device.
#define PARAM_NAME_SFNC_USERSETDEFAULT 							"UserSetDefault" 								//O IEnumeration R/W - B Selects the feature User Set to load and make active by default when the device is reset.
#define PARAM_NAME_SFNC_USERSETDEFAULTSELECTOR 					"UserSetDefaultSelector" 						//O IEnumeration R/W - B This feature is deprecated (See UserSetDefault).
#define PARAM_NAME_SFNC_USERSETFEATURESELECTOR 					"UserSetFeatureSelector" 						//R IEnumeration R/W - E Selects which individual UserSet feature to control.
#define PARAM_NAME_SFNC_USERSETFEATUREENABLE					"UserSetFeatureEnable"							//R IBoolean R/(W) - E Enables the selected feature and make it active in all the UserSets.

//2.15 Sequencer Control(Contains the features related to the control of the Sequencer for features change)
#define PARAM_NAME_SFNC_SEQUENCERMODE 							"SequencerMode" 								//R IEnumeration R/W - E Controls if the sequencer mechanism is active.
#define PARAM_NAME_SFNC_SEQUENCERCONFIGURATIONMODE 				"SequencerConfigurationMode" 					//R IEnumeration R/W - E Controls if the sequencer configuration mode is active.
#define PARAM_NAME_SFNC_SEQUENCERFEATURESELECTOR 				"SequencerFeatureSelector" 						//R IEnumeration R/W - E Selects which sequencer features to control.
#define PARAM_NAME_SFNC_SEQUENCERFEATUREENABLE					"SequencerFeatureEnable"						//R IBoolean R/(W) - E Enables the selected feature and make it active in all the sequencer sets.
#define PARAM_NAME_SFNC_SEQUENCERSETSELECTOR 					"SequencerSetSelector" 							//R IInteger R/W - E Selects the sequencer set to which further feature settings applies.
#define PARAM_NAME_SFNC_SEQUENCERSETSAVE						"SequencerSetSave"								//R ICommand (R)/W - E Saves the current device state to the sequencer set selected by the SequencerSetSelector.
#define PARAM_NAME_SFNC_SEQUENCERSETLOAD						"SequencerSetLoad"								//R ICommand (R)/W - E Loads the sequencer set selected by SequencerSetSelector in the device.
#define PARAM_NAME_SFNC_SEQUENCERSETACTIVE 						"SequencerSetActive" 							//O IInteger R - E Contains the currently active sequencer set.
#define PARAM_NAME_SFNC_SEQUENCERSETSTART 						"SequencerSetStart" 							//R IInteger R/W - E Sets the initial/start sequencer set, which is the first set used within a sequencer.
#define PARAM_NAME_SFNC_SEQUENCERPATHSELECTOR					"SequencerPathSelector"							//R IInteger R/W - E Selects to which branching path further path settings applies.
#define PARAM_NAME_SFNC_SEQUENCERSETNEXT 						"SequencerSetNext" 								//R IInteger R/W - E Specifies the next sequencer set.
#define PARAM_NAME_SFNC_SEQUENCERTRIGGERSOURCE					"SequencerTriggerSource"						//R IEnumeration R/W - E Specifies the internal signal or physical input line to use as the sequencer trigger source.
#define PARAM_NAME_SFNC_SEQUENCERTRIGGERACTIVATION				"SequencerTriggerActivation"					//R IEnumeration R/W - E Specifies the activation mode of the sequencer trigger.

//2.16File Access Control(Contains the features related to the generic file access of a device)
#define PARAM_NAME_SFNC_FILESELECTOR 							"FileSelector" 									//R IEnumeration R/(W) - G Selects the target file in the device.
#define PARAM_NAME_SFNC_FILEOPERATIONSELECTOR					"FileOperationSelector"							//R IEnumeration R/W - G Selects the target operation for the selected file in the device.
#define PARAM_NAME_SFNC_FILEOPERATIONEXECUTE					"FileOperationExecute"							//R ICommand (R)/W - G Executes the operation selected by FileOperationSelector on the selected file.
#define PARAM_NAME_SFNC_FILEOPENMODE							"FileOpenMode"									//R IEnumeration R/(W) - G Selects the access mode in which a file is opened in the device.
#define PARAM_NAME_SFNC_FILEACCESSBUFFER 						"FileAccessBuffer" 								//R IRegister R/(W) - G Defines the intermediate access buffer that allows the exchange of data between the device file storage and the application.
#define PARAM_NAME_SFNC_FILEACCESSOFFSET						"FileAccessOffset"								//R IInteger R/(W) B G Controls the Offset of the mapping between the device file storage and the FileAccessBuffer.
#define PARAM_NAME_SFNC_FILEACCESSLENGTH						"FileAccessLength"								//R IInteger R/W B G Controls the Length of the mapping between the device file storage and the FileAccessBuffer.
#define PARAM_NAME_SFNC_FILEOPERATIONSTATUS						"FileOperationStatus"							//R IEnumeration R - G Represents the file operation execution status.
#define PARAM_NAME_SFNC_FILEOPERATIONRESULT						"FileOperationResult"							//R IInteger R - G Represents the file operation result.
#define PARAM_NAME_SFNC_FILESIZE								"FileSize"										//R IInteger R B G Represents the size of the selected file in bytes.
             
//2.17Source Control(Contains the features related to the control of the multiple Source devices)
#define PARAM_NAME_SFNC_SOURCECOUNT 							"SourceCount" 									//O IInteger R/(W) - B Controls or returns the number of sources supported by the device.
#define PARAM_NAME_SFNC_SOURCESELECTOR							"SourceSelector"								//O IEnumeration R/W - B Selects the source to control.
#define PARAM_NAME_SFNC_SOURCEIDVALUE							"SourceIDValue"									//O IInteger R - E Retuns a unique Identifier value that correspond to the selected Source.

//2.18Transfer Control(Contains the features related to the control of the Transfers )
#define PARAM_NAME_SFNC_TRANSFERSELECTOR 						"TransferSelector" 								//O IEnumeration R/(W) - E Selects which stream transfers are currently controlled by the selected Transfer features.
#define PARAM_NAME_SFNC_TRANSFERCONTROLMODE						"TransferControlMode"							//R IEnumeration R/(W) - E Selects the control method for the transfers.
#define PARAM_NAME_SFNC_TRANSFEROPERATIONMODE					"TransferOperationMode"							//O IEnumeration R/(W) - E Selects the operation mode of the transfer.
#define PARAM_NAME_SFNC_TRANSFERBLOCKCOUNT						"TransferBlockCount"							//O IInteger R/(W) - E Specifies the number of data Blocks that the device should stream before stopping.
#define PARAM_NAME_SFNC_TRANSFERBURSTCOUNT 						"TransferBurstCount" 							//O IInteger R/W - E Number of Block(s) to transfer for each TransferBurstStart trigger.
#define PARAM_NAME_SFNC_TRANSFERQUEUEMAXBLOCKCOUNT				"TransferQueueMaxBlockCount"					//O IInteger R/(W) - E Controls the maximum number of data blocks that can be stored in the block queue of the selected stream.
#define PARAM_NAME_SFNC_TRANSFERQUEUECURRENTBLOCKCOUNT			"TransferQueueCurrentBlockCount"				//O IInteger R - E Returns the number of Block(s) currently in the transfer queue.
#define PARAM_NAME_SFNC_TRANSFERQUEUEMODE						"TransferQueueMode"								//O IEnumeration R/(W) - E Specifies the operation mode of the transfer queue.
#define PARAM_NAME_SFNC_TRANSFERSTART							"TransferStart"									//O ICommand (R)/W - E Starts the streaming of data blocks out of the device.
#define PARAM_NAME_SFNC_TRANSFERSTOP							"TransferStop"									//O ICommand (R)/W - E Stops the streaming of data Block(s).
#define PARAM_NAME_SFNC_TRANSFERABORT							"TransferAbort"									//O ICommand (R)/W - E Aborts immediately the streaming of data block(s).
#define PARAM_NAME_SFNC_TRANSFERPAUSE							"TransferPause"									//O ICommand (R)/W - G Pauses the streaming of data Block(s).
#define PARAM_NAME_SFNC_TRANSFERRESUME							"TransferResume"								//O ICommand (R)/W - G Resumes a data Blocks streaming that was previously paused by a TransferPause command.
#define PARAM_NAME_SFNC_TRANSFERTRIGGERSELECTOR					"TransferTriggerSelector"						//O IEnumeration R/W - G Selects the type of transfer trigger to configure.
#define PARAM_NAME_SFNC_TRANSFERTRIGGERMODE						"TransferTriggerMode"							//R IEnumeration R/W - G Controls if the selected trigger is active.
#define PARAM_NAME_SFNC_TRANSFERTRIGGERSOURCE					"TransferTriggerSource"							//O IEnumeration R/W - G Specifies the signal to use as the trigger source for transfers.
#define PARAM_NAME_SFNC_TRANSFERTRIGGERACTIVATION				"TransferTriggerActivation"						//O IEnumeration R/W - G Specifies the activation mode of the transfer control trigger.
#define PARAM_NAME_SFNC_TRANSFERSTATUSSELECTOR					"TransferStatusSelector"						//R IEnumeration R/W - G Selects which status of the transfer module to read.
#define PARAM_NAME_SFNC_TRANSFERSTATUS							"TransferStatus"								//R IBool R - G Reads the status of the Transfer module signal selected by TransferStatusSelector.
#define PARAM_NAME_SFNC_TRANSFERCOMPONENTSELECTOR				"TransferComponentSelector"						//O IEnumeration R/W - G Selects the color component for the control of the	TransferStreamChannel feature.
#define PARAM_NAME_SFNC_TRANSFERSTREAMCHANNEL					"TransferStreamChannel"							//O IInteger R/W - G Selects the streaming channel that will be used to transfer the selected stream of data.

//2.19Scan 3D Control(Contains the features related to the control of the 3D scan features)
#define PARAM_NAME_SFNC_SCAN3DEXTRACTIONSELECTOR 				"Scan3dExtractionSelector" 						//O IEnumeration R/W - E Selects the 3DExtraction processing module to control (if multiple ones are present).
#define PARAM_NAME_SFNC_SCAN3DEXTRACTIONSOURCE 					"Scan3dExtractionSource" 						//O IEnumeration R/W - E Selects the sensor¡¯s data source region for 3D Extraction module.
#define PARAM_NAME_SFNC_SCAN3DEXTRACTIONMETHOD					"Scan3dExtractionMethod"						//O IEnumeration R/W - E Selects the method for extracting 3D from the input sensor data.
#define PARAM_NAME_SFNC_SCAN3DDISTANCEUNIT						"Scan3dDistanceUnit"							//O IEnumeration R/(W) - E Specifies the unit used when delivering (calibrated) distance data.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATESYSTEM					"Scan3dCoordinateSystem"						//O IEnumeration R/(W) - E Specifies the Coordinate system to use for the device.
#define PARAM_NAME_SFNC_SCAN3DOUTPUTMODE						"Scan3dOutputMode"								//O IEnumeration R/(W) - E Controls the Calibration and data organization of the device and the coordinates transmitted.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATESYSTEMREFERENCE  		"Scan3dCoordinateSystemReference"				//O IEnumeration R/W - E Defines coordinate system reference location.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATESELECTOR         		"Scan3dCoordinateSelector"         				//O IEnumeration R/W - E Selects the individual coordinates in the vectors for 3D information/transformation.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATESCALE					"Scan3dCoordinateScale"							//O IFloat R/(W) - E Scale factor when transforming a pixel from relative coordinates to world coordinates.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATEOFFSET					"Scan3dCoordinateOffset"						//O IFloat R/(W) - E Offset when transforming a pixel from relative coordinates to world coordinates.
#define PARAM_NAME_SFNC_SCAN3DINVALIDDATAFLAG					"Scan3dInvalidDataFlag"							//O IBoolean R/(W) - E Enables the definition of a non-valid flag value in the data stream.
#define PARAM_NAME_SFNC_SCAN3DINVALIDDATAVALUE					"Scan3dInvalidDataValue"						//O IFloat R/(W) - E Value which identifies a non-valid pixel if Scan3dInvalidDataFlag is enabled.
#define PARAM_NAME_SFNC_SCAN3DAXISMIN							"Scan3dAxisMin"									//O IFloat R - E Minimum valid transmitted coordinate value of the selected Axis.
#define PARAM_NAME_SFNC_SCAN3DAXISMAX							"Scan3dAxisMax"									//O IFloat R - E Maximum valid transmitted coordinate value of the selected Axis.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATETRANSFORMSELECTOR		"Scan3dCoordinateTransformSelector"				//O IEnumeration R/W - E Sets the index to read/write a coordinate transform value.
#define PARAM_NAME_SFNC_SCAN3DTRANSFORMVALUE 					"Scan3dTransformValue" 							//O IFloat R/W - E Specifies the transform value selected.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATEREFERENCESELECTOR		"Scan3dCoordinateReferenceSelector"				//O IEnumeration R/W - E Sets the index to read a coordinate system reference value defining the transform of a point from the current (Anchor or Transformed) system to the reference system.
#define PARAM_NAME_SFNC_SCAN3DCOORDINATEREFERENCEVALUE			"Scan3dCoordinateReferenceValue"				//O IFloat R - E Returns the reference value selected.

//2.20Chunk Data Control(Contains the features related to the Chunk Data Control)
#define PARAM_NAME_SFNC_CHUNKMODEACTIVE 						"ChunkModeActive" 								//R IBoolean R/W - E Activates the inclusion of Chunk data in the payload of the image.
#define PARAM_NAME_SFNC_CHUNKSELECTOR 							"ChunkSelector" 								//R IEnumeration R/W - E Selects which Chunk to enable or control.
#define PARAM_NAME_SFNC_CHUNKENABLE								"ChunkEnable"									//R IBoolean R/W - E Enables the inclusion of the selected Chunk data in the payload of the image.
#define PARAM_NAME_SFNC_CHUNKCOMPONENTSELECTOR 					"ChunkComponentSelector" 						//O IEnumeration R - E Selects the Component from which to retreive data from.
#define PARAM_NAME_SFNC_CHUNKCOMPONENTID 						"ChunkComponentID" 								//O IEnumeration R - E Returns the Component Identifier of the selected Component image.
#define PARAM_NAME_SFNC_CHUNKCOMPONENTIDVALUE					"ChunkComponentIDValue"							//O IInteger R - E Returns a unique Identifier value that correspond to the selected chunk Component.
#define PARAM_NAME_SFNC_CHUNKIMAGECOMPONENT 					"ChunkImageComponent" 							//O IEnumeration R - E This feature is deprecated (See ChunkComponentID).
#define PARAM_NAME_SFNC_CHUNKPARTSELECTOR 						"ChunkPartSelector" 							//O IInteger R/W - E This feature is deprecated (See ChunkComponentSelector).
#define PARAM_NAME_SFNC_CHUNKIMAGE 								"ChunkImage" 									//R IRegister R - G Returns the entire image data included in the payload.
#define PARAM_NAME_SFNC_CHUNKOFFSETX 							"ChunkOffsetX" 									//R IInteger R - E Returns the OffsetX of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKOFFSETY 							"ChunkOffsetY" 									//R IInteger R - E Returns the OffsetY of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKWIDTH 								"ChunkWidth" 									//R IInteger R - E Returns the Width of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKHEIGHT 							"ChunkHeight" 									//R IInteger R - E Returns the Height of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKPIXELFORMAT 						"ChunkPixelFormat" 								//R IEnumeration R - E Returns the PixelFormat of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKPIXELDYNAMICRANGEMIN 				"ChunkPixelDynamicRangeMin" 					//R IInteger R - E Returns the minimum value of dynamic range of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKPIXELDYNAMICRANGEMAX 				"ChunkPixelDynamicRangeMax" 					//R IInteger R - E Returns the maximum value of dynamic range of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKTIMESTAMP 							"ChunkTimestamp" 								//R IInteger R - E Returns the Timestamp of the image included in the payload at the time of the FrameStart internal event.
#define PARAM_NAME_SFNC_CHUNKTIMESTAMPLATCHVALUE 				"ChunkTimestampLatchValue" 						//R IInteger R ns E Returns the last Timestamp latched with the TimestampLatch command.
#define PARAM_NAME_SFNC_CHUNKLINESTATUSALL 						"ChunkLineStatusAll" 							//R IInteger R - E Returns the status of all the I/O lines at the time of the FrameStart internal event.
#define PARAM_NAME_SFNC_CHUNKCOUNTERSELECTOR 					"ChunkCounterSelector" 							//R IEnumeration R/W - E Selects which counter to retrieve data from.
#define PARAM_NAME_SFNC_CHUNKCOUNTERVALUE						"ChunkCounterValue"								//R IInteger R - E Returns the value of the selected Chunk counter at the time of the FrameStart event.
#define PARAM_NAME_SFNC_CHUNKTIMERSELECTOR 						"ChunkTimerSelector" 							//R IEnumeration R/W - E Selects which Timer to retrieve data from.
#define PARAM_NAME_SFNC_CHUNKTIMERVALUE							"ChunkTimerValue"								//R IFloat R us E Returns the value of the selected Timer at the time of the FrameStart internal event.
#define PARAM_NAME_SFNC_CHUNKSCANLINESELECTOR 					"ChunkScanLineSelector" 						//O IInteger R - E Index for vector representation of one chunk value per line in an image.
#define PARAM_NAME_SFNC_CHUNKENCODERSELECTOR 					"ChunkEncoderSelector" 							//R IEnumeration R/W - E Selects which Encoder to retrieve data from.
#define PARAM_NAME_SFNC_CHUNKENCODERVALUE						"ChunkEncoderValue"								//R IInteger R - E Returns the counter¡¯s value of the selected Encoder at the time of the FrameStart in area scan mode or the counter¡¯s value at the time of the LineStart selected by ChunkScanLineSelector in Linescan mode.
#define PARAM_NAME_SFNC_CHUNKENCODERSTATUS						"ChunkEncoderStatus"							//O IEnumeration R - E Returns the motion status of the selected encoder.
#define PARAM_NAME_SFNC_CHUNKEXPOSURETIMESELECTOR 				"ChunkExposureTimeSelector" 					//O IEnumeration R/W - E Selects which exposure time is read by the ChunkExposureTime feature.
#define PARAM_NAME_SFNC_CHUNKEXPOSURETIME						"ChunkExposureTime"								//R IFloat R us E Returns the exposure time used to capture the image.
#define PARAM_NAME_SFNC_CHUNKGAINSELECTOR 						"ChunkGainSelector" 							//R IEnumeration R/W - E Selects which Gain to return.
#define PARAM_NAME_SFNC_CHUNKGAIN								"ChunkGain"										//R IFloat R - E Returns the gain used to capture the image.
#define PARAM_NAME_SFNC_CHUNKBLACKLEVELSELECTOR 				"ChunkBlackLevelSelector" 						//R IEnumeration R/W - E Selects which Black Level to return.
#define PARAM_NAME_SFNC_CHUNKBLACKLEVEL							"ChunkBlackLevel"								//R IFloat R - E Returns the black level used to capture the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKLINEPITCH 							"ChunkLinePitch" 								//R IInteger R B E Returns the LinePitch of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKFRAMEID 							"ChunkFrameID" 									//R IInteger R - E Returns the unique Identifier of the frame (or image) included in the payload.
#define PARAM_NAME_SFNC_CHUNKSOURCESELECTOR 					"ChunkSourceSelector" 							//O IEnumeration R - E Selects which Source to retrieve data from.
#define PARAM_NAME_SFNC_CHUNKSOURCEID 							"ChunkSourceID" 								//O IEnumeration R - E Returns the Identifier of Source that the image comes from.
#define PARAM_NAME_SFNC_CHUNKSOURCEIDVALUE 						"ChunkSourceIDValue" 							//R IInteger R - E Returns the unique integer Identifier value of the Source that the image comes from.
#define PARAM_NAME_SFNC_CHUNKREGIONSELECTOR 					"ChunkRegionSelector" 							//O IEnumeration R - E Selects which Region to retrieve data from.
#define PARAM_NAME_SFNC_CHUNKREGIONID 							"ChunkRegionID" 								//O IEnumeration R - E Returns the Identifier of Region that the image comes from.
#define PARAM_NAME_SFNC_CHUNKREGIONIDVALUE 						"ChunkRegionIDValue" 							//R IInteger R - E Returns the unique integer Identifier value of the Region that the image comes from.
#define PARAM_NAME_SFNC_CHUNKTRANSFERBLOCKID 					"ChunkTransferBlockID" 							//R IInteger R - E Returns the unique identifier of the transfer block used to transport the payload.
#define PARAM_NAME_SFNC_CHUNKTRANSFERSTREAMID 					"ChunkTransferStreamID" 						//R IEnumeration R - E Returns identifier of the stream that generated this block.
#define PARAM_NAME_SFNC_CHUNKTRANSFERQUEUECURRENTBLOCKCOUNT		"ChunkTransferQueueCurrentBlockCount"			//O IInteger R - E Returns the current number of blocks in the transfer queue.
#define PARAM_NAME_SFNC_CHUNKSTREAMCHANNELID 					"ChunkStreamChannelID" 							//R IInteger R - E Returns identifier of the stream channel used to carry the block.
#define PARAM_NAME_SFNC_CHUNKSEQUENCERSETACTIVE 				"ChunkSequencerSetActive" 						//R IInteger R - E Return the index of the active set of the running sequencer included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DDISTANCEUNIT	 				"ChunkScan3dDistanceUnit"	 					//O IEnumeration R - E Returns the Distance Unit of the payload image.
#define PARAM_NAME_SFNC_CHUNKSCAN3DOUTPUTMODE 					"ChunkScan3dOutputMode" 						//O IEnumeration R - E Returns the Calibrated Mode of the payload image.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATESYSTEM 			"ChunkScan3dCoordinateSystem" 					//O IEnumeration R - E Returns the Coordinate System of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATESYSTEMREFERENCE	"ChunkScan3dCoordinateSystemReference"			//O IEnumeration R - E Returns the Coordinate System Position of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATESELECTOR 			"ChunkScan3dCoordinateSelector" 				//O IEnumeration R - E Selects which Coordinate to retrieve data from.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATESCALE				"ChunkScan3dCoordinateScale"					//O IFloat R - E Returns the Scale for the selected coordinate axis of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATEOFFSET 			"ChunkScan3dCoordinateOffset" 					//O IFloat R - E Returns the Offset for the selected coordinate axis of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DINVALIDDATAFLAG     			"ChunkScan3dInvalidDataFlag"     				//O IBoolean R - E Returns if a specific non-valid data flag is used in the data stream.
#define PARAM_NAME_SFNC_CHUNKSCAN3DINVALIDDATAVALUE				"ChunkScan3dInvalidDataValue"					//O IFloat R - E Returns the Invalid Data Value used for the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DAXISMIN						"ChunkScan3dAxisMin"							//O IFloat R - E Returns the Minimum Axis value for the selected coordinate axis of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DAXISMAX						"ChunkScan3dAxisMax"							//O IFloat R - E Returns the Maximum Axis value for the selected coordinate axis of the image included in the payload.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATETRANSFORMSELECTOR	"ChunkScan3dCoordinateTransformSelector"		//O IEnumeration R/W - E Selector for transform values.
#define PARAM_NAME_SFNC_CHUNKSCAN3DTRANSFORMVALUE				"ChunkScan3dTransformValue"						//O IFloat R - E Returns the transform value.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATEREFERENCESELECTOR	"ChunkScan3dCoordinateReferenceSelector"		//O IEnumeration R/W - E Selector to read a coordinate system reference value defining the transform of a point from one system to the other.
#define PARAM_NAME_SFNC_CHUNKSCAN3DCOORDINATEREFERENCEVALUE		"ChunkScan3dCoordinateReferenceValue"			//O IFloat R - E Reads the value of a position or pose coordinate for the anchor or transformed coordinate systems relative to the reference point.


//2.21Test Control(Contains the features related to the control of the test features )
#define PARAM_NAME_SFNC_TESTPENDINGACK 							"TestPendingAck" 								//O IInteger R/W ms G Tests the device's pending acknowledge feature.
#define PARAM_NAME_SFNC_TESTEVENTGENERATE 						"TestEventGenerate" 							//O ICommand (R)/W - G Generates a Test Event.

//2.22GenICam Control(Contains the features related to GenICam control and access)
#define PARAM_NAME_SFNC_TLPARAMSLOCKED							"TLParamsLocked" 								//M IInteger R/W - I Used by the Transport Layer to prevent critical features from changing during acquisition.

//2.23Transport Layer Control(Contains the features related to the Transport Layer Control)
#define PARAM_NAME_SFNC_PAYLOADSIZE 							"PayloadSize" 									//R IInteger R B E Provides the number of bytes transferred for each image or chunk on the stream channel.
#define PARAM_NAME_SFNC_DEVICETAPGEOMETRY 						"DeviceTapGeometry" 							//R IEnumeration R/(W) - E This device tap geometry feature describes the geometrical properties characterizing the taps of a camera as presented at the output of the device.
#define PARAM_NAME_SFNC_GEVVERSIONMAJOR 						"GevVersionMajor" 								//R IInteger R - E This feature is deprecated (See DeviceTLVersionMajor).
#define PARAM_NAME_SFNC_GEVVERSIONMINOR 						"GevVersionMinor" 								//R IInteger R - E This feature is deprecated (See DeviceTLVersionMinor).
#define PARAM_NAME_SFNC_GEVDEVICEMODEISBIGENDIAN 				"GevDeviceModeIsBigEndian" 						//O IBoolean R - G This feature is deprecated (See DeviceRegistersEndianness).
#define PARAM_NAME_SFNC_GEVDEVICECLASS 							"GevDeviceClass" 								//O IEnumeration R - G This feature is deprecated (See DeviceType).
#define PARAM_NAME_SFNC_GEVDEVICEMODECHARACTERSET 				"GevDeviceModeCharacterSet" 					//O IEnumeration R - G This feature is deprecated (See DeviceCharacterSet).
#define PARAM_NAME_SFNC_GEVPHYSICALLINKCONFIGURATION 			"GevPhysicalLinkConfiguration" 					//O IEnumeration R/W - E Controls the principal physical link configuration to use on next restart/power-up of the device.
#define PARAM_NAME_SFNC_GEVCURRENTPHYSICALLINKCONFIGURATION		"GevCurrentPhysicalLinkConfiguration"			//O IEnumeration R - E Indicates the current physical link configuration of the device.
#define PARAM_NAME_SFNC_GEVACTIVELINKCOUNT 						"GevActiveLinkCount" 							//O IInteger R - E Indicates the current number of active logical links.
#define PARAM_NAME_SFNC_GEVSUPPORTEDOPTIONSELECTOR 				"GevSupportedOptionSelector" 					//O IEnumeration R/W - E Selects the GEV option to interrogate for existing support.
#define PARAM_NAME_SFNC_GEVSUPPORTEDOPTION						"GevSupportedOption"							//O IBoolean R - E Returns if the selected GEV option is supported.
#define PARAM_NAME_SFNC_GEVINTERFACESELECTOR 					"GevInterfaceSelector" 							//O IInteger R/W - B Selects which logical link to control.
#define PARAM_NAME_SFNC_GEVLINKSPEED							"GevLinkSpeed"									//O IInteger R Mbs E This feature is deprecated (See DeviceLinkSpeed).
#define PARAM_NAME_SFNC_GEVMACADDRESS							"GevMACAddress"									//O IInteger R - B MAC address of the logical link.
#define PARAM_NAME_SFNC_GEVPAUSEFRAMERECEPTION					"GevPAUSEFrameReception"						//O IBoolean R/(W) - E Controls whether incoming PAUSE Frames are handled on the given logical link.
#define PARAM_NAME_SFNC_GEVPAUSEFRAMETRANSMISSION				"GevPAUSEFrameTransmission"						//O IBoolean R/(W) - E Controls whether PAUSE Frames can be generated on the given logical link.
#define PARAM_NAME_SFNC_GEVCURRENTIPCONFIGURATIONLLA			"GevCurrentIPConfigurationLLA"					//O IBoolean R/W - B Controls whether the Link Local Address IP configuration scheme is activated on the given logical link.
#define PARAM_NAME_SFNC_GEVCURRENTIPCONFIGURATIONDHCP			"GevCurrentIPConfigurationDHCP"					//O IBoolean R/W - B Controls whether the DHCP IP configuration scheme is activated on the given logical link.
#define PARAM_NAME_SFNC_GEVCURRENTIPCONFIGURATIONPERSISTENTIP	"GevCurrentIPConfigurationPersistentIP"			//O IBoolean R/W - B Controls whether the PersistentIP configuration scheme is activated on the given logical link.
#define PARAM_NAME_SFNC_GEVCURRENTIPADDRESS						"GevCurrentIPAddress"							//O IInteger R - B Reports the IP address for the given logical link.
#define PARAM_NAME_SFNC_GEVCURRENTSUBNETMASK					"GevCurrentSubnetMask"							//O IInteger R - B Reports the subnet mask of the given logical link.
#define PARAM_NAME_SFNC_GEVCURRENTDEFAULTGATEWAY				"GevCurrentDefaultGateway"						//O IInteger R - B Reports the default gateway IP address to be used on the given logical link.
#define PARAM_NAME_SFNC_GEVIPCONFIGURATIONSTATUS				"GevIPConfigurationStatus"						//O IEnumeration R - B Reports the current IP configuration status.
#define PARAM_NAME_SFNC_GEVFIRSTURL 							"GevFirstURL" 									//O IString R - G Indicates the first URL to the GenICam XML device description file.
#define PARAM_NAME_SFNC_GEVSECONDURL 							"GevSecondURL" 									//O IString R - G Indicates the second URL to the GenICam XML device description file.
#define PARAM_NAME_SFNC_GEVNUMBEROFINTERFACES 					"GevNumberOfInterfaces" 						//O IInteger R - E This feature is deprecated (See DeviceLinkSelector).
#define PARAM_NAME_SFNC_GEVPERSISTENTIPADDRESS					"GevPersistentIPAddress"						//O IInteger R/W - B Controls the Persistent IP address for this logical link.
#define PARAM_NAME_SFNC_GEVPERSISTENTSUBNETMASK					"GevPersistentSubnetMask"						//O IInteger R/W - B Controls the Persistent subnet mask associated with the Persistent IP address on this logical link.
#define PARAM_NAME_SFNC_GEVPERSISTENTDEFAULTGATEWAY				"GevPersistentDefaultGateway"					//O IInteger R/W - B Controls the persistent default gateway for this logical link.
#define PARAM_NAME_SFNC_GEVMESSAGECHANNELCOUNT 					"GevMessageChannelCount" 						//O IInteger R - E This feature is deprecated (See DeviceEventChannelCount).
#define PARAM_NAME_SFNC_GEVSTREAMCHANNELCOUNT 					"GevStreamChannelCount" 						//O IInteger R - E This feature is deprecated (See DeviceStreamChannelCount).
#define PARAM_NAME_SFNC_GEVHEARTBEATTIMEOUT 					"GevHeartbeatTimeout" 							//O IInteger R/W ms G This feature is deprecated (See DeviceLinkHeartbeatTimeout).
#define PARAM_NAME_SFNC_GEVTIMESTAMPTICKFREQUENCY 				"GevTimestampTickFrequency" 					//O IInteger R Hz E This feature is deprecated (See the increment of the TimestampLatchValue feature).
#define PARAM_NAME_SFNC_GEVTIMESTAMPCONTROLLATCH 				"GevTimestampControlLatch" 						//O ICommand W - E This feature is deprecated (See TimestampLatch).
#define PARAM_NAME_SFNC_GEVTIMESTAMPCONTROLRESET 				"GevTimestampControlReset" 						//O ICommand W - E This feature is deprecated (See TimestampReset).
#define PARAM_NAME_SFNC_GEVTIMESTAMPVALUE 						"GevTimestampValue" 							//O IInteger R E This feature is deprecated (See TimestampLatchValue).
#define PARAM_NAME_SFNC_GEVDISCOVERYACKDELAY 					"GevDiscoveryAckDelay" 							//O IInteger R/(W) ms E Indicates the maximum randomized delay the device will wait to acknowledge a discovery command.
#define PARAM_NAME_SFNC_GEVIEEE1588 							"GevIEEE1588" 									//O IBoolean R/W - E Enables the IEEE 1588 Precision Time Protocol to control the timestamp register.
#define PARAM_NAME_SFNC_GEVIEEE1588CLOCKACCURACY 				"GevIEEE1588ClockAccuracy" 						//O IEnumeration R/(W) - E Indicates the expected accuracy of the device clock when it is the grandmaster, or in the event it becomes the grandmaster.
#define PARAM_NAME_SFNC_GEVIEEE1588STATUS 						"GevIEEE1588Status" 							//O IEnumeration R - E Provides the status of the IEEE 1588 clock.
#define PARAM_NAME_SFNC_GEVGVCPEXTENDEDSTATUSCODESSELECTOR		"GevGVCPExtendedStatusCodesSelector"			//O IEnumeration R/W - G Selects the GigE Vision version to control extended status codes for.
#define PARAM_NAME_SFNC_GEVGVCPEXTENDEDSTATUSCODES				"GevGVCPExtendedStatusCodes"					//O IBoolean R/W - G Enables the generation of extended status codes.
#define PARAM_NAME_SFNC_GEVGVCPPENDINGACK						"GevGVCPPendingAck"								//O IBoolean R/W - G Enables the generation of PENDING_ACK.
#define PARAM_NAME_SFNC_GEVGVCPHEARTBEATDISABLE 				"GevGVCPHeartbeatDisable" 						//O IBoolean R/W - E This feature is deprecated (See DeviceHeartbeatMode).
#define PARAM_NAME_SFNC_GEVGVCPPENDINGTIMEOUT 					"GevGVCPPendingTimeout" 						//O IInteger R - G This feature is deprecated (See DeviceLinkCommandTimeout).
#define PARAM_NAME_SFNC_GEVPRIMARYAPPLICATIONSWITCHOVERKEY		"GevPrimaryApplicationSwitchoverKey"			//O IInteger W-O - G Controls the key to use to authenticate primary application switchover requests.
#define PARAM_NAME_SFNC_GEVGVSPEXTENDEDIDMODE 					"GevGVSPExtendedIDMode" 						//O IEnumeration R/(W) - E Enables the extended IDs mode.
#define PARAM_NAME_SFNC_GEVCCP 									"GevCCP" 										//O IEnumeration R/W - G Controls the device access privilege of an application.
#define PARAM_NAME_SFNC_GEVPRIMARYAPPLICATIONSOCKET 			"GevPrimaryApplicationSocket" 					//O IInteger R - G Returns the UDP source port of the primary application.
#define PARAM_NAME_SFNC_GEVPRIMARYAPPLICATIONIPADDRESS 			"GevPrimaryApplicationIPAddress" 				//O IInteger R - G Returns the address of the primary application.
#define PARAM_NAME_SFNC_GEVMCPHOSTPORT 							"GevMCPHostPort" 								//O IInteger R/W - G Controls the port to which the device must send messages.
#define PARAM_NAME_SFNC_GEVMCDA 								"GevMCDA" 										//O IInteger R/W - G Controls the destination IP address for the message channel.
#define PARAM_NAME_SFNC_GEVMCTT 								"GevMCTT" 										//O IInteger R/W ms G Provides the transmission timeout value in milliseconds.
#define PARAM_NAME_SFNC_GEVMCRC 								"GevMCRC" 										//O IInteger R/W - G Controls the number of retransmissions allowed when a message channel message times out.
#define PARAM_NAME_SFNC_GEVMCSP 								"GevMCSP" 										//O IInteger R - G This feature indicates the source port for the message channel.
#define PARAM_NAME_SFNC_GEVSTREAMCHANNELSELECTOR 				"GevStreamChannelSelector" 						//O IInteger R/W - E Selects the stream channel to control.
#define PARAM_NAME_SFNC_GEVSCCFGPACKETRESENDDESTINATION			"GevSCCFGPacketResendDestination"				//O IBoolean R/W - G Enables the alternate IP destination for stream packets resent due to a packet resend request.
#define PARAM_NAME_SFNC_GEVSCCFGALLINTRANSMISSION				"GevSCCFGAllInTransmission"						//O IBoolean R/W - G Enables the selected GVSP transmitter to use the single packet per data block All-in Transmission mode.
#define PARAM_NAME_SFNC_GEVSCCFGUNCONDITIONALSTREAMING   		"GevSCCFGUnconditionalStreaming"				//O IBoolean R/W - G Enables the camera to continue to stream, for this stream channel, if its control channel is closed or regardless of the reception of any ICMP messages (such as destination unreachable messages).
#define PARAM_NAME_SFNC_GEVSCCFGEXTENDEDCHUNKDATA				"GevSCCFGExtendedChunkData"						//O IBoolean R/W - G Enables cameras to use the extended chunk data payload type for this stream channel.
#define PARAM_NAME_SFNC_GEVSCPDIRECTION							"GevSCPDirection"								//O IEnumeration R - G This feature is deprecated (See DeviceStreamChannelType).
#define PARAM_NAME_SFNC_GEVSCPINTERFACEINDEX					"GevSCPInterfaceIndex"							//O IInteger R/(W) - G Index of the logical link to use.
#define PARAM_NAME_SFNC_GEVSCPHOSTPORT							"GevSCPHostPort"								//O IInteger R/W - G Controls the port of the selected channel to which a GVSP transmitter must send data stream or the port from which a GVSP receiver may receive data stream.
#define PARAM_NAME_SFNC_GEVSCPSFIRETESTPACKET					"GevSCPSFireTestPacket"							//O IBoolean R/W - G Sends a test packet.
#define PARAM_NAME_SFNC_GEVSCPSDONOTFRAGMENT					"GevSCPSDoNotFragment"							//O IBoolean R/W - G The state of this feature is copied into the "do not fragment" bit of IP header of each stream packet.
#define PARAM_NAME_SFNC_GEVSCPSBIGENDIAN						"GevSCPSBigEndian"								//O IBoolean R/W - G This feature is deprecated (See DeviceStreamChannelEndianness).
#define PARAM_NAME_SFNC_GEVSCPSPACKETSIZE						"GevSCPSPacketSize"								//R IInteger R/(W) B E This GigE Vision specific feature corresponds to DeviceStreamChannelPacketSize and should be kept in sync with it.
#define PARAM_NAME_SFNC_GEVSCPD									"GevSCPD"										//R IInteger R/W E Controls the delay (in GEV timestamp counter unit) to insert between each packet for this stream channel.
#define PARAM_NAME_SFNC_GEVSCDA									"GevSCDA"										//O IInteger R/W - G Controls the destination IP address of the selected stream channel to which a GVSP transmitter must send data stream or the destination IP address from which a GVSP receiver may receive data stream.
#define PARAM_NAME_SFNC_GEVSCSP									"GevSCSP"										//O IInteger R - G Indicates the source port of the stream channel.
#define PARAM_NAME_SFNC_GEVSCZONECOUNT							"GevSCZoneCount"								//O IInteger R - G Reports the number of zones per block transmitted on the selected stream channel.
#define PARAM_NAME_SFNC_GEVSCZONEDIRECTIONALL					"GevSCZoneDirectionAll"							//O IInteger R - G Reports the transmission direction of each zone transmitted on the selected stream channel.
#define PARAM_NAME_SFNC_GEVSCZONECONFIGURATIONLOCK				"GevSCZoneConfigurationLock"					//O IBoolean R/W - G Controls whether the selected stream channel multi-zone configuration is locked.
             
             

#define PARAM_NAME_SENSOR_OFFSETH								"SensorOffsetH"						//R IInteger R/W - E First column of visible pixels to be read out(starting Horizontal value).
#define PARAM_NAME_SENSOR_OFFSETV								"SensorOffsetV"						//R IInteger R/W - E First row of visible pixels to be read out(starting Vertical value).
#define PARAM_NAME_SENSOR_WIDTH									"SensorWidth"						//R IInteger R/W - E Visible pixels to be read out(Window Width).
#define PARAM_NAME_SENSOR_HEIGHT								"SensorHeight"						//R IInteger R/W - E Visible pixels to be read out(Window Height).
#define PARAM_NAME_SENSOR_BLANKINGH								"SensorBlankingH"					//R IInteger R/W - E Horizontal Blanking.
#define PARAM_NAME_SENSOR_BLANKINGV								"SensorBlankingV"					//R IInteger R/W - E Vertical Blanking.
#define PARAM_NAME_SENSOR_EXPOSURETIMERAW						"SensorExposureTimeRaw"				//R IInteger R/W - E Total integration time in number of rows.
#define PARAM_NAME_SENSOR_EXPOSUREAUTOENABLE					"SensorExposureAECEnable"			//R IBoolean R/W - E Automatic Exposure Control(AEC).
#define PARAM_NAME_SENSOR_GAINANALOGRAW							"SensorGainAnalogRaw"				//R IInteger R/W - E Analog Gain
#define PARAM_NAME_SENSOR_GAINANALOGAUTOENABLE					"SensorGainAnalogAGCEnable"			//R IBoolean R/W - E Automatic gain control (AGC).
#define PARAM_NAME_SENSOR_GAINANALOGAGCMAX						"SensorGainAnalogAGCMax"			//R IInteger R/W - E Automatic gain control (AGC) as the upper threshold of gain(Maximum Analog Gain).
#define PARAM_NAME_SENSOR_GAINDIGITRAW							"SensorGainDigitalRaw"				//R IInteger R/W - E Digital Gain
#define PARAM_NAME_SENSOR_GAINDIGITAUTOENABLE					"SensorGainDigitalAGCEnable"		//R IBoolean R/W - E Automatic Exposure Control.
#define PARAM_NAME_SENSOR_GAINDIGITAGCMAX						"SensorGainDigitalAGCMax"			//R IInteger R/W - E Automatic gain control (AGC) as the upper threshold of gain(Maximum Digital Gain).
#define PARAM_NAME_SENSOR_BITLUT								"SensorBitOuputLUT"					//R IEnumeration R/W - E Sensor Bit Output Mode

	//I/O Control Param
#define PARAM_NAME_TRIGGER_DEBOUNCE								"IOControlTriggerDebounce"			//R IInteger R/W - E Trigger Debounce pulse width
#define PARAM_NAME_TRIGGER_WIDTH								"IOControlTriggerPulseWidth"		//R IInteger R/W - E Trigger Pulse Width
#define PARAM_NAME_TRIGGER_INTERVAL								"IOControlTriggerInterval"			//R IInteger R/W - E Specifies the interval in microseconds (us) to apply between twice trigger
#define PARAM_NAME_TRIGGER_POLARITY								"IOControlTriggerPolarity"			//R IBoolean R/W - E Trigger Polarity

#define PARAM_NAME_STROBE_ENABLE								"IOControlStrobeEnable"				//R IBoolean R/W - E STROBE Signal Output Enable
#define PARAM_NAME_STROBE_WIDTH									"IOControlStrobeWidth"				//R IInteger R/W - E STROBE Signal Pulse Width
#define PARAM_NAME_STROBE_DELAY									"IOControlStrobeDelay"				//R IInteger R/W - E STROBE Signal Output Delay
#define PARAM_NAME_STROBE_POLARITY								"IOControlStrobePolarity"			//R IBoolean R/W - E STROBE Signal Polarity

	//Image Process Unit
#define PARAM_NAME_IPU_BRIGHTNESS								"ImageProcessBrightness"			//R IInteger R/W - E HW/SW Brightness Image Processing
#define PARAM_NAME_IPU_CONTRAST									"ImageProcessContrast"				//R IInteger R/W - E HW/SW Contrast Image Processing


#define PARAM_NAME_GRAB_FRAMERATE								"AcquisitionRate"						//Instant acquisition rate.
#define PARAM_NAME_GRAB_BANDWIDTH								"Bandwidth"								//Instant throughput in bits per second. Note that this value is calculated based on a finite sequence of received blocks within a finite period of time.
#define PARAM_NAME_GRAB_FRAMECOUNT								"BlockCount"							//Number of blocks received from the device.
	

#endif       
             
             
