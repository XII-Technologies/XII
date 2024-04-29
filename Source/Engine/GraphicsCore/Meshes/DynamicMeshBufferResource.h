#pragma once

#include <Core/ResourceManager/Resource.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

using xiiDynamicMeshBufferResourceHandle = xiiTypedResourceHandle<class xiiDynamicMeshBufferResource>;

struct xiiDynamicMeshBufferResourceDescriptor
{
  xiiEnum<xiiGALPrimitiveTopology> m_Topology        = xiiGALPrimitiveTopology::TriangleList;
  xiiEnum<xiiGALValueType>         m_IndexType       = xiiGALValueType::UInt32;
  xiiUInt32                        m_uiMaxPrimitives = 0;
  xiiUInt32                        m_uiMaxVertices   = 0;
  bool                             m_bColorStream    = false;
};

struct XII_GRAPHICSCORE_DLL xiiDynamicMeshVertex
{
  XII_DECLARE_POD_TYPE();

  xiiVec3 m_vPosition;
  xiiVec2 m_vTexCoord;
  xiiVec3 m_vEncodedNormal;
  xiiVec4 m_vEncodedTangent;
  //xiiColorLinearUB m_Color;

  XII_ALWAYS_INLINE void EncodeNormal(const xiiVec3& vNormal)
  {
    // store in [0; 1] range
    m_vEncodedNormal = vNormal * 0.5f + xiiVec3(0.5f);

    // this is the same
    //xiiMeshBufferUtils::EncodeNormal(normal, xiiByteArrayPtr(reinterpret_cast<xiiUInt8*>(&m_vEncodedNormal), sizeof(xiiVec3)), xiiMeshNormalPrecision::_32Bit).IgnoreResult();
  }

  XII_ALWAYS_INLINE void EncodeTangent(const xiiVec3& vTangent, float fBitangentSign)
  {
    // store in [0; 1] range
    m_vEncodedTangent.x = vTangent.x * 0.5f + 0.5f;
    m_vEncodedTangent.y = vTangent.y * 0.5f + 0.5f;
    m_vEncodedTangent.z = vTangent.z * 0.5f + 0.5f;
    m_vEncodedTangent.w = fBitangentSign < 0.0f ? 0.0f : 1.0f;

    // this is the same
    //xiiMeshBufferUtils::EncodeTangent(tangent, bitangentSign, xiiByteArrayPtr(reinterpret_cast<xiiUInt8*>(&m_vEncodedTangent), sizeof(xiiVec4)), xiiMeshNormalPrecision::_32Bit).IgnoreResult();
  }
};

class XII_GRAPHICSCORE_DLL xiiDynamicMeshBufferResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicMeshBufferResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDynamicMeshBufferResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiDynamicMeshBufferResource, xiiDynamicMeshBufferResourceDescriptor);

public:
  xiiDynamicMeshBufferResource();
  ~xiiDynamicMeshBufferResource();

  XII_ALWAYS_INLINE const xiiDynamicMeshBufferResourceDescriptor& GetDescriptor() const { return m_Descriptor; }
  XII_ALWAYS_INLINE xiiGALBufferHandle                            GetVertexBuffer() const { return m_hVertexBuffer; }
  XII_ALWAYS_INLINE xiiGALBufferHandle                            GetIndexBuffer() const { return m_hIndexBuffer; }
  XII_ALWAYS_INLINE xiiGALBufferHandle                            GetColorBuffer() const { return m_hColorBuffer; }

  /// \brief Grants write access to the vertex data, and flags the data as 'dirty'.
  xiiArrayPtr<xiiDynamicMeshVertex> AccessVertexData()
  {
    m_bAccessedVB = true;
    return m_VertexData;
  }

  /// \brief Grants write access to the 16 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 16 bit indices.
  xiiArrayPtr<xiiUInt16> AccessIndex16Data()
  {
    m_bAccessedIB = true;
    return m_Index16Data;
  }

  /// \brief Grants write access to the 32 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 32 bit indices.
  xiiArrayPtr<xiiUInt32> AccessIndex32Data()
  {
    m_bAccessedIB = true;
    return m_Index32Data;
  }

  /// \brief Grants write access to the color data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if creation of the color buffer was enabled.
  xiiArrayPtr<xiiColorLinearUB> AccessColorData()
  {
    m_bAccessedCB = true;
    return m_ColorData;
  }

  const xiiInputLayoutInfo& GetInputLayout() const { return m_InputLayout; }

  /// \brief Uploads the current vertex and index data to the GPU.
  ///
  /// If all values are set to default, the entire data is uploaded.
  /// If \a uiNumVertices or \a uiNumIndices is set to the max value, all vertices or indices (after their start offset) are uploaded.
  ///
  /// In all other cases, the number of elements to upload must be within valid bounds.
  ///
  /// This function can be used to only upload a subset of the modified data.
  ///
  /// Note that this function doesn't do anything, if the vertex or index data wasn't recently accessed through AccessVertexData(), AccessIndex16Data() or AccessIndex32Data(). So if you want to upload multiple pieces of the data to the GPU, you have to call these functions in between to flag the uploaded data as out-of-date.
  void UpdateGpuBuffer(xiiGALCommandList* pGALCommandList, xiiUInt32 uiFirstVertex = 0, xiiUInt32 uiNumVertices = xiiMath::MaxValue<xiiUInt32>(), xiiUInt32 uiFirstIndex = 0, xiiUInt32 uiNumIndices = xiiMath::MaxValue<xiiUInt32>(), xiiBitflags<xiiGALMapFlags> mapFlags = xiiGALMapFlags::Discard);

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  bool m_bAccessedVB = false;
  bool m_bAccessedIB = false;
  bool m_bAccessedCB = false;

  xiiGALBufferHandle                     m_hVertexBuffer;
  xiiGALBufferHandle                     m_hIndexBuffer;
  xiiGALBufferHandle                     m_hColorBuffer;
  xiiDynamicMeshBufferResourceDescriptor m_Descriptor;

  xiiInputLayoutInfo                                                m_InputLayout;
  xiiDynamicArray<xiiDynamicMeshVertex, xiiAlignedAllocatorWrapper> m_VertexData;
  xiiDynamicArray<xiiUInt16, xiiAlignedAllocatorWrapper>            m_Index16Data;
  xiiDynamicArray<xiiUInt32, xiiAlignedAllocatorWrapper>            m_Index32Data;
  xiiDynamicArray<xiiColorLinearUB, xiiAlignedAllocatorWrapper>     m_ColorData;
};
