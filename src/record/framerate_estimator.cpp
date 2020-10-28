/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include "record/framerate_estimator.h"

void framerate_estimator_s::initialize(const uint frameCount)
{
    this->fps = 0;
    this->prevFrameCount = frameCount;
    this->timer.start();

    return;
}

void framerate_estimator_s::update(const uint frameCount)
{
    const uint frames = (frameCount - this->prevFrameCount);

    this->fps = (frames / (this->timer.nsecsElapsed() / 1000000000.0));
    this->prevFrameCount = frameCount;
    this->timer.restart();

    return;
}

double framerate_estimator_s::framerate(void)
{
    return this->fps;
}
