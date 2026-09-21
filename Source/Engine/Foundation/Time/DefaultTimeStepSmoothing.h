/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Time/Clock.h>

/// Implements a simple time step smoothing algorithm.
///
/// The description for the algorithm was taken from here:
/// http://bitsquid.blogspot.de/2010/10/time-step-smoothing.html
///
/// This class implements that algorithm pretty much verbatim.
/// It does not implement keeping track of the time dept and paying that off later, though.
class XII_FOUNDATION_DLL xiiDefaultTimeStepSmoothing : public xiiTimeStepSmoothing
{
public:
  xiiDefaultTimeStepSmoothing();

  virtual xiiTime GetSmoothedTimeStep(xiiTime rawTimeStep, const xiiClock* pClock) override;

  virtual void Reset(const xiiClock* pClock) override;

  /// Changes the factor with which to lerp from the last used time step to the new average time step. Default is 0.2
  ///
  /// A value of 1.0 would mean that the new average time step is used immediately. The lower the value the more slowly the
  /// time step will change from its previous value to the new average value, thus smoothing the time step even more.
  void SetLerpFactor(float f) { m_fLerpFactor = f; }

private:
  float                            m_fLerpFactor;
  xiiTime                          m_LastTimeStepTaken;
  xiiStaticRingBuffer<xiiTime, 11> m_LastTimeSteps;
};
