/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>

class xiiProgress;
class xiiProgressRange;

/// Through these events the state of a xiiProgress instance is communicated.
///
/// Other code can use this to visualize the progress in different ways.
/// For instance a GUI application can show a progress bar dialog and a game
/// could show a loading screen.
struct XII_FOUNDATION_DLL xiiProgressEvent
{
  enum class Type
  {
    ProgressStarted, ///< Sent when the the first progress starts
    ProgressEnded,   ///< Sent when progress finishes or is canceled
    ProgressChanged, ///< Sent whenever the progress value changes. Not necessarily in every update step.
    CancelClicked,   ///< The user just clicked cancel (for the first time).
  };

  Type         m_Type;
  xiiProgress* m_pProgressbar;
};

/// Manages the way a progress bar is subdivided and advanced.
///
/// xiiProgress represents a single progress bar. It can be sub-divided into groups and sub-groups using xiiProgressbarRange.
/// From the ranges and the current advancement, a final progress percentage is computed. Every time a significant change
/// takes place, an event is broadcast. This allows other code to display the progress, either in a GUI application
/// or in a fullscreen loading screen or in any other way appropriate.
class XII_FOUNDATION_DLL xiiProgress
{
public:
  xiiProgress();
  ~xiiProgress();

  /// Returns the current overall progress in [0; 1] range.
  float GetCompletion() const;

  /// Sets the current overall progress in [0; 1] range. Should not be called directly, typically called by xiiProgreesRange.
  void SetCompletion(float fCompletion);

  /// Returns the current 'headline' text for the progress bar
  xiiStringView GetMainDisplayText() const;

  /// Returns the current detail text for the progress bar
  xiiStringView GetStepDisplayText() const;

  /// Used to inform xiiProgress of outside user input. May have an effect or not.
  void UserClickedCancel();

  /// Whether the user requested to cancel the operation.
  bool WasCanceled() const;

  /// Returns whether the current operations may be canceled or not.
  bool AllowUserCancel() const;

  /// Returns the currently set default xiiProgress instance. This will always be valid.
  static xiiProgress* GetGlobalProgressbar();

  /// Allows to set a custom xiiProgress instance as the global default instance.
  static void SetGlobalProgressbar(xiiProgress* pProgress);

  /// Events are sent when the progress changes
  xiiEvent<const xiiProgressEvent&> m_Events;

  /// Custom user data.
  void* m_pUserData = nullptr;

private:
  friend class xiiProgressRange;
  void SetActiveRange(xiiProgressRange* pRange);

  xiiProgressRange* m_pActiveRange = nullptr;

  xiiString m_sCurrentDisplayText;
  bool      m_bCancelClicked = false;
  bool      m_bEnableCancel  = true;

  float m_fLastReportedCompletion = 0.0f;
  float m_fCurrentCompletion      = 0.0f;
};

/// xiiProgressRange is the preferred method to inform the system of progress.
///
/// xiiProgressRange is a scoped class, ie. upon creation it adds a range to the current progress
/// and upon destruction the entire range is considered to be completed.
/// Ranges can be nested. For instance when a top level range consists of three 'steps',
/// then opening a nested range will sub-divide that first step. When the nested range is closed,
/// the first top-level step is finished and BeginNextStep() should be called on the top-level range.
/// Subsequently the second step is active and can again be further subdivided with another nested xiiProgressRange.
class XII_FOUNDATION_DLL xiiProgressRange
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiProgressRange);

public:
  /// Creates a progress range scope.
  ///
  /// If any other progress range is currently active, it will become the parent range and the currently active step will be subdivided.
  /// \param szDisplayText is the main display text for this range.
  /// \param uiSteps is the number of steps that this range will be subdivided into
  /// \param bAllowCancel specifies whether the user can cancel this operation
  /// \param pProgressbar can be specified, if available, otherwise the currently active xiiProgress instance is used.
  xiiProgressRange(xiiStringView sDisplayText, xiiUInt32 uiSteps, bool bAllowCancel, xiiProgress* pProgressbar = nullptr);

  /// Creates a progress range scope without steps. Use SetCompletion to manually set the completion value.
  xiiProgressRange(xiiStringView sDisplayText, bool bAllowCancel, xiiProgress* pProgressbar = nullptr);

  /// The destructor closes the current range. All progress in this range is assumed to have completed,
  /// even if BeginNextStep() has not been called once for every subdivision step.
  ~xiiProgressRange();

  /// Returns the xiiProgress instance that this range uses.
  xiiProgress* GetProgressbar() const;

  /// Allows to weigh each step differently.
  ///
  /// This makes it possible to divide an operation into two steps, but have one part take up 90% and the other 10%.
  /// \param uiStep The index for the step to set the weight
  /// \param fWeight The weighting in [0; 1] range
  void SetStepWeighting(xiiUInt32 uiStep, float fWeight);

  /// Should be called whenever a new sub-step is started to advance the progress.
  ///
  /// \param szStepDisplayText The sub-text for the next step to be displayed.
  /// \param uiNumSteps How many steps have been completed.
  /// \return Returns false if the user clicked cancel.
  bool BeginNextStep(xiiStringView sStepDisplayText, xiiUInt32 uiNumSteps = 1);

  /// Manually set the completion value between 0..1.
  bool SetCompletion(double fCompletionFactor);

  /// Whether the user requested to cancel the operation.
  bool WasCanceled() const;

private:
  friend class xiiProgress;

  void  Init(xiiStringView sDisplayText, bool bAllowCancel, xiiProgress* pProgressbar);
  float GetStepWeight(xiiUInt32 uiStep) const;
  void  ComputeCurStepBaseAndRange(double& out_base, double& out_range);

  xiiProgressRange* m_pParentRange = nullptr;
  xiiProgress*      m_pProgressbar = nullptr;

  xiiInt32                       m_iCurrentStep = 0;
  xiiString                      m_sDisplayText;
  xiiString                      m_sStepDisplayText;
  xiiHashTable<xiiUInt32, float> m_StepWeights;

  bool   m_bAllowCancel        = false;
  double m_fPercentageBase     = 0.0;
  double m_fPercentageRange    = 0.0;
  double m_fWeightedCompletion = 0.0;
  double m_fSummedWeight       = 0.0;
};
