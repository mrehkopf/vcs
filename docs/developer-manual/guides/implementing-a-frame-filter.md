# Implementing a frame filter

[Frame filters](@ref src/filter/filter.h) provide functionality to the end-user of VCS for manipulating the pixels of captured frames. Blurring, cropping, and denoising are some examples of what these filters could be used for.

In this guide, you'll learn how to implement new frame filters for VCS. The guide will go over a sample implementation of a filter that fills each pixel of an input frame with a solid color.

*Note*: Future versions of VCS may come to support frame filters as dynamic libraries, but for now their code is compiled statically into the VCS executable.

## Sample implementation

There are three steps to implementing a frame filter:

1. Subclass `abstract_filter_c`.
2. Subclass `filtergui_c`.
3. Declare the filter's existence to VCS.

The sub-sections below explain each step in detail, in the process creating a sample frame filter that fills each input frame's pixels with a solid, user-customizable color.

### Subclassing abstract_filter_c

The `abstract_filter_c` class provides metadata about a given filter, as well as a function to apply the filter's processing onto the pixels of an input frame.

We'll create a new file, *filter_filler.h*, where we'll define a subclass of `abstract_filter_c` for our sample pixel filler filter. The file has the following contents:

```cpp
class filter_filler_c : public abstract_filter_c
{
public:
    // Boilerplate to make the filter clonable. All filters must include this line.
    CLONABLE_FILTER_TYPE(filter_filler_c)

    // The filter's user-customizable parameters. In this case, the RGB fill color.
    // The naming of the enumerators is up to you; no particular style is required.
    enum { PARAM_RED,
           PARAM_GREEN,
           PARAM_BLUE };

    // The constructor that's called whenever a new instance of this filter is created.
    // Note: We set the default fill color to 150, 10, 200. Instances of the filter
    // will start with those values unless 'initialParamValues' specifies other values.
    filter_filler_c(const std::vector<std::pair<unsigned, double>> &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_RED, 150},
            {PARAM_GREEN, 10},
            {PARAM_BLUE, 200}
        }, initialParamValues)
    {
        // Note: We'll create the filter's GUI (filtergui_filler_c) later in this guide.
        // In any case, here we attach an instance of the GUI to this new instance of
        // the filter.
        this->guiDescription = new filtergui_filler_c(this);
    }

    // Applies the filter's processing onto the pixels of an input frame.
    void apply(image_s *const image) override;

    // Metadata about the filter. These data serve both to uniquely identify the filter
    // and for presenting it to the end-user in the VCS UI.
    std::string uuid(void) const override { return "6f6b513d-359e-43c7-8de5-de29b1559d10"; }
    std::string name(void) const override { return "Filler"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }
};
```

In another new file, *filter_filler.cpp*, we'll provide an implementation of the filter's `apply()` function:

```cpp
void filter_filler_c::apply(image_s *const image)
{
    // All filters should assert the validity of their input data in this way.
    this->assert_input_validity(image);

    // The RGB fill color. Note: The parameter values (e.g. PARAM_RED) can be
    // customized by the end-user at run-time via the filter's GUI.
    const uint8_t red   = this->parameter(PARAM_RED);
    const uint8_t green = this->parameter(PARAM_GREEN);
    const uint8_t blue  = this->parameter(PARAM_BLUE);
    const uint8_t alpha = 255;
    
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    output = cv::Scalar(blue, green, red, alpha);

    return;
}
```

When the filter is active in VCS's [filter graph dialog](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.0/#dialog-windows-filter-graph-dialog), this function will be called by VCS with the data of each frame as they're captured.

*Note*: The `apply()` function is prohibited from modifying the input frame's resolution.

### Subclassing filtergui_c

The `filtergui_c` class provides a template for a filter's GUI. The GUI &ndash; which will be available to the end-user via VCS's [filter graph dialog](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.0/#dialog-windows-filter-graph-dialog) &ndash; consists of UI widgets for adjusting the filter's parameters (e.g. the radius of a blurring filter) at run-time.

*Note*: The GUI template is defined using an API that's independent of VCS's display framework (e.g. Qt). VCS will automatically translate the template into the native format of the framework being used.

To contain the sample filter's GUI template, we'll create two new files, *filtergui_filter_filler.h* and *filtergui_filter_filler.cpp*.

In *filtergui_filter_filler.h*:

```cpp
class filtergui_filler_c : public filtergui_c
{
public:
    filtergui_filler_c(abstract_filter_c *const filter);
};
```

The header file simply needs to set up a subclass of `filtergui_c` and declare its constructor.

In *filtergui_filter_filler.cpp*:

```cpp
filtergui_filler_c::filtergui_filler_c(abstract_filter_c *const filter)
{
    // A GUI widget that lets the user control a value from 0 to 255 for the
    // color fill's red component. The set_value() function gets called when
    // the user changes the value.
    auto *const red = new filtergui_spinbox_s;
    red->get_value = [=]{return filter->parameter(filter_filler_c::PARAM_RED);};
    red->set_value = [=](const double value){filter->set_parameter(filter_filler_c::PARAM_RED, value);};
    red->minValue = 0;
    red->maxValue = 255;
    
    // Same as above but for the fill's green component.
    auto *const green = new filtergui_spinbox_s;
    green->get_value = [=]{return filter->parameter(filter_filler_c::PARAM_BIT_COUNT_GREEN);};
    green->set_value = [=](const double value){filter->set_parameter(filter_filler_c::PARAM_GREEN, value);};
    green->minValue = 0;
    green->maxValue = 255;
    
    // For the blue component.
    auto *const blue = new filtergui_spinbox_s;
    blue->get_value = [=]{return filter->parameter(filter_filler_c::PARAM_BLUE);};
    blue->set_value = [=](const double value){filter->set_parameter(filter_filler_c::PARAM_BLUE, value);};
    blue->minValue = 0;
    blue->maxValue = 255;
    
    // Insert the widgets into a row labeled "RGB".
    this->guiFields.push_back({"RGB", {red, green, blue}});

    return;
}
```

The constructor's implementation populates the `guiFields` vector of the class instance with the filter's GUI widgets for controlling the fill color.

The sample filter's implementation is now complete. All that's left to do is to make VCS aware of it.

### Declaring the filter to VCS

Filters are declared in kf_initialize_filters():

```cpp
void kf_initialize_filters(void)
{
    KNOWN_FILTER_TYPES =
    {
        new filter_blur_c(),
        new filter_delta_histogram_c(),
        // ...
    };
}
```

We'll simply append our new filter onto the list:

```cpp
void kf_initialize_filters(void)
{
    KNOWN_FILTER_TYPES =
    {
        new filter_blur_c(),
        new filter_delta_histogram_c(),
        // ...
        new filter_filler_c() // <- Our new filter.
    };
}
```

That's it. The filter will now appear as a selectable entry in the menus of VCS's [filter graph dialog](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.0/#dialog-windows-filter-graph-dialog).

## Programmatic usage

Although frame filters will typically be controlled by the end-user via VCS's [filter graph dialog](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.0/#dialog-windows-filter-graph-dialog), they can also be created and invoked programmatically:

```cpp
// Create an instance of the filter using the templated kf_create_filter_instance().
auto *colorFill = kf_create_filter_instance<filter_filler_c>();

// Or: Create an instance of the filter using its UUID.
// auto *filler = kf_create_filter_instance("6f6b513d-359e-43c7-8de5-de29b1559d10").

colorFill->set_parameters({
    {filter_filler_c::PARAM_RED, 255},
    {filter_filler_c::PARAM_GREEN, 0},
    {filter_filler_c::PARAM_BLUE,  0}
});

colorFill->apply(image);
```
