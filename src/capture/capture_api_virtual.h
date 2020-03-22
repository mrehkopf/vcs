/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Provides a public interface for interacting with the capture device.
 *
 */

#include <string>
#include "capture/capture_api.h"

struct capture_api_virtual_s : public capture_api_s
{
    capture_api_virtual_s()
    {
        this->frameBuffer.r = this->defaultResolution;

        return;
    }

    virtual bool initialize(void) override;
    virtual bool start_capture(void) override { return true; };
    virtual bool stop_capture(void) override { return true; };

    // Getters.
    virtual std::string              get_device_name(void)             const override { return "Virtual VCS capture device"; }
    virtual std::string              get_api_name(void)                const override { return "Virtual VCS capture API"; }
    virtual std::string              get_device_driver_version(void)   const override { return "1.0"; };
    virtual std::string              get_device_firmware_version(void) const override { return "1.0"; }
    virtual int                      get_maximum_input_count(void)     const override { return 2;  };
    virtual capture_color_settings_s get_default_color_settings(void)  const override { return capture_color_settings_s{}; };
    virtual capture_color_settings_s get_minimum_color_settings(void)  const override { return capture_color_settings_s{}; };
    virtual capture_color_settings_s get_maximum_color_settings(void)  const override { return capture_color_settings_s{}; };
    virtual capture_video_settings_s get_default_video_settings(void)  const override { return capture_video_settings_s{}; };
    virtual capture_video_settings_s get_minimum_video_settings(void)  const override { return capture_video_settings_s{}; };
    virtual capture_video_settings_s get_maximum_video_settings(void)  const override { return capture_video_settings_s{}; };
    virtual resolution_s             get_resolution(void)              const override { return this->defaultResolution; };
    virtual resolution_s             get_minimum_resolution(void)      const override { return this->defaultResolution; };
    virtual resolution_s             get_maximum_resolution(void)      const override { return this->defaultResolution; };
    virtual const captured_frame_s&  get_frame_buffer(void)            const override;
    virtual capture_color_settings_s get_color_settings(void)          const override { return {}; };
    virtual capture_video_settings_s get_video_settings(void)          const override { return {}; };
    virtual capture_signal_s         get_signal_info(void)             const override { return {}; };
    virtual int                      get_frame_rate(void)              const override { return 0; };
    virtual capture_event_e          get_latest_capture_event(void)    const override;

    // Setters.
    virtual void mark_frame_buffer_as_processed(void) override;

private:
    const resolution_s defaultResolution = resolution_s{640, 480, 32};

    // We don't do any actual capturing, so draw an animating pattern in the
    // frame buffer, instead.
    void animate_frame_buffer(void);
};
