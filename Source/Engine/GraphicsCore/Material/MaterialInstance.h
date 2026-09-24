/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Material/MaterialParameterBlock.h>

/// One schema-defined resource binding. The bindless index is resolved by the renderer and is
/// intentionally kept separate from the resource handle so streaming can replace either safely.
struct XII_GRAPHICSCORE_DLL xiiMaterialResourceBinding
{
  xiiMaterialParameterId         m_Id;
  xiiTexture2DResourceHandle     m_hTexture2D;
  xiiTextureCubeResourceHandle   m_hTextureCube;
  xiiUInt32                      m_uiBindlessIndex = xiiInvalidIndex;
  xiiUInt32                      m_uiRevision      = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialResourceBinding);

/// Immutable frame-local material data. Render extraction consumes snapshots rather than locking
/// live material objects while command lists are being recorded on worker threads.
struct XII_GRAPHICSCORE_DLL xiiMaterialInstanceSnapshot
{
  xiiSharedPtr<const xiiMaterialSchema>       m_pSchema;
  xiiDynamicArray<xiiUInt8>                   m_ParameterData;
  xiiDynamicArray<xiiMaterialResourceBinding> m_ResourceBindings;
  xiiMaterialRuntimeState                     m_RuntimeState;
  xiiBitflags<xiiMaterialDirtyFlags>          m_DirtyFlags = xiiMaterialDirtyFlags::None;
  xiiUInt32                                   m_uiRevision  = 0U;
};

/// Mutable, thread-safe runtime material instance.
///
/// Instances share an immutable schema, but own their parameter bytes and resource bindings. This
/// makes one authored material usable as a template for thousands of inexpensive simulation or
/// rendering instances without modifying the resource-manager object.
class XII_GRAPHICSCORE_DLL xiiMaterialInstance final : public xiiRefCounted
{
public:
  xiiResult Initialize(xiiSharedPtr<const xiiMaterialSchema> pSchema, const xiiMaterialRuntimeState& runtimeState = {});

  xiiResult SetParameter(xiiMaterialParameterId id, const xiiVariant& value);
  xiiResult SetParameter(xiiStringView sName, const xiiVariant& value);
  [[nodiscard]] xiiVariant GetParameter(xiiMaterialParameterId id) const;

  xiiResult SetTexture2D(xiiMaterialParameterId id, const xiiTexture2DResourceHandle& hTexture);
  xiiResult SetTextureCube(xiiMaterialParameterId id, const xiiTextureCubeResourceHandle& hTexture);
  xiiResult SetBindlessIndex(xiiMaterialParameterId id, xiiUInt32 uiBindlessIndex);

  void SetRuntimeState(const xiiMaterialRuntimeState& runtimeState);
  [[nodiscard]] xiiMaterialRuntimeState GetRuntimeState() const;
  [[nodiscard]] xiiSharedPtr<const xiiMaterialSchema> GetSchema() const;
  [[nodiscard]] xiiUInt32 GetRevision() const;

  /// Copies a coherent view without consuming dirty state.
  void CreateSnapshot(xiiMaterialInstanceSnapshot& out_snapshot) const;

  /// Copies a coherent view and consumes the aggregate dirty flags. GPU storage uses per-frame
  /// revisions as the final authority, so consuming these flags never loses an upload.
  void ConsumeSnapshot(xiiMaterialInstanceSnapshot& out_snapshot);

private:
  xiiResult SetResourceBinding(xiiMaterialParameterId id, const xiiTexture2DResourceHandle* pTexture2D, const xiiTextureCubeResourceHandle* pTextureCube, const xiiUInt32* pBindlessIndex);
  void      CreateSnapshotLocked(xiiMaterialInstanceSnapshot& out_snapshot) const;

  mutable xiiMutex                         m_Mutex;
  xiiSharedPtr<const xiiMaterialSchema>    m_pSchema;
  xiiMaterialParameterBlock                m_Parameters;
  xiiDynamicArray<xiiMaterialResourceBinding> m_ResourceBindings;
  xiiMaterialRuntimeState                  m_RuntimeState;
  xiiBitflags<xiiMaterialDirtyFlags>       m_DirtyFlags = xiiMaterialDirtyFlags::None;
  xiiUInt32                                m_uiRevision  = 0U;
};
