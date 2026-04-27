#pragma once

#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Components/Lights/ReflectionProbeComponent.h>

/// \brief Sort key used to prioritize which probe gets updated this frame.
struct XII_GRAPHICSCORE_DLL xiiReflectionProbeSortKey
{
  float             m_fScore        = 0.0f; ///< Higher = update sooner.
  xiiComponentHandle m_hComponent;
};

/// \brief World module that manages realtime reflection probe capture scheduling.
///
/// Each frame the manager scores all active xiiReflectionProbeComponents (based on
/// camera distance, time-since-last-capture, and component priority) and dispatches
/// cube-map captures for the top-N probes within the per-frame budget.
class XII_GRAPHICSCORE_DLL xiiReflectionProbeManager : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionProbeManager, xiiWorldModule);

public:
  xiiReflectionProbeManager(xiiWorld* pWorld);
  ~xiiReflectionProbeManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  /// \brief Maximum number of realtime probe captures dispatched per frame.
  void     SetMaxCapturesPerFrame(xiiUInt32 n) { m_uiMaxCapturesPerFrame = n; }
  xiiUInt32 GetMaxCapturesPerFrame() const     { return m_uiMaxCapturesPerFrame; }

  /// \brief Forces all realtime probes to recapture on the next frame.
  void InvalidateAll();

private:
  void Update(const xiiWorldModule::UpdateContext& ctx);

  xiiUInt32                               m_uiMaxCapturesPerFrame = 2;
  xiiDynamicArray<xiiReflectionProbeSortKey> m_SortedProbes;
};
