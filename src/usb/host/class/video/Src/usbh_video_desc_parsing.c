//Parsing UVC descriptors

#include "usbh_video_desc_parsing.h"
#include "usbh_conf.h"
#include "usbh_enum.h"
#include "usbh_core.h"

//Target UVC mode
USBH_VIDEO_TargetFormat_t USBH_VIDEO_Target_Format = UVC_CAPTURE_MODE;

int USBH_VIDEO_Target_Width = UVC_TARGET_WIDTH;// Width in pixels
int USBH_VIDEO_Target_Height = UVC_TARGET_HEIGHT;// Height in pixels

// Value of "bFormatIndex" for target settings (set in USBH_VIDEO_AnalyseFormatDescriptors)
int  USBH_VIDEO_Best_bFormatIndex = -1;

// Value of "bFrameIndex" for target settings (set in USBH_VIDEO_AnalyseFrameDescriptors)
int  USBH_VIDEO_Best_bFrameIndex = -1;

/**
  * @brief  Find IN Video Streaming interfaces
  * @param  phost: Host handle
  * @retval USBH Status
  */
usbh_status USBH_VIDEO_FindStreamingIN(usbh_host  *phost)
{
  uint8_t interface, alt_settings;
  usbh_status status = USBH_FAIL ;
  VIDEO_HandleTypeDef *VIDEO_Handle;

  VIDEO_Handle = (VIDEO_HandleTypeDef *) phost->active_class->class_data; 

  // Look For VIDEOSTREAMING IN interface (data FROM camera)
    
#if 1  
  alt_settings = 0;
  interface = 0 ;
  
  for (interface = 0;  interface < USBH_MAX_NUM_INTERFACES ; interface++ )
  {    
    for (alt_settings = 0;  alt_settings < USBH_MAX_ALT_SETTING ; alt_settings++ )
    {
        if((phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].itf_desc.bInterfaceClass == CC_VIDEO) &&
           (phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].itf_desc.bInterfaceSubClass == USB_SUBCLASS_VIDEOSTREAMING))
        {              
              if ((phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].ep_desc[0].bEndpointAddress & 0x81) && // is IN EP
                 (phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].ep_desc[0].wMaxPacketSize > 0))
              {  
                printf("Current interface = %d alt_settings = %d \r\n",interface,alt_settings);
                VIDEO_Handle->stream_in[alt_settings].Ep = phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].ep_desc[0].bEndpointAddress;
                VIDEO_Handle->stream_in[alt_settings].EpSize = phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].ep_desc[0].wMaxPacketSize;
                VIDEO_Handle->stream_in[alt_settings].interface = phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].itf_desc.bInterfaceNumber;        
                VIDEO_Handle->stream_in[alt_settings].AltSettings = phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].itf_desc.bAlternateSetting;
                VIDEO_Handle->stream_in[alt_settings].Poll = phost->dev_prop.cfg_desc_set.itf_desc_set[interface][alt_settings].ep_desc[0].bInterval;   
                VIDEO_Handle->stream_in[alt_settings].valid = 1; 
  
            
//                VIDEO_Handle->stream_in[alt_settings].Ep = 0x81;
//                VIDEO_Handle->stream_in[alt_settings].EpSize = 3060;
//                VIDEO_Handle->stream_in[alt_settings].interface = 1;        
//                VIDEO_Handle->stream_in[alt_settings].AltSettings = 0;
//                VIDEO_Handle->stream_in[alt_settings].Poll = 1;   
//                VIDEO_Handle->stream_in[alt_settings].valid = 1; 
//                alt_settings++;  
              }      
        }
    }
  }
  
                    VIDEO_Handle->stream_in[1].Ep =0x81;
                VIDEO_Handle->stream_in[1].EpSize = 0x3B0;
                VIDEO_Handle->stream_in[1].interface = 0x01;        
                VIDEO_Handle->stream_in[1].AltSettings = 0x01;
                VIDEO_Handle->stream_in[1].Poll = 0x01;   
                VIDEO_Handle->stream_in[1].valid = 1; 
  
         alt_settings = 1 ;
#else
  alt_settings = 0;
  for (interface = 0;  interface < USBH_MAX_NUM_INTERFACES ; interface ++ )
  {
    if((phost->dev_prop.cfg_desc_set.itf_desc_set[interface].itf_desc.bInterfaceClass == CC_VIDEO) &&
       (phost->dev_prop.cfg_desc_set.itf_desc_set[interface].itf_desc.bInterfaceSubClass == USB_SUBCLASS_VIDEOSTREAMING))
    {
      if((phost->dev_prop.cfg_desc_set.itf_desc_set[interface].ep_desc[0].bEndpointAddress& 0x81) && // is IN EP
         (phost->dev_prop.cfg_desc_set.itf_desc_set[interface].ep_desc[0].wMaxPacketSize > 0))
      {
        VIDEO_Handle->stream_in[alt_settings].Ep = phost->dev_prop.cfg_desc_set.itf_desc_set[interface].ep_desc[0].bEndpointAddress;
        VIDEO_Handle->stream_in[alt_settings].EpSize = phost->dev_prop.cfg_desc_set.itf_desc_set[interface].ep_desc[0].wMaxPacketSize;
        VIDEO_Handle->stream_in[alt_settings].interface = phost->dev_prop.cfg_desc_set.itf_desc_set[interface].itf_desc.bInterfaceNumber;        
        VIDEO_Handle->stream_in[alt_settings].AltSettings = phost->dev_prop.cfg_desc_set.itf_desc_set[interface].itf_desc.bAlternateSetting;
        VIDEO_Handle->stream_in[alt_settings].Poll = phost->dev_prop.cfg_desc_set.itf_desc_set[interface].ep_desc[0].bInterval;   
        VIDEO_Handle->stream_in[alt_settings].valid = 1; 
        alt_settings++;
      }
    }
  }
#endif 

  if(alt_settings > 0)
  {  
     status = USBH_OK;
  }
  
  return status;
}


/**
  * @brief  Parse VC and interfaces Descriptors
  * @param  phost: Host handle
  * @retval USBH Status
  */
usbh_status USBH_VIDEO_ParseCSDescriptors(usbh_host *phost)
{
  //Pointer to header of descriptor
  usb_desc_header            *pdesc ;
  uint16_t                      ptr;
  int8_t                        itf_index = 0;
  int8_t                        itf_number = 0; 
  int8_t                        alt_setting;   
  VIDEO_HandleTypeDef           *VIDEO_Handle;
  
  VIDEO_Handle =  (VIDEO_HandleTypeDef *)phost->active_class->class_data;  
  pdesc   = (usb_desc_header *)(phost->dev_prop.cfgdesc_rawdata);
  ptr = USB_LEN_CFG_DESC;
  
  VIDEO_Handle->class_desc.InputTerminalNum = 0;
  VIDEO_Handle->class_desc.OutputTerminalNum = 0;  
  VIDEO_Handle->class_desc.ASNum = 0;
  
  //while(ptr < phost->dev_prop.CfgDesc.wTotalLength)
  while(ptr < phost->dev_prop.cfg_desc_set.cfg_desc.wTotalLength)    
  {
      pdesc = usbh_nextdesc_get((uint8_t *)pdesc, &ptr);
      
      switch (pdesc->bDescriptorType)
      {      
          case USB_DESC_TYPE_INTERFACE:
            itf_number = *((uint8_t *)pdesc + 2);//bInterfaceNumber
            alt_setting = *((uint8_t *)pdesc + 3);
            itf_index = usbh_interfaceindex_find(&phost->dev_prop, itf_number, alt_setting);
            break;        
          case USB_DESC_TYPE_CS_INTERFACE://0x24 - Class specific descriptor 
            //if(itf_number <= phost->device.CfgDesc.bNumInterfaces)
            if(itf_number <= phost->dev_prop.cfg_desc_set.cfg_desc.bNumInterfaces)  
            {            
                ParseCSDescriptors(&VIDEO_Handle->class_desc,
                                   //phost->device.CfgDesc.Itf_Desc[itf_index].bInterfaceSubClass,
                                   phost->dev_prop.cfg_desc_set.itf_desc_set[itf_index][0].itf_desc.bInterfaceSubClass,
                                   (uint8_t *)pdesc);
            }
            break;
            
          default:
            break; 
      }
  }
  return USBH_OK;
}

/**
  * @brief  Parse Class specific descriptor
  * @param  vs_subclass: bInterfaceSubClass
  * pdesc: current desctiptor
  * @retval USBH Status
  */
usbh_status ParseCSDescriptors(VIDEO_ClassSpecificDescTypedef *class_desc, 
                                      uint8_t vs_subclass, 
                                      uint8_t *pdesc)
{
  uint8_t desc_number = 0;
    
  if(vs_subclass == USB_SUBCLASS_VIDEOCONTROL)
  {
    switch(pdesc[2])
    {
    case UVC_VC_HEADER: 
      class_desc->cs_desc.HeaderDesc = (VIDEO_HeaderDescTypeDef *)pdesc;
      break;
      
    case UVC_VC_INPUT_TERMINAL:
      class_desc->cs_desc.InputTerminalDesc[class_desc->InputTerminalNum++] = (VIDEO_ITDescTypeDef*) pdesc;    
      break;
      
    case UVC_VC_OUTPUT_TERMINAL:
      class_desc->cs_desc.OutputTerminalDesc[class_desc->OutputTerminalNum++] = (VIDEO_OTDescTypeDef*) pdesc;   
      break;
      
    case UVC_VC_SELECTOR_UNIT:
      class_desc->cs_desc.SelectorUnitDesc[class_desc->SelectorUnitNum++] = (VIDEO_SelectorDescTypeDef*) pdesc; 
      break;    

    default: 
      break;
    }
  }
  else if(vs_subclass == USB_SUBCLASS_VIDEOSTREAMING)
  {
    switch(pdesc[2])
    {      
    case UVC_VS_INPUT_HEADER:
      if (class_desc->InputHeaderNum < VIDEO_MAX_NUM_IN_HEADER)
        class_desc->vs_desc.InputHeader[class_desc->InputHeaderNum++] = (VIDEO_InHeaderDescTypeDef*) pdesc; 
      break;
      
    //***** MJPEG *****
      
    case UVC_VS_FORMAT_MJPEG:
      if (class_desc->MJPEGFormatNum < VIDEO_MAX_MJPEG_FORMAT)
        class_desc->vs_desc.MJPEGFormat[class_desc->MJPEGFormatNum++] = (VIDEO_MJPEGFormatDescTypeDef*) pdesc; 
      break;
      
    case UVC_VS_FRAME_MJPEG:
      desc_number = class_desc->MJPEGFrameNum; 
      
      if (desc_number < VIDEO_MAX_MJPEG_FRAME_D)
      {
        class_desc->vs_desc.MJPEGFrame[desc_number] = (VIDEO_MJPEGFrameDescTypeDef*) pdesc;
//        uint16_t width = LE16(class_desc->vs_desc.MJPEGFrame[desc_number]->wWidth);
//        uint16_t height = LE16(class_desc->vs_desc.MJPEGFrame[desc_number]->wHeight);
        
        //USBH_DbgLog("MJPEG Frame detected: %d x %d", width, height);
        class_desc->MJPEGFrameNum++;
      }
      break;
      
    //***** UNCOMPRESSED ***** 
      
    case UVC_VS_FORMAT_UNCOMPRESSED:
      if (class_desc->UncompFormatNum < VIDEO_MAX_UNCOMP_FORMAT)
        class_desc->vs_desc.UncompFormat[class_desc->UncompFormatNum++] = (VIDEO_UncompFormatDescTypeDef*) pdesc; 
      break;
      
      //-------
      
    case UVC_VS_FRAME_UNCOMPRESSED:
      desc_number = class_desc->UncompFrameNum; 
      
      if (desc_number < VIDEO_MAX_UNCOMP_FRAME_D)
      {
        class_desc->vs_desc.UncompFrame[desc_number] = (VIDEO_UncompFrameDescTypeDef*) pdesc;
        uint16_t width = LE16(class_desc->vs_desc.UncompFrame[desc_number]->wWidth);
        uint16_t height = LE16(class_desc->vs_desc.UncompFrame[desc_number]->wHeight);
        
        USBH_DbgLog("Uncompressed Frame detected: %d x %d", width, height);
        class_desc->UncompFrameNum++;
      }
      break;
      
    default:
      break;
    }
  }
 
  return USBH_OK;
}

/*
 * Check if camera have needed Format descriptor (base for MJPEG/Uncompressed frames)
 */
void USBH_VIDEO_AnalyseFormatDescriptors(VIDEO_ClassSpecificDescTypedef *class_desc)
{
  USBH_VIDEO_Best_bFormatIndex = -1;
  
  if (USBH_VIDEO_Target_Format == USBH_VIDEO_MJPEG)
  {
    if (class_desc->MJPEGFormatNum != 1)
    {
      USBH_ErrLog("Not supported MJPEG descriptors number: %d", class_desc->MJPEGFormatNum);
    }
    else
    {
      VIDEO_MJPEGFormatDescTypeDef* mjpeg_format_desc;
      mjpeg_format_desc = class_desc->vs_desc.MJPEGFormat[0];
      USBH_VIDEO_Best_bFormatIndex = mjpeg_format_desc->bFormatIndex;
    }
    return;
  }
  else if (USBH_VIDEO_Target_Format == USBH_VIDEO_YUY2)
  {
    if (class_desc->UncompFormatNum != 1)
    {
      USBH_ErrLog("Not supported UNCOMP descriptors number: %d", class_desc->UncompFormatNum);
      return;
    }
    else
    {
      //Camera have a single Format descriptor, so we need to check if this descriptor is really YUY2
      VIDEO_UncompFormatDescTypeDef* uncomp_format_desc;
      uncomp_format_desc = class_desc->vs_desc.UncompFormat[0];
      
      if (memcmp(&uncomp_format_desc->guidFormat, "YUY2", 4) != 0)
      {
        USBH_ErrLog("Not supported UNCOMP descriptor type");
        return;
      }
      else
      {
        // Found!
        USBH_VIDEO_Best_bFormatIndex = uncomp_format_desc->bFormatIndex;
      }
    }
  }
}

/*
 * Check if camera have needed Frame descriptor (whith target image width)
 */
void USBH_VIDEO_AnalyseFrameDescriptors(VIDEO_ClassSpecificDescTypedef *class_desc)
{
  USBH_VIDEO_Best_bFrameIndex = -1;
    
  if (USBH_VIDEO_Target_Format == USBH_VIDEO_MJPEG)
  {
    for (uint8_t i = 0; i < class_desc->MJPEGFrameNum; i++)
    {
      VIDEO_MJPEGFrameDescTypeDef* mjpeg_frame_desc;
      mjpeg_frame_desc = class_desc->vs_desc.MJPEGFrame[i];
      if ((LE16(mjpeg_frame_desc->wWidth) == USBH_VIDEO_Target_Width) && \
        (LE16(mjpeg_frame_desc->wHeight) == USBH_VIDEO_Target_Height))
      {
        //Found!
        USBH_VIDEO_Best_bFrameIndex = mjpeg_frame_desc->bFrameIndex;
      }
    }
  }
  else if (USBH_VIDEO_Target_Format == USBH_VIDEO_YUY2)
  {
    for (uint8_t i = 0; i < class_desc->UncompFrameNum; i++)
    {
      VIDEO_UncompFrameDescTypeDef* uncomp_frame_desc;
      uncomp_frame_desc = class_desc->vs_desc.UncompFrame[i];
      if ((LE16(uncomp_frame_desc->wWidth) == USBH_VIDEO_Target_Width) && \
        (LE16(uncomp_frame_desc->wHeight) == USBH_VIDEO_Target_Height))
      {
        //Found!
        USBH_VIDEO_Best_bFrameIndex = uncomp_frame_desc->bFrameIndex;
      }
    }
  }
}
