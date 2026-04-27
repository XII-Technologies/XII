#pragma once

#include <GraphicsCore/Declarations.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Vec3.h>

// ============================================================
//  Wire-format structures  (GPU-facing, must stay POD / packed)
// ============================================================

/// \brief Per-meshlet descriptor stored in the xiiMeshletResource.
///
/// A meshlet is a small cluster of up to 64 vertices and 124 triangles.
/// This layout matches the NV_mesh_shader / EXT_mesh_shader conventions and
/// the output produced by meshoptimizer::meshopt_buildMeshlets().
struct XII_GRAPHICSCORE_DLL xiiMeshletDescriptor
{
  xiiUInt32 m_uiVertexOffset;    ///< First vertex index into the packed vertex buffer.
  xiiUInt32 m_uiTriangleOffset;  ///< First byte into the triangle index list (3 bytes per tri).
  xiiUInt8  m_uiVertexCount;     ///< Number of unique vertices in this meshlet (≤ 64).
  xiiUInt8  m_uiTriangleCount;   ///< Number of triangles in this meshlet (≤ 124).
  xiiUInt16 m_uiPad;
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiMeshletDescriptor) == 12);

/// \brief Tight spatial bounds + backface cone for per-meshlet GPU culling.
///
/// The cone normal and apex allow an efficient backface-cone cull in the
/// task/amplification shader stage before any vertex work is done.
struct XII_GRAPHICSCORE_DLL xiiMeshletBounds
{
  xiiVec3  m_vCenter;       ///< Bounding sphere centre (object space).
  float    m_fRadius;       ///< Bounding sphere radius.
  xiiVec3  m_vConeApex;     ///< Cone apex (object space, = tightest vertex along -normal).
  float    m_fConeCutoff;   ///< cos(backface half-angle); meshlet invisible if dot(view, normal) < cutoff.
  xiiVec3  m_vConeAxis;     ///< Normalised mean cluster normal (object space).
  xiiInt8  m_iConeAxisS8[3];///< S8-normalised cone axis for GPU-side cull (packed alternative).
  xiiInt8  m_iConeCutoffS8; ///< S8-normalised cone cutoff.
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiMeshletBounds) == 48);

/// \brief Quantised, interleaved per-vertex payload stored in the meshlet resource.
///
/// All attributes are packed to minimise vertex-fetch bandwidth.
/// Position is stored as 16-bit fixed-point relative to the meshlet AABB,
/// tangent frame as an Oct-encoded 16-bit pair (R16G16_SNORM),
/// UV as fp16 pair.
struct XII_GRAPHICSCORE_DLL xiiPackedMeshletVertex
{
  xiiUInt16 m_uiPosX, m_uiPosY, m_uiPosZ; ///< 16-bit quantised position (decode via meshlet AABB).
  xiiUInt16 m_uiPosW;                       ///< Padding to 8 bytes.

  xiiInt16  m_iNormalX, m_iNormalY;         ///< Oct-encoded world normal (SNORM16).
  xiiInt16  m_iTangentX, m_iTangentY;       ///< Oct-encoded tangent (SNORM16), W sign in bit 15.

  xiiUInt16 m_uiUV0X, m_uiUV0Y;             ///< UV channel 0 (fp16).
  xiiUInt16 m_uiUV1X, m_uiUV1Y;             ///< UV channel 1 (fp16), for lightmaps / decals.
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiPackedMeshletVertex) == 24);

/// \brief Per-meshlet decode constants needed to reconstruct the original position.
struct XII_GRAPHICSCORE_DLL xiiMeshletAABB
{
  xiiVec3 m_vMin; ///< Object-space AABB minimum.
  float   m_fPad0;
  xiiVec3 m_vMax; ///< Object-space AABB maximum.
  float   m_fPad1;
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiMeshletAABB) == 32);

// ============================================================
//  Resource descriptor (used by the asset pipeline)
// ============================================================

struct XII_GRAPHICSCORE_DLL xiiMeshletResourceDescriptor
{
  /// Packed vertex data — one entry per unique vertex across all meshlets in this LOD.
  xiiDynamicArray<xiiPackedMeshletVertex> m_PackedVertices;

  /// Global vertex remap: local meshlet vertex index → global vertex index in the source mesh.
  xiiDynamicArray<xiiUInt32> m_VertexRemap;

  /// Compact triangle index list.  Each triangle = 3 consecutive xiiUInt8 local vertex indices.
  xiiDynamicArray<xiiUInt8> m_TriangleIndices;

  /// One descriptor per meshlet.
  xiiDynamicArray<xiiMeshletDescriptor> m_Meshlets;

  /// Per-meshlet culling bounds.
  xiiDynamicArray<xiiMeshletBounds> m_MeshletBounds;

  /// Per-meshlet decode AABB (parallel to m_Meshlets).
  xiiDynamicArray<xiiMeshletAABB> m_MeshletAABBs;

  /// Number of LOD levels stored consecutively. Each LOD is a contiguous sub-range of m_Meshlets.
  xiiUInt8 m_uiNumLODs = 1;

  /// Meshlet count for each LOD (up to 8 LODs).
  xiiUInt32 m_LODMeshletCounts[8] = {};

  /// First meshlet index for each LOD.
  xiiUInt32 m_LODMeshletOffsets[8] = {};

  /// Screen-coverage threshold at which each LOD level is selected (0 = always, 1 = never).
  float m_LODScreenCoverage[8] = {};
};

// ============================================================
//  Resource class
// ============================================================

class XII_GRAPHICSCORE_DLL xiiMeshletResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshletResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshletResource, xiiMeshletResourceDescriptor);

public:
  xiiMeshletResource();
  ~xiiMeshletResource();

  // ---- Accessors ------------------------------------------------

  xiiUInt32 GetMeshletCount() const { return m_uiMeshletCount; }
  xiiUInt32 GetVertexCount()  const { return m_uiVertexCount; }
  xiiUInt32 GetTriangleCount()const { return m_uiTriangleCount; }
  xiiUInt8  GetLODCount()     const { return m_uiNumLODs; }

  /// \brief Returns the [offset, count) range of meshlets for the given LOD level.
  void GetLODRange(xiiUInt8 uiLOD, xiiUInt32& out_uiOffset, xiiUInt32& out_uiCount) const;

  /// \brief Returns the GPU structured buffer containing all xiiMeshletDescriptor records.
  /// The buffer is valid after the resource transitions to Loaded state.
  xiiGALBufferHandle GetDescriptorBuffer()  const { return m_hDescriptorBuffer; }

  /// \brief Returns the GPU structured buffer containing all xiiMeshletBounds records.
  xiiGALBufferHandle GetBoundsBuffer()      const { return m_hBoundsBuffer; }

  /// \brief Returns the GPU buffer containing all xiiPackedMeshletVertex records.
  xiiGALBufferHandle GetVertexBuffer()      const { return m_hVertexBuffer; }

  /// \brief Returns the GPU buffer containing the compact triangle index list (R8_UINT).
  xiiGALBufferHandle GetTriangleBuffer()    const { return m_hTriangleBuffer; }

  /// \brief Returns the GPU buffer containing per-meshlet AABB decode constants.
  xiiGALBufferHandle GetAABBBuffer()        const { return m_hAABBBuffer; }

protected:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                ReportResourceIsMissing() override;

private:
  void CreateGPUBuffers(const xiiMeshletResourceDescriptor& desc);

  xiiGALBufferHandle m_hDescriptorBuffer; ///< GPU buffer — xiiMeshletDescriptor[]
  xiiGALBufferHandle m_hBoundsBuffer;     ///< GPU buffer — xiiMeshletBounds[]
  xiiGALBufferHandle m_hVertexBuffer;     ///< GPU buffer — xiiPackedMeshletVertex[]
  xiiGALBufferHandle m_hTriangleBuffer;   ///< GPU buffer — xiiUInt8[] (triangle indices)
  xiiGALBufferHandle m_hAABBBuffer;       ///< GPU buffer — xiiMeshletAABB[]

  xiiUInt32 m_uiMeshletCount  = 0;
  xiiUInt32 m_uiVertexCount   = 0;
  xiiUInt32 m_uiTriangleCount = 0;
  xiiUInt8  m_uiNumLODs       = 1;

  xiiUInt32 m_LODMeshletOffsets[8] = {};
  xiiUInt32 m_LODMeshletCounts[8]  = {};
  float     m_LODScreenCoverage[8] = {};
};
