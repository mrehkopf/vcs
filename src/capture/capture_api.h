/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Provides a public interface for interacting with the capture device.
 *
 */

#include <string>
#include "common/globals.h"
#include "display/display.h"
#include "capture/capture.h"

struct capture_api_s
{
    virtual bool initialize(void)    = 0;
    virtual bool start_capture(void) = 0;
    virtual bool stop_capture(void)  = 0;

    // Capture device/API capabilities.
    virtual bool device_supports_component_capture(void) const { return false; }
    virtual bool device_supports_composite_capture(void) const { return false; }
    virtual bool device_supports_deinterlacing(void)     const { return false; }
    virtual bool device_supports_svideo(void)            const { return false; }
    virtual bool device_supports_dma(void)               const { return false; }
    virtual bool device_supports_dvi(void)               const { return false; }
    virtual bool device_supports_vga(void)               const { return false; }
    virtual bool device_supports_yuv(void)               const { return false; }

    // Getters.
    virtual std::string              get_device_firmware_version(void) const = 0;
    virtual std::string              get_device_driver_version(void)   const = 0;
    virtual std::string              get_device_name(void)             const = 0;
    virtual std::string              get_api_name(void)                const = 0;
    virtual int                      get_maximum_input_count(void)     const = 0;
    virtual capture_color_settings_s get_color_settings(void)          const = 0;
    virtual capture_color_settings_s get_default_color_settings(void)  const = 0;
    virtual capture_color_settings_s get_minimum_color_settings(void)  const = 0;
    virtual capture_color_settings_s get_maximum_color_settings(void)  const = 0;
    virtual capture_video_settings_s get_video_settings(void)          const = 0;
    virtual capture_video_settings_s get_default_video_settings(void)  const = 0;
    virtual capture_video_settings_s get_minimum_video_settings(void)  const = 0;
    virtual capture_video_settings_s get_maximum_video_settings(void)  const = 0;
    virtual resolution_s             get_resolution(void)              const = 0;
    virtual resolution_s             get_minimum_resolution(void)      const = 0;
    virtual resolution_s             get_maximum_resolution(void)      const = 0;
    virtual const captured_frame_s&  get_frame_buffer(void)            const = 0;
    virtual capture_event_e          get_latest_capture_event(void)    const = 0;
    virtual capture_signal_s         get_signal_info(void)             const = 0;
    virtual int                      get_frame_rate(void)              const = 0;

    // Setters.
    virtual void set_mode_params(const std::vector<video_mode_params_s> &modeParams) { (void)modeParams; return;       };
    virtual bool set_mode_parameters_for_resolution(const resolution_s r)            { (void)r;          return false; };
    virtual void set_color_settings(const capture_color_settings_s c)                { (void)c;          return;       };
    virtual void set_video_settings(const capture_video_settings_s v)                { (void)v;          return;       };
    virtual bool adjust_video_horizontal_offset(const int delta)                     { (void)delta;      return false; };
    virtual bool adjust_video_vertical_offset(const int delta)                       { (void)delta;      return false; };
    virtual void mark_frame_buffer_as_processed(void)                                {                   return;       };
    virtual bool set_color_depth(const resolution_s r)                               { (void)r;          return false; };
    virtual bool set_input_channel(const u32 channel)                                { (void)channel;    return false; };
    virtual bool set_input_color_depth(const u32 bpp)                                { (void)bpp;        return false; };
    virtual bool set_frame_dropping(const u32 drop)                                  { (void)drop;       return false; };
    virtual void apply_new_capture_resolution(void)                                  {                   return;       };
    virtual void reset_missed_frames_count(void)                                     {                   return;       };

protected:
    // Frames sent by the capture hardware will be stored here for processing
    // in VCS's thread.
    captured_frame_s frameBuffer;
};
