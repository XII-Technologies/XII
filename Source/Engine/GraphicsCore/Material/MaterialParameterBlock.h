/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Material/MaterialSchema.h>

/// Dense CPU representation of a material's typed values.
///
/// The byte block follows the schema's canonical GPU layout and can therefore be copied directly
/// into structured or constant-buffer storage. Variants are retained for editor inspection and
/// serialization, but never need to be searched by name during rendering.
class XII_GRAPHICSCORE_DLL xiiMaterialParameterBlock
{
public:
  xiiResult Initialize(xiiSharedPtr<const xiiMaterialSchema> pSchema);
  void      Clear();

  [[nodiscard]] const xiiSharedPtr<const xiiMaterialSchema>& GetSchema() const { return m_pSchema; }
  [[nodiscard]] xiiArrayPtr<const xiiUInt8> GetData() const { return m_Data; }
  [[nodiscard]] xiiArrayPtr<const xiiVariant> GetValues() const { return m_Values; }
  [[nodiscard]] xiiUInt32 GetRevision() const { return m_uiRevision; }
  [[nodiscard]] bool IsDirty() const { return m_uiDirtyStart != xiiInvalidIndex; }
  [[nodiscard]] xiiArrayPtr<const xiiUInt8> GetDirtyData() const;
  [[nodiscard]] xiiUInt32 GetDirtyOffset() const { return IsDirty() ? m_uiDirtyStart : 0U; }

  xiiResult SetValue(xiiMaterialParameterId id, const xiiVariant& value);
  xiiResult SetValue(const xiiTempHashedString& sName, const xiiVariant& value);
  [[nodiscard]] const xiiVariant* GetValue(xiiMaterialParameterId id) const;
  [[nodiscard]] const xiiVariant* GetValue(const xiiTempHashedString& sName) const;

  /// Marks the current bytes as consumed by a frame snapshot or GPU upload.
  void ClearDirtyRange();

private:
  xiiResult PackValue(const xiiMaterialParameterDefinition& definition, const xiiVariant& value);
  void      MarkDirty(xiiUInt32 uiOffset, xiiUInt32 uiSize);

  xiiSharedPtr<const xiiMaterialSchema> m_pSchema;
  xiiDynamicArray<xiiUInt8>             m_Data;
  xiiDynamicArray<xiiVariant>           m_Values;
  xiiUInt32                             m_uiRevision   = 0U;
  xiiUInt32                             m_uiDirtyStart = xiiInvalidIndex;
  xiiUInt32                             m_uiDirtyEnd   = 0U;
};
