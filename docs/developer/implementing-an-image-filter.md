# Implementing an image filter in VCS

Image filters provide capabilities to the VCS end-user to manipulate the pixels of captured frames prior to display. Blurring, cropping, etc. are examples of what these filters could be used for.

This guide presents a sample implementation of an image filter &ndash; one that fills the image with a solid, user-customizable color &ndash; to give you an idea of how to go about creating your own filters for VCS.

## Going about it

There are two steps to creating an image filter:

1. Provide an implementation of the filter by subclassing `abstract_filter_c` and `abstract_gui_s`.
2. Declare the filter to VCS.

### Provide an implementation of the filter

The `abstract_filter_c` class is an interface between VCS and your filter implementation. It includes an instance of `abstract_gui_s`, which is a GUI framework-independent interface between the VCS filter graph UI and the user-customizable parameters of your filter.

Let's create a new file, **filler.h**, where we can define our pixel filler filter:

```cpp
#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filler : public abstract_filter_c
{
public:
    // Boilerplate to make the filter clonable. All filters must include this line.
    // Simply pass in the class name.
    CLONABLE_FILTER_TYPE(filler)

    // The filter's user-customizable parameters. In this case, the RGB fill color.
    // The naming of the enumerators is up to you; no particular style is required.
    enum {
        PARAM_RED,
        PARAM_GREEN,
        PARAM_BLUE
    };

    // The constructor that's called whenever a new instance of this filter is
    // created.
    // 
    // Note: We set the default fill color to 150, 10, 200. Instances of the filter
    // will start with those values unless 'initialParamValues' specifies otherwise.
    filler(const filter_params_t &initialValues = {}) :
        abstract_filter_c({
            {PARAM_RED, 150},
            {PARAM_GREEN, 10},
            {PARAM_BLUE, 200}
        }, initialParamValues)
    {
        // Create the GUI for this filter, to be used in VCS's filter graph.
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            // GUI widgets that let the user select a value from 0 to 255 for the
            // color fill's color channels. The set_value() function gets called
            // when the user changes the value in the UI.
            std::array<abstract_gui::spinner*, 3> widgets;
            for (unsigned paramIdx: {PARAM_RED, PARAM_GREEN, PARAM_BLUE})
            {
                auto *widget = widgets.at(paramIdx) = filtergui::spinner(filter, paramIdx);
                widget->minValue = 0;
                widget->maxValue = 255;
            }

            gui->fields.push_back({"Red", {widgets.at(PARAM_RED)}});
            gui->fields.push_back({"Green", {widgets.at(PARAM_GREEN)}});
            gui->fields.push_back({"Blue", {widgets.at(PARAM_BLUE)}});
        });
    }

    // Applies the filter's processing onto the pixels of an input image.
    void apply(image_s *const image) override
    {
        cv::Mat output = cv::Mat(
            image->resolution.h,
            image->resolution.w,
            CV_8UC4,
            image->pixels
        );

        // Paint every pixel with the fill color.
        output = cv::Scalar(
            this->parameter(PARAM_BLUE),
            this->parameter(PARAM_GREEN),
            this->parameter(PARAM_RED),
            255
        );
    }

    // Metadata about the filter. Changing the UUID will break compatibility with
    // earlier filter configurations the user may have saved.
    std::string uuid(void) const override { return "6f6b513d-359e-43c7-8de5-de29b1559d10"; }
    std::string name(void) const override { return "Filler"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }
};
```

### Declare the filter to VCS

VCS's filters are declared in `kf_initialize_filters()`. Simply append the filler filter onto the list:

```cpp
void kf_initialize_filters()
{
    KNOWN_FILTER_TYPES =
    {
        new filter_blur_c(),
        new filter_delta_histogram_c(),
        new filter_frame_rate_c(),
        // ...
        new filler() // <- Our filter.
    };
}
```

That's it. The filter will now appear as a selectable entry in VCS's filter graph.
