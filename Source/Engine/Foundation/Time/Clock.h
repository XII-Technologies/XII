/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

class xiiTimeStepSmoothing;

/// A clock that can be speed up, slowed down, paused, etc. Useful for updating game logic, rendering, etc.
class XII_FOUNDATION_DLL xiiClock
{
public:
  /// Returns the global clock.
  static xiiClock* GetGlobalClock() { return s_pGlobalClock; }

public:
  /// Constructor.
  xiiClock(xiiStringView sName); // [tested]

  /// Resets all values to their default. E.g. call this after a new level has loaded to start fresh.
  ///
  /// If \a bEverything is false, only the current state of the clock is reset (accumulated time, speed, paused).
  /// Otherwise the clock is entirely reset, clearing also the time step smoother, min/max time steps and fixed time step.
  void Reset(bool bEverything); // [tested]

  /// Updates the clock using the time difference since the last call to Update().
  ///
  /// If a fixed time step is set, that will be used as the time difference.
  /// If the timer is paused, the time difference is set to zero.
  /// The time difference will then be scaled and clamped according to the clock speed and minimum and maximum time step.
  void Update(); // [tested]

  /// Sets a time step smoother for this clock. Pass nullptr to deactivate time step smoothing.
  ///
  /// Also calls xiiTimeStepSmoothing::Reset() on any non-nullptr pSmoother.
  void SetTimeStepSmoothing(xiiTimeStepSmoothing* pSmoother);

  /// Returns the object used for time step smoothing (if any).
  xiiTimeStepSmoothing* GetTimeStepSmoothing() const; // [tested]

  /// Sets the clock to be paused or running.
  void SetPaused(bool bPaused); // [tested]

  /// Returns the paused state.
  bool GetPaused() const; // [tested]

  /// Sets a fixed time step for updating the clock.
  ///
  /// If tDiff is set to zero (the default), fixed time stepping is disabled.
  /// Fixed time stepping allows to run the simulation at a constant rate, which is useful
  /// for recording videos or to step subsystems that require constant steps.
  /// Clock speed, pause and min/max time step are still being applied even when the time step is fixed.
  void SetFixedTimeStep(xiiTime diff = xiiTime()); // [tested]

  /// Returns the value for the fixed time step (zero if it is disabled).
  xiiTime GetFixedTimeStep() const; // [tested]

  /// Allows to replace the current accumulated time.
  ///
  /// This can be used to reset the time to a specific point, e.g. when a game state is loaded from file,
  /// one should also reset the time to the time that was used when the game state was saved, to ensure
  /// that game objects that stored the accumulated time for reference, will continue to work.
  /// However, prefer to use Save() and Load() as those functions will store and restore the entire clock state.
  void SetAccumulatedTime(xiiTime t); // [tested]

  /// Returns the accumulated time since the last call to Reset().
  ///
  /// The accumulated time is basically the 'absolute' time in the game world.
  /// Since this is the accumulation of all scaled, paused and clamped time steps,
  /// it will most likely have no relation to the real time that has passed.
  xiiTime GetAccumulatedTime() const; // [tested]

  /// Returns the time at which the clock was update.
  xiiTime GetLastUpdateTime() const { return m_LastTimeUpdate; }

  /// Returns the time difference between the last two calls to Update().
  ///
  /// This is the main function to use to query how much to advance some simulation.
  /// The time step is already scaled, clamped, etc.
  xiiTime GetTimeDiff() const; // [tested]

  /// The factor with which to scale the time step during calls to Update().
  void SetSpeed(double fFactor); // [tested]

  /// Returns the clock speed multiplier.
  double GetSpeed() const; // [tested]

  /// Sets the minimum time that must pass between clock updates.
  ///
  /// By default a minimum time step of 0.001 seconds is enabled to ensure that code does not break down
  /// due to very small time steps. The minimum time step is applied after the clock speed is applied.
  /// When a custom time step smoother is set, that class needs to apply the clock speed AND also clamp
  /// the value to the min/max time step (which means it can ignore or override that feature).
  /// When the clock is paused, it will always return a time step of zero.
  void SetMinimumTimeStep(xiiTime min); // [tested]

  /// Sets the maximum time that may pass between clock updates.
  ///
  /// By default a maximum time step of 0.1 seconds is enabled to ensure that code does not break down
  /// due to very large time steps. The maximum time step is applied after the clock speed is applied.
  /// When a custom time step smoother is set, that class needs to apply the clock speed AND also clamp
  /// the value to the min/max time step (which means it can ignore or override that feature).
  /// \sa SetMinimumTimeStep
  void SetMaximumTimeStep(xiiTime max); // [tested]

  /// Returns the value for the minimum time step.
  /// \sa SetMinimumTimeStep
  xiiTime GetMinimumTimeStep() const; // [tested]

  /// Returns the value for the maximum time step.
  /// \sa SetMaximumTimeStep
  xiiTime GetMaximumTimeStep() const; // [tested]

  /// Serializes the current clock state to a stream.
  void Save(xiiStreamWriter& ref_stream) const;

  /// Deserializes the current clock state from a stream.
  void Load(xiiStreamReader& ref_stream);

  /// Sets the name of the clock. Useful to identify the clock in tools such as xiiInspector.
  void SetClockName(xiiStringView sName);

  /// Returns the name of the clock. All clocks get default names 'Clock N', unless the user specifies another name with
  /// SetClockName.
  xiiStringView GetClockName() const;

public:
  /// The data that is sent through the event interface.
  struct EventData
  {
    xiiStringView m_sClockName;

    xiiTime m_RawTimeStep;
    xiiTime m_SmoothedTimeStep;
  };

  using Event = xiiEvent<const EventData&, xiiMutex>;

  /// Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddEventHandler(Event::Handler handler) { s_TimeEvents.AddEventHandler(handler); }

  /// Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveEventHandler(Event::Handler handler) { s_TimeEvents.RemoveEventHandler(handler); }


private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Clock);

  static Event     s_TimeEvents;
  static xiiClock* s_pGlobalClock;

  xiiString m_sName;

  xiiTime m_AccumulatedTime;
  xiiTime m_LastTimeDiff;
  xiiTime m_FixedTimeStep;
  xiiTime m_LastTimeUpdate;
  xiiTime m_MinTimeStep;
  xiiTime m_MaxTimeStep;

  double m_fSpeed;
  bool   m_bPaused;

  xiiTimeStepSmoothing* m_pTimeStepSmoother;
};


/// Base class for all time step smoothing algorithms.
///
/// By deriving from this class you can implement your own algorithms for time step smoothing.
/// Then just set an instance of that class on one of the clocks and it will be applied to the time step.
class XII_FOUNDATION_DLL xiiTimeStepSmoothing
{
public:
  virtual ~xiiTimeStepSmoothing() = default;

  /// The function to override to implement time step smoothing.
  ///
  /// \param RawTimeStep
  ///   The actual raw time difference since the last clock update without any modification.
  /// \param pClock
  ///   The clock that calls this time step smoother.
  ///   Can be used to look up the clock speed and min/max time step.
  ///
  /// \note It is the responsibility of each xiiTimeStepSmoothing class to implement
  /// clock speed and also to clamp the time step to the min/max values.
  /// This allows the smoothing algorithm to override these values, if necessary.
  virtual xiiTime GetSmoothedTimeStep(xiiTime rawTimeStep, const xiiClock* pClock) = 0;

  /// Called when xiiClock::Reset(), xiiClock::Load() or xiiClock::SetPaused(true) was called.
  ///
  /// \param pClock
  ///   The clock that is calling this function.
  virtual void Reset(const xiiClock* pClock) = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiClock);

#include <Foundation/Time/Implementation/Clock_inl.h>
