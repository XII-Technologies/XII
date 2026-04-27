#pragma once

#include <GraphicsCore/Declarations.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <GAL/Device/GALDevice.h>
#include <GAL/Resources/GALBuffer.h>

// ============================================================
//  Vertex layout flags — controls which streams are uploaded
// ============================================================

struct XII_GRAPHICSCORE_DLL xiiDynamicMeshVertexDataFlags
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Position  = XII_BIT(0), ///< xiiVec3 positions (always present).
    Normal    = XII_BIT(1), ///< xiiVec3 normals.
    Tangent   = XII_BIT(2), ///< xiiVec4 tangents (W = bitangent sign).
    TexCoord0 = XII_BIT(3), ///< xiiVec2 UV channel 0.
    TexCoord1 = XII_BIT(4), ///< xiiVec2 UV channel 1.
    Color     = XII_BIT(5), ///< xiiColorLinearUB vertex colour.

    Default   = Position | Normal | TexCoord0,
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiDynamicMeshVertexDataFlags);

// ============================================================
//  Resource descriptor
// ============================================================

/// \brief CPU-side data used to create or update a xiiDynamicMeshResource.
///
/// The descriptor is intentionally lightweight — callers fill the arrays each
/// frame and call xiiDynamicMeshResource::UpdateVertices() / UpdateIndices().
struct XII_GRAPHICSCORE_DLL xiiDynamicMeshResourceDescriptor
{
  xiiDynamicArray<xiiVec3>          m_Positions;   ///< Vertex positions (required).
  xiiDynamicArray<xiiVec3>          m_Normals;     ///< Normals (optional, auto-computed if empty).
  xiiDynamicArray<xiiVec4>          m_Tangents;    ///< Tangents + bitangent sign in W.
  xiiDynamicArray<xiiVec2>          m_TexCoords0;  ///< UV channel 0.
  xiiDynamicArray<xiiVec2>          m_TexCoords1;  ///< UV channel 1.
  xiiDynamicArray<xiiColorLinearUB> m_Colors;      ///< Vertex colours.
  xiiDynamicArray<xiiUInt32>        m_Indices;     ///< Triangle index list (3 per triangle).

  xiiBoundingBoxSphere m_Bounds = xiiBoundingBoxSphere::MakeZero();

  xiiEnum<xiiDynamicMeshVertexDataFlags> m_VertexDataFlags = xiiDynamicMeshVertexDataFlags::Default;

  /// \brief Maximum vertex/index capacity to pre-allocate on the GPU (ring-buffer size).
  xiiUInt32 m_uiMaxVertices = 4096;
  xiiUInt32 m_uiMaxIndices  = 16384;

  /// \brief Number of in-flight frames buffered on the GPU to avoid stalls.
  xiiUInt8 m_uiRingBufferFrames = 2;
};

// ============================================================
//  Mesh sub-range (one draw call per sub-mesh)
// ============================================================

struct XII_GRAPHICSCORE_DLL xiiDynamicMeshSubMesh
{
  xiiUInt32                 m_uiFirstIndex   = 0;
  xiiUInt32                 m_uiIndexCount   = 0;
  xiiMaterialResourceHandle m_hMaterial;
};

// ============================================================
//  Resource class
// ============================================================

/// \brief A GPU-resident mesh whose geometry is updated from the CPU each frame.
///
/// Uses a double/triple-buffered ring of vertex and index upload heaps to
/// avoid GPU stalls. The caller updates geometry through:
///   - `BeginModifyGeometry()` to obtain scratch CPU pointers.
///   - `EndModifyGeometry()` to flush and advance the ring-buffer frame.
///
/// The resource owns the GPU vertex buffer (VB), index buffer (IB),
/// and an SRV-accessible structured buffer for use in vertex-pull compute shaders.
class XII_GRAPHICSCORE_DLL xiiDynamicMeshResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDynamicMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiDynamicMeshResource, xiiDynamicMeshResourceDescriptor);

public:
  xiiDynamicMeshResource();
  ~xiiDynamicMeshResource();

  // ---- Geometry update API ---------------------------------

  struct GeometryView
  {
    xiiArrayPtr<xiiVec3>          Positions;
    xiiArrayPtr<xiiVec3>          Normals;
    xiiArrayPtr<xiiVec4>          Tangents;
    xiiArrayPtr<xiiVec2>          TexCoords0;
    xiiArrayPtr<xiiVec2>          TexCoords1;
    xiiArrayPtr<xiiColorLinearUB> Colors;
    xiiArrayPtr<xiiUInt32>        Indices;
  };

  /// \brief Returns writable CPU pointers for the current ring-buffer slot.
  /// \note Must be called exactly once per frame before any rendering.
  GeometryView BeginModifyGeometry(xiiUInt32 uiVertexCount, xiiUInt32 uiIndexCount);

  /// \brief Finalises the upload and records the actual bounding volume.
  void EndModifyGeometry(const xiiBoundingBoxSphere& bounds);

  /// \brief Updates sub-mesh (draw range + material) definitions.
  void SetSubMeshes(xiiArrayPtr<const xiiDynamicMeshSubMesh> subMeshes);

  // ---- GPU resource accessors ------------------------------

  xiiGALBufferHandle GetVertexBuffer()   const;
  xiiGALBufferHandle GetIndexBuffer()    const;

  /// \brief Returns the current byte offset into the vertex buffer for this frame's data.
  xiiUInt32 GetVertexBufferOffset() const { return m_uiCurrentVertexOffset; }
  xiiUInt32 GetIndexBufferOffset()  const { return m_uiCurrentIndexOffset; }
  xiiUInt32 GetCurrentVertexCount() const { return m_uiCurrentVertexCount; }
  xiiUInt32 GetCurrentIndexCount()  const { return m_uiCurrentIndexCount; }

  xiiArrayPtr<const xiiDynamicMeshSubMesh> GetSubMeshes() const { return m_SubMeshes; }

  const xiiBoundingBoxSphere& GetBounds() const { return m_CurrentBounds; }

protected:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                ReportResourceIsMissing() override;

private:
  void AllocateRingBuffer(const xiiDynamicMeshResourceDescriptor& desc);

  // Ring-buffered GPU upload heaps
  struct RingFrame
  {
    xiiGALBufferHandle m_hVertexUpload;
    xiiGALBufferHandle m_hIndexUpload;
    void*              m_pMappedVertex = nullptr;
    void*              m_pMappedIndex  = nullptr;
    xiiUInt64          m_uiFenceValue  = 0;
  };

  xiiStaticArray<RingFrame, 3> m_RingFrames;
  xiiGALBufferHandle           m_hVertexBuffer;
  xiiGALBufferHandle           m_hIndexBuffer;

  xiiDynamicArray<xiiDynamicMeshSubMesh> m_SubMeshes;

  xiiBoundingBoxSphere m_CurrentBounds = xiiBoundingBoxSphere::MakeZero();

  xiiUInt32 m_uiMaxVertices         = 0;
  xiiUInt32 m_uiMaxIndices          = 0;
  xiiUInt32 m_uiCurrentVertexOffset = 0;
  xiiUInt32 m_uiCurrentIndexOffset  = 0;
  xiiUInt32 m_uiCurrentVertexCount  = 0;
  xiiUInt32 m_uiCurrentIndexCount   = 0;
  xiiUInt8  m_uiRingFrames          = 2;
  xiiUInt8  m_uiCurrentFrame        = 0;

  xiiEnum<xiiDynamicMeshVertexDataFlags> m_VertexDataFlags;
};
