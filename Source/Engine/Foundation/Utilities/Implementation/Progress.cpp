/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Progress.h>

static xiiProgress* s_pGlobal = nullptr;

xiiProgress::xiiProgress() = default;

xiiProgress::~xiiProgress()
{
  if (s_pGlobal == this)
  {
    s_pGlobal = nullptr;
  }
}

float xiiProgress::GetCompletion() const
{
  return m_fCurrentCompletion;
}

void xiiProgress::SetCompletion(float fCompletion)
{
  XII_ASSERT_DEV(fCompletion >= 0.0f && fCompletion <= 1.0f, "Completion value {0} is out of valid range", fCompletion);

  m_fCurrentCompletion = fCompletion;

  if (fCompletion > m_fLastReportedCompletion + 0.001f)
  {
    m_fLastReportedCompletion = fCompletion;

    xiiProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type         = xiiProgressEvent::Type::ProgressChanged;

    m_Events.Broadcast(e, 1);
  }
}

void xiiProgress::SetActiveRange(xiiProgressRange* pRange)
{
  if (m_pActiveRange == nullptr && pRange != nullptr)
  {
    m_fLastReportedCompletion = 0.0;
    m_fCurrentCompletion      = 0.0;
    m_bCancelClicked          = false;
    m_bEnableCancel           = pRange->m_bAllowCancel;

    xiiProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type         = xiiProgressEvent::Type::ProgressStarted;

    m_Events.Broadcast(e);
  }

  if (m_pActiveRange != nullptr && pRange == nullptr)
  {
    xiiProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type         = xiiProgressEvent::Type::ProgressEnded;

    m_Events.Broadcast(e);
  }

  m_pActiveRange = pRange;
}

xiiStringView xiiProgress::GetMainDisplayText() const
{
  if (m_pActiveRange == nullptr)
    return {};

  return m_pActiveRange->m_sDisplayText;
}

xiiStringView xiiProgress::GetStepDisplayText() const
{
  if (m_pActiveRange == nullptr)
    return {};

  return m_pActiveRange->m_sStepDisplayText;
}

void xiiProgress::UserClickedCancel()
{
  if (m_bCancelClicked)
    return;

  m_bCancelClicked = true;

  xiiProgressEvent e;
  e.m_Type         = xiiProgressEvent::Type::CancelClicked;
  e.m_pProgressbar = this;

  m_Events.Broadcast(e, 1);
}

bool xiiProgress::WasCanceled() const
{
  return m_bCancelClicked;
}

bool xiiProgress::AllowUserCancel() const
{
  return m_bEnableCancel;
}

xiiProgress* xiiProgress::GetGlobalProgressbar()
{
  if (!s_pGlobal)
  {
    static xiiProgress s_Global;
    return &s_Global;
  }

  return s_pGlobal;
}

void xiiProgress::SetGlobalProgressbar(xiiProgress* pProgress)
{
  s_pGlobal = pProgress;
}

//////////////////////////////////////////////////////////////////////////

xiiProgressRange::xiiProgressRange(xiiStringView sDisplayText, xiiUInt32 uiSteps, bool bAllowCancel, xiiProgress* pProgressbar /*= nullptr*/)
{
  XII_ASSERT_DEV(uiSteps > 0, "Every progress range must have at least one step to complete");

  m_iCurrentStep        = -1;
  m_fWeightedCompletion = -1.0;
  m_fSummedWeight       = (double)uiSteps;

  Init(sDisplayText, bAllowCancel, pProgressbar);
}

xiiProgressRange::xiiProgressRange(xiiStringView sDisplayText, bool bAllowCancel, xiiProgress* pProgressbar /*= nullptr*/)
{
  Init(sDisplayText, bAllowCancel, pProgressbar);
}

void xiiProgressRange::Init(xiiStringView sDisplayText, bool bAllowCancel, xiiProgress* pProgressbar)
{
  if (pProgressbar == nullptr)
    m_pProgressbar = xiiProgress::GetGlobalProgressbar();
  else
    m_pProgressbar = pProgressbar;

  XII_ASSERT_DEV(m_pProgressbar != nullptr, "No global progress-bar context available.");

  m_bAllowCancel = bAllowCancel;
  m_sDisplayText = sDisplayText;

  m_pParentRange = m_pProgressbar->m_pActiveRange;

  if (m_pParentRange == nullptr)
  {
    m_fPercentageBase  = 0.0;
    m_fPercentageRange = 1.0;
  }
  else
  {
    m_pParentRange->ComputeCurStepBaseAndRange(m_fPercentageBase, m_fPercentageRange);
  }

  m_pProgressbar->SetActiveRange(this);
}

xiiProgressRange::~xiiProgressRange()
{
  m_pProgressbar->SetCompletion((float)(m_fPercentageBase + m_fPercentageRange));
  m_pProgressbar->SetActiveRange(m_pParentRange);
}

xiiProgress* xiiProgressRange::GetProgressbar() const
{
  return m_pProgressbar;
}

void xiiProgressRange::SetStepWeighting(xiiUInt32 uiStep, float fWeight)
{
  XII_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_fSummedWeight -= GetStepWeight(uiStep);
  m_fSummedWeight += fWeight;
  m_StepWeights[uiStep] = fWeight;
}

float xiiProgressRange::GetStepWeight(xiiUInt32 uiStep) const
{
  const float* pOldWeight = m_StepWeights.GetValue(uiStep);
  return pOldWeight != nullptr ? *pOldWeight : 1.0f;
}

void xiiProgressRange::ComputeCurStepBaseAndRange(double& out_base, double& out_range)
{
  const double internalBase  = xiiMath::Max(m_fWeightedCompletion, 0.0) / m_fSummedWeight;
  const double internalRange = GetStepWeight(xiiMath::Max(m_iCurrentStep, 0)) / m_fSummedWeight;

  out_range = internalRange * m_fPercentageRange;
  out_base  = m_fPercentageBase + (internalBase * m_fPercentageRange);

  XII_ASSERT_DEBUG(out_base <= 1.0f, "Invalid range");
  XII_ASSERT_DEBUG(out_range <= 1.0f, "Invalid range");
  XII_ASSERT_DEBUG(out_base + out_range <= 1.0f, "Invalid range");
}

bool xiiProgressRange::BeginNextStep(xiiStringView sStepDisplayText, xiiUInt32 uiNumSteps)
{
  XII_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_sStepDisplayText = sStepDisplayText;

  for (xiiUInt32 i = 0; i < uiNumSteps; ++i)
  {
    m_fWeightedCompletion += GetStepWeight(m_iCurrentStep + i);
  }
  m_iCurrentStep += uiNumSteps;

  const double internalCompletion = m_fWeightedCompletion / m_fSummedWeight;
  const double finalCompletion    = m_fPercentageBase + internalCompletion * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
}

bool xiiProgressRange::SetCompletion(double fCompletionFactor)
{
  XII_ASSERT_DEV(m_fSummedWeight == 0.0, "This function is only supported if ProgressRange was initialized without steps");

  const double finalCompletion = m_fPercentageBase + fCompletionFactor * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
}

bool xiiProgressRange::WasCanceled() const
{
  if (!m_pProgressbar->m_bCancelClicked)
    return false;

  const xiiProgressRange* pCur = this;

  // if there is any action in the stack above, that cannot be canceled
  // all sub actions should be fully executed, even if they could be canceled
  while (pCur)
  {
    if (!pCur->m_bAllowCancel)
      return false;

    pCur = pCur->m_pParentRange;
  }

  return true;
}

XII_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Progress);
