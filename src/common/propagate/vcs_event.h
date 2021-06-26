/*
 * 2020-2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * The VCS event system allows the various subsystems of VCS to receive notification
 * when certain runtime events occur.
 *
 */

#ifndef VCS_COMMON_PROPAGATE_VCS_EVENT_H
#define VCS_COMMON_PROPAGATE_VCS_EVENT_H

#include <functional>
#include <list>

// An event that passes an argument to its event handlers.
template <typename T>
class vcs_event_c
{
public:
    void subscribe(std::function<void(T)> handlerFn)
    {
        this->subscribedHandlers.push_back(handlerFn);

        return;
    }

    // For event handlers that want to ignore the callback argument.
    void subscribe(std::function<void(void)> handlerFn)
    {
        this->subscribedHandlersNoArgs.push_back(handlerFn);

        return;
    }

    void fire(T value) const
    {
        for (const auto &handlerFn: this->subscribedHandlers)
        {
            handlerFn(value);
        }

        for (const auto &handlerFn: this->subscribedHandlersNoArgs)
        {
            handlerFn();
        }

        return;
    }

private:
    std::list<std::function<void(T)>> subscribedHandlers;
    std::list<std::function<void(void)>> subscribedHandlersNoArgs;
};

// An event that passes no arguments to its event handlers.
template <>
class vcs_event_c<void>
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
