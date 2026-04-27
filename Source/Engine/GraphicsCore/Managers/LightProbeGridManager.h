#pragma once

#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Components/Lights/LightProbeGridComponent.h>

/// \brief World module that bakes and streams irradiance probe grids.
///
/// Iterates all xiiLightProbeGridComponents in the world each frame (or on request),
/// determines which grids need an update, dispatches GPU bake work, and uploads
/// the resulting SH atlas to the component's resource handle.
class XII_GRAPHICSCORE_DLL xiiLightProbeGridManager : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiLightProbeGridManager, xiiWorldModule);

public:
  xiiLightProbeGridManager(xiiWorld* pWorld);
  ~xiiLightProbeGridManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  /// \brief Marks all grids dirty so they will be rebaked on the next update.
  void InvalidateAll();

  /// \brief Marks a specific grid dirty by component handle.
  void InvalidateGrid(xiiComponentHandle hComponent);

  /// \brief Returns whether any grids are currently pending a bake.
  bool HasPendingBakes() const { return !m_DirtyGrids.IsEmpty(); }

private:
  void Update(const xiiWorldModule::UpdateContext& ctx);

  xiiDynamicArray<xiiComponentHandle> m_DirtyGrids;
  bool                                m_bBakeOnNextUpdate = false;
};
