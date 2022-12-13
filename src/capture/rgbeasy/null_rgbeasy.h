/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * Parts copyright (c) 2007, 2010 Datapath Ltd.
 *
 * Null wrappers for the RGBEASY API. Useful for building VCS on platforms where
 * the API isn't available - for basic UI devving or the like. Note that by disabling
 * the API, you disable all capture functionality.
 *
 */

#ifndef VCS_CAPTURE_NULL_RGBEASY_H
#define VCS_CAPTURE_NULL_RGBEASY_H

#include "common/globals.h"

#define NULL_RGBEASY_FUNCTION(fn_name)      (DEBUG(("Function called: %s.", fn_name)), RGBERROR_NO_ERROR)
#define NULL_RGBEASY_CONSTANT               -1
#define NULL_RGBEASY_POINTER                void*
#ifndef _WIN32 /// Temp hack.
    #define ULONG_PTR                       unsigned long*
#endif

// Functions.
#define RGBGetCaptureState(...)             NULL_RGBEASY_FUNCTION("RGBGetCaptureState")
#define RGBSetErrorFn(...)                  NULL_RGBEASY_FUNCTION("RGBSetErrorFn")
#define RGBGetOutputSize(...)               NULL_RGBEASY_FUNCTION("RGBGetOutputSize")
#define RGBSetFrameCapturedFn(...)          NULL_RGBEASY_FUNCTION("RGBSetFrameCapturedFn")
#define RGBSetInvalidSignalFn(...)          NULL_RGBEASY_FUNCTION("RGBSetInvalidSignalFn")
#define RGBSetModeChangedFn(...)            NULL_RGBEASY_FUNCTION("RGBSetModeChangedFn")
#define RGBSetNoSignalFn(...)               NULL_RGBEASY_FUNCTION("RGBSetNoSignalFn")
#define RGBCloseInput(...)                  NULL_RGBEASY_FUNCTION("RGBCloseInput")
#define RGBFree(...)                        NULL_RGBEASY_FUNCTION("RGBFree")
#define RGBStartCapture(...)                NULL_RGBEASY_FUNCTION("RGBStartCapture")
#define RGBGetModeInfo(...)                 NULL_RGBEASY_FUNCTION("RGBGetModeInfo")
#define RGBStopCapture(...)                 NULL_RGBEASY_FUNCTION("RGBStopCapture")
#define RGBGetCaptureCard(...)              NULL_RGBEASY_FUNCTION("RGBGetCaptureCard")
#define RGBPauseCapture(...)                NULL_RGBEASY_FUNCTION("RGBPauseCapture")
#define RGBResumeCapture(...)               NULL_RGBEASY_FUNCTION("RGBResumeCapture")
#define RGBGetNumberOfInputs(...)           NULL_RGBEASY_FUNCTION("RGBGetNumberOfInputs")
#define RGBGetCaptureWidthMinimum(...)      NULL_RGBEASY_FUNCTION("RGBGetCaptureWidthMinimum")
#define RGBGetCaptureHeightMinimum(...)     NULL_RGBEASY_FUNCTION("RGBGetCaptureHeightMinimum")
#define RGBGetCaptureWidthMaximum(...)      NULL_RGBEASY_FUNCTION("RGBGetCaptureWidthMaximum")
#define RGBGetCaptureHeightMaximum(...)     NULL_RGBEASY_FUNCTION("RGBGetCaptureHeightMaximum")
#define RGBTestCaptureWidth(...)            NULL_RGBEASY_FUNCTION("RGBTestCaptureWidth")
#define RGBSetCaptureWidth(...)             NULL_RGBEASY_FUNCTION("RGBSetCaptureWidth")
#define RGBSetCaptureHeight(...)            NULL_RGBEASY_FUNCTION("RGBSetCaptureHeight")
#define RGBSetOutputSize(...)               NULL_RGBEASY_FUNCTION("RGBSetOutputSize")
#define RGBSetPhase(...)                    NULL_RGBEASY_FUNCTION("RGBSetPhase")
#define RGBSetBlackLevel(...)               NULL_RGBEASY_FUNCTION("RGBSetBlackLevel")
#define RGBSetHorScale(...)                 NULL_RGBEASY_FUNCTION("RGBSetHorScale")
#define RGBSetHorPosition(...)              NULL_RGBEASY_FUNCTION("RGBSetHorPosition")
#define RGBSetVerPosition(...)              NULL_RGBEASY_FUNCTION("RGBSetVerPosition")
#define RGBSetBrightness(...)               NULL_RGBEASY_FUNCTION("RGBSetBrightness")
#define RGBSetContrast(...)                 NULL_RGBEASY_FUNCTION("RGBSetContrast")
#define RGBSetColourBalance(...)            NULL_RGBEASY_FUNCTION("RGBSetColourBalance")
#define RGBSetFrameDropping(...)            NULL_RGBEASY_FUNCTION("RGBSetFrameDropping")
#define RGBGetFrameRate(...)                NULL_RGBEASY_FUNCTION("RGBGetFrameRate")
#define RGBGetInputSignalType(...)          NULL_RGBEASY_FUNCTION("RGBGetInputSignalType")
#define RGBGetInputInfo(...)                NULL_RGBEASY_FUNCTION("RGBGetInputInfo")
#define RGBGetCaptureWidth(...)             NULL_RGBEASY_FUNCTION("RGBGetCaptureWidth")
#define RGBGetCaptureHeight(...)            NULL_RGBEASY_FUNCTION("RGBGetCaptureHeight")
#define RGBSetInput(...)                    NULL_RGBEASY_FUNCTION("RGBSetInput")
#define RGBLoad(...)                        NULL_RGBEASY_FUNCTION("RGBLoad")
#define RGBOpenInput(...)                   NULL_RGBEASY_FUNCTION("RGBOpenInput")
#define RGBSetDMADirect(...)                NULL_RGBEASY_FUNCTION("RGBSetDMADirect")
#define RGBSetDeinterlace(...)              NULL_RGBEASY_FUNCTION("RGBSetDeinterlace")
#define RGBSetPixelFormat(...)              NULL_RGBEASY_FUNCTION("RGBSetPixelFormat")
#define RGBUseOutputBuffers(...)            NULL_RGBEASY_FUNCTION("RGBUseOutputBuffers")
#define RGBGetBrightness(...)               NULL_RGBEASY_FUNCTION("RGBGetBrightness")
#define RGBGetContrast(...)                 NULL_RGBEASY_FUNCTION("RGBGetContrast")
#define RGBGetColourBalance(...)            NULL_RGBEASY_FUNCTION("RGBGetColourBalance")
#define RGBGetBrightnessMaximum(...)        NULL_RGBEASY_FUNCTION("RGBGetBrightnessMaximum")
#define RGBGetContrastMaximum(...)          NULL_RGBEASY_FUNCTION("RGBGetContrastMaximum")
#define RGBGetColourBalanceMaximum(...)     NULL_RGBEASY_FUNCTION("RGBGetColourBalanceMaximum")
#define RGBGetBrightnessMinimum(...)        NULL_RGBEASY_FUNCTION("RGBGetBrightnessMinimum")
#define RGBGetContrastMinimum(...)          NULL_RGBEASY_FUNCTION("RGBGetContrastMinimum")
#define RGBGetColourBalanceMinimum(...)     NULL_RGBEASY_FUNCTION("RGBGetColourBalanceMinimum")
#define RGBGetBrightnessDefault(...)        NULL_RGBEASY_FUNCTION("RGBGetBrightnessDefault")
#define RGBGetContrastDefault(...)          NULL_RGBEASY_FUNCTION("RGBGetContrastDefault")
#define RGBGetColourBalanceDefault(...)     NULL_RGBEASY_FUNCTION("RGBGetColourBalanceDefault")
#define RGBGetPhase(...)                    NULL_RGBEASY_FUNCTION("RGBGetPhase")
#define RGBGetBlackLevel(...)               NULL_RGBEASY_FUNCTION("RGBGetBlackLevel")
#define RGBGetHorScale(...)                 NULL_RGBEASY_FUNCTION("RGBGetHorScale")
#define RGBGetHorPosition(...)              NULL_RGBEASY_FUNCTION("RGBGetHorPosition")
#define RGBGetVerPosition(...)              NULL_RGBEASY_FUNCTION("RGBGetVerPosition")
#define RGBGetPhaseMaximum(...)             NULL_RGBEASY_FUNCTION("RGBGetPhaseMaximum")
#define RGBGetBlackLevelMaximum(...)        NULL_RGBEASY_FUNCTION("RGBGetBlackLevelMaximum")
#define RGBGetHorScaleMaximum(...)          NULL_RGBEASY_FUNCTION("RGBGetHorScaleMaximum")
#define RGBGetHorPositionMaximum(...)       NULL_RGBEASY_FUNCTION("RGBGetHorPositionMaximum")
#define RGBGetVerPositionMaximum(...)       NULL_RGBEASY_FUNCTION("RGBGetVerPositionMaximum")
#define RGBGetDMADirect(...)                NULL_RGBEASY_FUNCTION("RGBGetDMADirect")
#define RGBGetFrameDroppingMinimum(...)     NULL_RGBEASY_FUNCTION("RGBGetFrameDroppingMinimum")
#define RGBGetFrameDroppingMaximum(...)     NULL_RGBEASY_FUNCTION("RGBGetFrameDroppingMaximum")
#define RGBIsDeinterlaceSupported(...)      NULL_RGBEASY_FUNCTION("RGBIsDeinterlaceSupported")
#define RGBIsDirectDMASupported(...)        NULL_RGBEASY_FUNCTION("RGBIsDirectDMASupported")
#define RGBIsYUVSupported(...)              NULL_RGBEASY_FUNCTION("RGBIsYUVSupported")
#define RGBInputIsVGASupported(...)         NULL_RGBEASY_FUNCTION("RGBInputIsVGASupported")
#define RGBInputIsCompositeSupported(...)   NULL_RGBEASY_FUNCTION("RGBInputIsCompositeSupported")
#define RGBInputIsComponentSupported(...)   NULL_RGBEASY_FUNCTION("RGBInputIsComponentSupported")
#define RGBInputIsDVISupported(...)         NULL_RGBEASY_FUNCTION("RGBInputIsDVISupported")
#define RGBInputIsSVideoSupported(...)      NULL_RGBEASY_FUNCTION("RGBInputIsSVideoSupported")
#define RGBGetPhaseMinimum(...)             NULL_RGBEASY_FUNCTION("RGBGetPhaseMinimum")
#define RGBGetBlackLevelMinimum(...)        NULL_RGBEASY_FUNCTION("RGBGetBlackLevelMinimum")
#define RGBGetHorScaleMinimum(...)          NULL_RGBEASY_FUNCTION("RGBGetHorScaleMinimum")
#define RGBGetHorPositionMinimum(...)       NULL_RGBEASY_FUNCTION("RGBGetHorPositionMinimum")
#define RGBGetVerPositionMinimum(...)       NULL_RGBEASY_FUNCTION("RGBGetVerPositionMinimum")
#define RGBGetPhaseDefault(...)             NULL_RGBEASY_FUNCTION("RGBGetPhaseDefault")
#define RGBGetBlackLevelDefault(...)        NULL_RGBEASY_FUNCTION("RGBGetBlackLevelDefault")
#define RGBGetHorScaleDefault(...)          NULL_RGBEASY_FUNCTION("RGBGetHorScaleDefault")
#define RGBGetHorPositionDefault(...)       NULL_RGBEASY_FUNCTION("RGBGetHorPositionDefault")
#define RGBGetVerPositionDefault(...)       NULL_RGBEASY_FUNCTION("RGBGetVerPositionDefault")

// Constants.
#define RGBERROR_NO_ERROR                   0
#define CAPTURECARD_DGC103                  0
#define CAPTURECARD_DGC133                  1

// Enums.
typedef enum _DEINTERLACE
{
   RGB_DEINTERLACE_WEAVE = 0,
   RGB_DEINTERLACE_BOB = 1,
   RGB_DEINTERLACE_FIELD_0 = 2,
   RGB_DEINTERLACE_FIELD_1 = 3,
}  DEINTERLACE, *PDEINTERLACE;

typedef enum _CAPTURESTATE
{
   RGB_STATE_CAPTURING        = 0,
   RGB_STATE_NOSIGNAL         = 1,
   RGB_STATE_INVALIDSIGNAL    = 2,
   RGB_STATE_PAUSED           = 3,
   RGB_STATE_ERROR            = 4,
}  CAPTURESTATE, *PCAPTURESTATE;

typedef enum _ANALOG_INPUT_TYPE
{
   RGB_TYPE_VGA   = 0,
   RGB_TYPE_VIDEO = 1,
}  ANALOG_INPUT_TYPE, *PANALOG_INPUT_TYPE;

typedef enum _CAPTURECARD
{
   RGB_CAPTURECARD_DGC103     = CAPTURECARD_DGC103,
   RGB_CAPTURECARD_DGC133     = CAPTURECARD_DGC133,
}  CAPTURECARD, *PCAPTURECARD;

typedef enum _SIGNALTYPE
{
   RGB_SIGNALTYPE_NOSIGNAL = 0,
   RGB_SIGNALTYPE_VGA = 1,
   RGB_SIGNALTYPE_DVI = 2,
   RGB_SIGNALTYPE_YPRPB = 3,
   RGB_SIGNALTYPE_COMPOSITE = 4,
   RGB_SIGNALTYPE_SVIDEO = 5,
   RGB_SIGNALTYPE_OUTOFRANGE = 6,
}  SIGNALTYPE, *PSIGNALTYPE;

typedef enum _PIXELFORMAT
{
   RGB_PIXELFORMAT_AUTO = 0,
   RGB_PIXELFORMAT_555  = 1,
   RGB_PIXELFORMAT_565  = 2,
   RGB_PIXELFORMAT_888  = 3,
   RGB_PIXELFORMAT_GREY = 4,
   RGB_PIXELFORMAT_RGB24 = 5,
   RGB_PIXELFORMAT_YUY2 = 6,
}  PIXELFORMAT, *PPIXELFORMAT;

// Structs.
typedef struct
{
    unsigned long Size;
    unsigned long RefreshRate;
    unsigned long LineRate;
    unsigned long TotalNumberOfLines;
    long BInterlaced;
    long BDVI;
    ANALOG_INPUT_TYPE AnalogType;
} RGBMODECHANGEDINFO, *PRGBMODECHANGEDINFO;

typedef struct
{
   unsigned long  Size;
   CAPTURESTATE   State;
   unsigned long  RefreshRate;
   unsigned long  LineRate;
   unsigned long  TotalNumberOfLines;
   long           BInterlaced;
   long           BDVI;
   ANALOG_INPUT_TYPE AnalogType;
}  RGBMODEINFO, *PRGBMODEINFO;

typedef struct tagDriverVer
{
   unsigned long Major;
   unsigned long Minor;
   unsigned long Micro;
   unsigned long Revision;

} RGBDRIVERVER, *PRGBDRIVERVER;

typedef struct tagLocation
{
   unsigned long Bus;
   unsigned long Device;
   unsigned long Function;

} RGBLOCATION, *PRGBLOCATION;

typedef struct tagRGBDevInfo
{
   unsigned long  Size;

   RGBDRIVERVER   Driver;
   RGBLOCATION    Location;
   unsigned long  FirmWare;
   unsigned long  VHDL;
   unsigned long  Identifier[2];

} RGBINPUTINFO, *PRGBINPUTINFO;

// Misc.
#define RGBFRAMECAPTUREDFN                  NULL_RGBEASY_POINTER
#define RGBMODECHANGEDFN                    NULL_RGBEASY_POINTER
#define RGBNOSIGNALFN                       NULL_RGBEASY_POINTER
#define HRGBDLL                             NULL_RGBEASY_POINTER
#define HRGB                                NULL_RGBEASY_POINTER

#endif
