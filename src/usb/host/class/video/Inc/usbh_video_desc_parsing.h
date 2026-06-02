#ifndef _USBH_VIDEO_DESC_PARSING_H
#define _USBH_VIDEO_DESC_PARSING_H

#include "usbh_video.h"

usbh_status USBH_VIDEO_FindStreamingIN(usbh_host *phost);
usbh_status USBH_VIDEO_ParseCSDescriptors(usbh_host *phost);
usbh_status ParseCSDescriptors(VIDEO_ClassSpecificDescTypedef *class_desc, uint8_t ac_subclass, uint8_t *pdesc);                                                                          
void USBH_VIDEO_AnalyseFormatDescriptors(VIDEO_ClassSpecificDescTypedef *class_desc);
void USBH_VIDEO_AnalyseFrameDescriptors(VIDEO_ClassSpecificDescTypedef *class_desc);

extern USBH_VIDEO_TargetFormat_t USBH_VIDEO_Target_Format;
extern int USBH_VIDEO_Best_bFormatIndex;
extern int USBH_VIDEO_Best_bFrameIndex;

#endif
