/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * The app event system allows the various subsystems of VCS to receive notification
 * when certain runtime events occur.
 *
 */

#ifndef VCS_COMMON_PROPAGATE_APP_EVENTS_H
#define VCS_COMMON_PROPAGATE_APP_EVENTS_H

#include <functional>
#include <list>

template <typename T>
class app_event_c
{
public:
    void subscribe(std::function<void(T)> handlerFn)
    {
        this->subscribedHandlers.push_back(handlerFn);

        return;
    }

    void fire(T value) const
    {
        for (const auto &handlerFn: this->subscribedHandlers)
        {
            handlerFn(value);
        }

        return;
    }

private:
    std::list<std::function<void(T)>> subscribedHandlers;
};

// An event that passes no parameters.
template <>
class app_event_c<void>
{
public:
    void subscribe(std::function<void(void)> handlerFn)
    {
        this->subscribedHandlers.push_back(handlerFn);

        return;
    }

    void fire(void) const
    {
        for (const auto &handlerFn: this->subscribedHandlers)
        {
            handlerFn();
        }

        return;
    }

private:
    std::list<std::function<void(void)>> subscribedHandlers;
};

#endif
