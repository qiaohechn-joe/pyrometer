
/* Define to prevent recursive  ----------------------------------------------*/
#ifndef __USBH_VIDEO_H
#define __USBH_VIDEO_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"
#include <string.h>
//Maximum endpoint size in bytes
#define UVC_RX_FIFO_SIZE_LIMIT          1024
   
//Image width
#define UVC_TARGET_WIDTH                320
#define UVC_TARGET_HEIGHT               240
   
#define UVC_CAPTURE_MODE                USBH_VIDEO_MJPEG
//#define UVC_CAPTURE_MODE                USBH_VIDEO_YUY2
   
// Uncompressed image frame size in byte
#define UVC_UNCOMP_FRAME_SIZE           (UVC_TARGET_WIDTH * UVC_TARGET_HEIGHT * 2)
   
#define UVC_MAX_FRAME_SIZE              UVC_UNCOMP_FRAME_SIZE
//TODO - UVC_MAX_FRAME_SIZE for MJPEG mode can be smaller.
//Needed value is send by camera - see "USBH_VS_GetCur" - dwMaxVideoFrameSize

#define HOST_USER_SELECT_CONFIGURATION          1
#define HOST_USER_CLASS_ACTIVE                  2
#define HOST_USER_CLASS_SELECTED                3
#define HOST_USER_CONNECTION                    4
#define HOST_USER_DISCONNECTION                 5
#define HOST_USER_UNRECOVERED_ERROR             6
#define HOST_USER_SET_BACKLIGHT                 7
#define HOST_USER_SET_GAIN                      8

typedef enum {
  APPLICATION_IDLE = 0,  
  APPLICATION_START,   
  APPLICATION_READY,
  APPLICATION_DISCONNECT,
}ApplicationTypeDef;

typedef enum
{
  USBH_VIDEO_MJPEG = 0,
  USBH_VIDEO_YUY2,
} USBH_VIDEO_TargetFormat_t;

/* States for VIDEO State Machine */
typedef enum
{
  VIDEO_INIT = 0, 
  VIDEO_IDLE,   
  VIDEO_CS_REQUESTS,
  VIDEO_SET_DEFAULT_FEATURE_UNIT,
  VIDEO_SET_INTERFACE,
  VIDEO_SET_STREAMING_INTERFACE,
  VIDEO_SET_CUR1,
  VIDEO_GET_RES,
  VIDEO_GET_CUR1,
  VIDEO_SET_CUR2,
  VIDEO_GET_CUR2,
  VIDEO_SET_CUR3,
  VIDEO_SET_INTERFACE0,
  VIDEO_SET_INTERFACE1,
  VIDEO_SET_INTERFACE2,
  VIDEO_ISOC_OUT,        
  VIDEO_ISOC_IN,  
  VIDEO_ISOC_POLL,
  VIDEO_ERROR,  
}
VIDEO_StateTypeDef;

typedef enum
{
  VIDEO_REQ_INIT = 1,
  VIDEO_REQ_IDLE, 
  VIDEO_REQ_SET_DEFAULT_IN_INTERFACE,
  VIDEO_REQ_SET_IN_INTERFACE,
  VIDEO_REQ_CS_REQUESTS,
}
VIDEO_ReqStateTypeDef;

typedef enum
{
 VIDEO_CONTROL_INIT = 1,
 VIDEO_CONTROL_CHANGE,
 VIDEO_CONTROL_IDLE, 
}
VIDEO_ControlStateTypeDef;

typedef enum
{
  VIDEO_STATE_IDLE = 1,
  VIDEO_STATE_START_IN,
  VIDEO_STATE_DATA_IN,
}
VIDEO_StreamStateTypeDef;


typedef struct
{
  uint8_t              Ep;//bEndpointAddress
  uint16_t             EpSize;//wMaxPacketSize
  uint8_t              AltSettings;//bAlternateSetting
  uint8_t              interface;//bInterfaceNumber
  uint8_t              valid; 
  uint16_t             Poll;//bInterval
}
VIDEO_STREAMING_IN_HandleTypeDef;

typedef struct
{
  uint8_t              Ep;
  uint16_t             EpSize; 
  uint8_t              interface; 
  uint8_t              AltSettings;
  uint8_t              supported;    

  uint8_t              Pipe;  
  uint8_t              Poll; 
  uint32_t             timer ; 
  
  uint8_t              asociated_as;
  
  uint8_t              *buf;
  uint8_t              *cbuf; 
  uint32_t             partial_ptr; 

  uint32_t             global_ptr;  
  uint16_t             frame_length;  
  uint32_t             total_length;  
}
VIDEO_InterfaceStreamPropTypeDef;



#define VIDEO_MAX_VIDEO_STD_INTERFACE      4

// Video Control Descriptor
#define VIDEO_MAX_NUM_IN_TERMINAL          1
#define VIDEO_MAX_NUM_OUT_TERMINAL         1
#define VIDEO_MAX_NUM_FEATURE_UNIT         1
#define VIDEO_MAX_NUM_SELECTOR_UNIT        1

// Video Steream Descriptor
#define VIDEO_MAX_NUM_IN_HEADER            1

// Video Steream Descriptor
#define VIDEO_MAX_MJPEG_FORMAT             1
#define VIDEO_MAX_MJPEG_FRAME_D            1

#define VIDEO_MAX_UNCOMP_FORMAT            1
#define VIDEO_MAX_UNCOMP_FRAME_D           1

#define VIDEO_MAX_SAMFREQ_NBR              5
#define VIDEO_MAX_INTERFACE_NBR            5
#define VIDEO_MAX_CONTROLS_NBR             5

#define VS_PROBE_CONTROL                   0x01
#define VS_COMMIT_CONTROL                  0x02

/*  Class-Specific VC Header Descriptor */
typedef struct
{
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  bDescriptorSubtype;//must be UVC_VC_HEADER
  uint8_t  bcdUVC[2];
  uint8_t  wTotalLength[2];//header+units+terminals
  uint8_t  dwClockFrequency[4];
  uint8_t  bInCollection;
  uint8_t  baInterfaceNr[VIDEO_MAX_INTERFACE_NBR];
} 
VIDEO_HeaderDescTypeDef;

/* VC Input Terminal Descriptor */
typedef struct 
{
  uint8_t  bLength;           
  uint8_t  bDescriptorType;
  uint8_t  bDescriptorSubtype;//must be UVC_VC_INPUT_TERMINAL
  uint8_t  bTerminalID;
  uint8_t  wTerminalType[2];//must be 0x0201 = (ITT_CAMERA)
  uint8_t  bAssocTerminal;
  uint8_t  iTerminal;
  uint8_t  wObjectiveFocalLengthMin[2];
  uint8_t  wObjectiveFocalLengthMax[2];
  uint8_t  wOcularFocalLength[2];
  uint8_t  bControlSize;
  uint8_t  bmControls[3];//in fact, size of this array if defined by "bControlSize" value
}
VIDEO_ITDescTypeDef;

/* VC Output Terminal Descriptor */
typedef struct 
{
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  bDescriptorSubtype;//must be UVC_VC_INPUT_TERMINAL      
  uint8_t  bTerminalID;
  uint8_t  wTerminalType[2];
  uint8_t  bAssocTerminal;
  uint8_t  bSourceID;
  uint8_t  iTerminal;
}
VIDEO_OTDescTypeDef;

/* Feature Descriptor */
typedef struct
{                                                             
  uint8_t  bLength;                                        
  uint8_t  bDescriptorType;                                
  uint8_t  bDescriptorSubtype;                             
  uint8_t  bUnitID;                                        
  uint8_t  bSourceID;                                      
  uint8_t  bControlSize;                                   
  uint8_t  bmaControls[VIDEO_MAX_CONTROLS_NBR][2];                                                                 
}
VIDEO_FeatureDescTypeDef;

/*  */
typedef struct
{                                                             
  uint8_t  bLength;                                        
  uint8_t  bDescriptorType;                                
  uint8_t  bDescriptorSubtype;                             
  uint8_t  bUnitID;                                        
  uint8_t  bNrInPins; 
  uint8_t  bSourceID0;  
  uint8_t  iSelector;                                                                   
}
VIDEO_SelectorDescTypeDef;


//**********************************************************************
//Video Stream Descriptors

/* VS Input Header Descriptor */
typedef struct 
{
  uint8_t  bLength;           
  uint8_t  bDescriptorType;
  uint8_t  bDescriptorSubtype;//must be UVC_VS_INPUT_HEADER
  uint8_t  bNumFormats;
  uint8_t  wTotalLength[2];
  uint8_t  bEndPointAddress;
  uint8_t  bmInfo;
  uint8_t  bTerminalLink;
  uint8_t  bStillCaptureMethod;
  uint8_t  bTriggerSupport;
  uint8_t  bTriggerUsage;
  uint8_t  bControlSize;
  uint8_t  bmaControls;
}
VIDEO_InHeaderDescTypeDef;


/* VS MJPEG Format Descriptor */
typedef struct
{
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubtype;
  uint8_t bFormatIndex;
  uint8_t bNumFrameDescriptors;
  uint8_t bmFlags;
  uint8_t bDefaultFrameIndex;
  uint8_t bAspectRatioX;
  uint8_t bAspectRatioY;
  uint8_t bmInterlaceFlags;
  uint8_t bCopyProtect;
} VIDEO_MJPEGFormatDescTypeDef;


/* VS MJPEG Frame Descriptor */
typedef struct 
{
  uint8_t  bLength;           
  uint8_t  bDescriptorType;
  uint8_t  bDescriptorSubtype;//must be UVC_VS_INPUT_HEADER
  uint8_t  bFrameIndex;
  uint8_t  bmCapabilities;
  uint8_t  wWidth[2];
  uint8_t  wHeight[2];
  uint8_t  dwMinBitRate[4];
  uint8_t  dwMaxBitRate[4];
  uint8_t  dwDefaultFrameInterval[4];
  uint8_t  bFrameIntervalType;
  //dwFrameInterval*N is here
}
VIDEO_MJPEGFrameDescTypeDef;

/* VS Uncompressed Format Typ Descriptor */
typedef struct 
{
  uint8_t  bLength;           
  uint8_t  bDescriptorType;
  uint8_t  bDescriptorSubtype;//must be UVC_VS_INPUT_HEADER
  uint8_t  bFormatIndex;
  uint8_t  bNumFrameDescriptors;
  uint8_t  guidFormat[16];
  uint8_t  bBitsPerPixel;
  uint8_t  bDefaultFrameIndex;
  uint8_t  bAspectRatioX;
  uint8_t  bAspectRatioY;
  uint8_t  bmInterfaceFlags;
  uint8_t  bCopyProtect;
}
VIDEO_UncompFormatDescTypeDef;

/* VS Uncompressed Frame Descriptor */
typedef struct 
{
  uint8_t  bLength;           
  uint8_t  bDescriptorType;
  uint8_t  bDescriptorSubtype;//must be UVC_VS_INPUT_HEADER
  uint8_t  bFrameIndex;
  uint8_t  bmCapabilities;
  uint8_t  wWidth[2];
  uint8_t  wHeight[2];
  uint8_t  dwMinBitRate[4];
  uint8_t  dwMaxBitRate[4];
  uint8_t  dwDefaultFrameInterval[4];
  uint8_t  bFrameIntervalType;
  //dwFrameInterval*N is here
}
VIDEO_UncompFrameDescTypeDef;



/* Class-Specific VC (Video Control) Interface Descriptor*/
typedef struct
{
  VIDEO_HeaderDescTypeDef   *HeaderDesc;
  VIDEO_ITDescTypeDef       *InputTerminalDesc [VIDEO_MAX_NUM_IN_TERMINAL];
  VIDEO_OTDescTypeDef       *OutputTerminalDesc[VIDEO_MAX_NUM_OUT_TERMINAL];
  VIDEO_FeatureDescTypeDef  *FeatureUnitDesc   [VIDEO_MAX_NUM_FEATURE_UNIT];
  VIDEO_SelectorDescTypeDef *SelectorUnitDesc  [VIDEO_MAX_NUM_SELECTOR_UNIT];
}
VIDEO_VCDescTypeDef;


/* Class-Specific VC (Video Control) Interface Descriptor*/
typedef struct
{
  VIDEO_InHeaderDescTypeDef     *InputHeader[VIDEO_MAX_NUM_IN_HEADER];
  
  VIDEO_MJPEGFormatDescTypeDef  *MJPEGFormat[VIDEO_MAX_MJPEG_FORMAT];
  VIDEO_MJPEGFrameDescTypeDef   *MJPEGFrame[VIDEO_MAX_MJPEG_FRAME_D];
  
  VIDEO_UncompFormatDescTypeDef *UncompFormat[VIDEO_MAX_UNCOMP_FORMAT];
  VIDEO_UncompFrameDescTypeDef  *UncompFrame[VIDEO_MAX_MJPEG_FRAME_D];
  
}
VIDEO_VSDescTypeDef;

typedef struct
{
  VIDEO_VCDescTypeDef   cs_desc; /* Only one control descriptor*/
  VIDEO_VSDescTypeDef   vs_desc;

  uint16_t ASNum;
  uint16_t InputTerminalNum;
  uint16_t OutputTerminalNum;
  uint16_t SelectorUnitNum;
  
  uint8_t InputHeaderNum;
  
  uint8_t MJPEGFormatNum;
  uint8_t MJPEGFrameNum;
  
  uint8_t UncompFormatNum;
  uint8_t UncompFrameNum;
}
VIDEO_ClassSpecificDescTypedef;

//****************************************************************************

//UVC 1.0 uses only 26 first bytes       01 00 01 01 80 84 1e 00 00 00 00 00 00 00 00 00 00 00 q1 b0 03 00 00   
   //00 00 01 01 40 42 0f 00 00 00 00 00 2f 00 00 00 00 00 00 58 02 00 b0 03 00 00

typedef __packed struct 
{
  uint16_t    bmHint;                      // 2
  uint8_t     bFormatIndex;                // 3
  uint8_t     bFrameIndex;                 // 4
  uint32_t    dwFrameInterval;             // 8
  uint16_t    wKeyFrameRate;               // 10
  uint16_t    wPFrameRate;                 // 12
  uint16_t    wCompQuality;                // 14
  uint16_t    wCompWindowSize;             // 16
  uint16_t    wDelay;                      // 18
  uint32_t    dwMaxVideoFrameSize;         // 22
  uint32_t    dwMaxPayloadTransferSize;    // 26
  uint32_t    dwClockFrequency;
  uint8_t     bmFramingInfo;
  uint8_t     bPreferedVersion;
  uint8_t     bMinVersion;
  uint8_t     bMaxVersion;
} VIDEO_ProbeTypedef;

typedef struct _VIDEO_Process
{
  VIDEO_ReqStateTypeDef              req_state;  
  VIDEO_ControlStateTypeDef          control_state; 
  VIDEO_StreamStateTypeDef           steam_in_state;
  
  VIDEO_STREAMING_IN_HandleTypeDef   stream_in[3];
  VIDEO_ClassSpecificDescTypedef     class_desc;
  
  VIDEO_InterfaceStreamPropTypeDef   camera;
  uint16_t                           mem[8];  
  uint8_t                            temp_feature;
}
VIDEO_HandleTypeDef;

/**
  * @}
  */ 

/** @defgroup USBH_VIDEO_CORE_Exported_Defines
  * @{
  */ 


/*Video Interface Subclass Codes*/
#define CC_VIDEO                        0x0E

/* Video Interface Subclass Codes */
#define USB_SUBCLASS_VIDEOCONTROL  0x01
#define USB_SUBCLASS_VIDEOSTREAMING  0x02
#define USB_SUBCLASS_VIDEO_INTERFACE_COLLECTION         0x03

//Class specific 
#define USB_DESC_TYPE_CS_INTERFACE      0x24
#define USB_DESC_TYPE_CS_ENDPOINT       0x25

// Video Class-Specific VideoControl Interface Descriptor Subtypes
// (USB_Video_Class_1.1.pdf, A.5 Video Class-Specific VC Interface Descriptor Subtypes)
#define UVC_VC_HEADER      0x01
#define UVC_VC_INPUT_TERMINAL    0x02
#define UVC_VC_OUTPUT_TERMINAL    0x03
#define UVC_VC_SELECTOR_UNIT    0x04
#define UVC_VC_PROCESSING_UNIT    0x05
#define UVC_VC_EXTENSION_UNIT    0x06

// Video Class-Specific VideoStreaming Interface Descriptor Subtypes
// (USB_Video_Class_1.1.pdf, A.6 Video Class-Specific VS Interface Descriptor Subtypes)
#define UVC_VS_UNDEFINED                0x00
#define UVC_VS_INPUT_HEADER             0x01
#define UVC_VS_OUTPUT_HEADER            0x02
#define UVC_VS_STILL_IMAGE_FRAME        0x03
#define UVC_VS_FORMAT_UNCOMPRESSED      0x04
#define UVC_VS_FRAME_UNCOMPRESSED       0x05
#define UVC_VS_FORMAT_MJPEG             0x06
#define UVC_VS_FRAME_MJPEG              0x07
#define UVC_VS_FORMAT_MPEG2TS           0x0A
#define UVC_VS_FORMAT_DV                0x0C
#define UVC_VS_COLORFORMAT              0x0D
#define UVC_VS_FORMAT_FRAME_BASED       0x10
#define UVC_VS_FRAME_FRAME_BASED        0x11
#define UVC_VS_FORMAT_STREAM_BASED      0x12

#define UVC_AS_GENERAL      0x01
#define UVC_FORMAT_TYPE      0x02
#define UVC_FORMAT_SPECIFIC    0x03

/* Video Class-Specific Endpoint Descriptor Subtypes */
#define UVC_EP_GENERAL      0x01

/* Video Class-Specific Request Codes */
#define UVC_SET_      0x00
#define UVC_GET_      0x80

#define UVC__CUR      0x1
#define UVC__MIN      0x2
#define UVC__MAX      0x3
#define UVC__RES      0x4
#define UVC__MEM      0x5

#define EXPO_MODE_AUTO      0x08
#define EXPO_MODE_MANUAL    0x01


#define UVC_SET_CUR      (UVC_SET_ | UVC__CUR)
#define UVC_GET_CUR      (UVC_GET_ | UVC__CUR)
#define UVC_SET_MIN      (UVC_SET_ | UVC__MIN)
#define UVC_GET_MIN      (UVC_GET_ | UVC__MIN)
#define UVC_SET_MAX      (UVC_SET_ | UVC__MAX)
#define UVC_GET_MAX      (UVC_GET_ | UVC__MAX)
#define UVC_SET_RES      (UVC_SET_ | UVC__RES)
#define UVC_GET_RES      (UVC_GET_ | UVC__RES)
#define UVC_SET_MEM      (UVC_SET_ | UVC__MEM)
#define UVC_GET_MEM      (UVC_GET_ | UVC__MEM)

#define UVC_GET_STAT      0xff

/* Terminals - 2.1 USB Terminal Types */
#define UVC_TERMINAL_UNDEFINED    0x100
#define UVC_TERMINAL_STREAMING    0x101
#define UVC_TERMINAL_VENDOR_SPEC  0x1FF

#define  UVC_CTL_SET_EM_MODE   1  
#define  UVC_CTL_SET_EM_VAL    2
#define  UVC_CTL_SET_BN_VAL    3
#define  UVC_CTL_SET_BL_VAL    4
#define  UVC_CTL_SET_GN_VAL    5
#define  UVC_CTL_GET_EM_MODE   6
#define  UVC_CTL_GET_EM_VAL    7
#define  UVC_CTL_GET_BN_VAL    8
#define  UVC_CTL_GET_BL_VAL    9
#define  UVC_CTL_GET_GN_VAL    10

/**
  * @}
  */ 

/** @defgroup USBH_VIDEO_CORE_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBH_VIDEO_CORE_Exported_Variables
  * @{
  */ 
extern usbh_class  VIDEO_Class ;
#define USBH_VIDEO_CLASS  &VIDEO_Class

extern usb_core_driver usbh_core;
extern usbh_host usb_host_uvc;
extern const uint32_t exposure_val[];
/**
  * @}
  */ 

/** @defgroup USBH_VIDEO_CORE_Exported_FunctionsPrototype
  * @{
  */ 
usbh_status USBH_VIDEO_SetFrequency (usbh_host  *phost, 
                                            uint16_t sample_rate,
                                            uint8_t  channel_num,
                                            uint8_t data_width);

usbh_status USBH_VS_SetCur(usbh_host  *phost, uint16_t request_type);
usbh_status USBH_VS_GetCur_test(usbh_host *phost, uint16_t request_type);
usbh_status USBH_VS_GetCur(usbh_host  *phost, uint16_t request_type);

usbh_status uvc_set_exposure_mode(usbh_host *phost, uint8_t mode);
usbh_status uvc_get_exposure_mode(usbh_host *phost,uint8_t *mode);
usbh_status uvc_set_exposure(usbh_host *phost, uint32_t exposure_val);
//usbh_status uvc_get_exposure(usbh_host *phost, uint32_t exposure_val);
usbh_status uvc_get_exposure(usbh_host *phost, float exposure_val);
usbh_status uvc_set_brightness(usbh_host *phost, int brightness_val);
int calc_exposure_idx(int value);

void USBH_UserProcess  (usbh_host *phost, uint8_t id);
int USBH_VIDEO_Process(usbh_host *phost);
//usbh_status USBH_VIDEO_Process(usbh_host  *phost);

void timer11_handler(void);
void data_send_task(void *arg);

void uvc_ctl_task(void *arg);
/**
  * @}
  */ 

#ifdef __cplusplus
}
#endif

#endif /* __USBH_VIDEO_H */

