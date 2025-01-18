#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsFoundation/Shader/InputLayout.h>

using xiiMeshBufferResourceHandle = xiiTypedResourceHandle<class xiiMeshBufferResource>;
class xiiGeometry;

struct XII_GRAPHICSCORE_DLL xiiVertexStreamInfo : public xiiHashableStruct<xiiVertexStreamInfo>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALInputLayoutSemantic> m_Semantic;
  xiiUInt8                           m_uiVertexBufferSlot = 0;
  xiiEnum<xiiGALResourceFormat>      m_Format;
  xiiUInt16                          m_uiOffset;      ///< at which byte offset the first element starts
  xiiUInt16                          m_uiElementSize; ///< the number of bytes for this element type (depends on the format); this is not the stride between elements!
};

struct XII_GRAPHICSCORE_DLL xiiInputLayoutInfo
{
  void ComputeHash();

  xiiHybridArray<xiiVertexStreamInfo, 8> m_VertexStreams;
  xiiUInt32                              m_uiHash;
};


struct XII_GRAPHICSCORE_DLL xiiMeshBufferResourceDescriptor
{
public:
  xiiMeshBufferResourceDescriptor();
  ~xiiMeshBufferResourceDescriptor();

  void Clear();

  /// \brief Use this function to add vertex streams to the mesh buffer. The return value is the index of the just added stream.
  xiiUInt32 AddStream(xiiEnum<xiiGALInputLayoutSemantic> semantic, xiiEnum<xiiGALResourceFormat> format);

  /// \brief Adds common vertex streams to the mesh buffer.
  ///
  /// The streams are added in this order (with the corresponding stream indices):
  /// * Position (index 0)
  /// * TexCoord0 (index 1)
  /// * Normal (index 2)
  /// * Tangent (index 3)
  void AddCommonStreams();

  /// \brief After all streams are added, call this to allocate the data for the streams. If uiNumPrimitives is 0, the mesh buffer will not
  /// use indexed rendering.
  void AllocateStreams(xiiUInt32 uiNumVertices, xiiEnum<xiiGALPrimitiveTopology> topology = xiiGALPrimitiveTopology::TriangleList, xiiUInt32 uiNumPrimitives = 0, bool bZeroFill = false);

  /// \brief Creates streams and fills them with data from the xiiGeometry. Only the geometry matching the given topology is used.
  ///  Streams that do not match any of the data inside the xiiGeometry directly are skipped.
  void AllocateStreamsFromGeometry(const xiiGeometry& geom, xiiEnum<xiiGALPrimitiveTopology> topology = xiiGALPrimitiveTopology::TriangleList);

  /// \brief Gives read access to the allocated vertex data
  xiiArrayPtr<const xiiUInt8> GetVertexBufferData() const;

  /// \brief Gives read access to the allocated index data
  xiiArrayPtr<const xiiUInt8> GetIndexBufferData() const;

  /// \brief Allows write access to the allocated vertex data. This can be used for copying data fast into the array.
  xiiDynamicArray<xiiUInt8, xiiAlignedAllocatorWrapper>& GetVertexBufferData();

  /// \brief Allows write access to the allocated index data. This can be used for copying data fast into the array.
  xiiDynamicArray<xiiUInt8, xiiAlignedAllocatorWrapper>& GetIndexBufferData();

  /// \brief Slow, but convenient method to write one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  /// data is the piece of data to write to the stream.
  template <typename TYPE>
  void SetVertexData(xiiUInt32 uiStream, xiiUInt32 uiVertexIndex, const TYPE& data)
  {
    reinterpret_cast<TYPE&>(m_VertexStreamData[m_uiVertexSize * uiVertexIndex + m_InputLayout.m_VertexStreams[uiStream].m_uiOffset]) = data;
  }

  /// \brief Slow, but convenient method to access one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  xiiArrayPtr<xiiUInt8> GetVertexData(xiiUInt32 uiStream, xiiUInt32 uiVertexIndex) { return m_VertexStreamData.GetArrayPtr().GetSubArray(m_uiVertexSize * uiVertexIndex + m_InputLayout.m_VertexStreams[uiStream].m_uiOffset); }

  /// \brief Writes the vertex index for the given point into the index buffer.
  void SetPointIndices(xiiUInt32 uiPoint, xiiUInt32 uiVertex0);

  /// \brief Writes the two vertex indices for the given line into the index buffer.
  void SetLineIndices(xiiUInt32 uiLine, xiiUInt32 uiVertex0, xiiUInt32 uiVertex1);

  /// \brief Writes the three vertex indices for the given triangle into the index buffer.
  void SetTriangleIndices(xiiUInt32 uiTriangle, xiiUInt32 uiVertex0, xiiUInt32 uiVertex1, xiiUInt32 uiVertex2);

  /// \brief Allows to read the stream info of the descriptor, which is filled out by AddStream()
  const xiiInputLayoutInfo& GetInputLayout() const { return m_InputLayout; }

  /// \brief Returns the byte size of all the data for one vertex.
  xiiUInt32 GetVertexDataSize() const { return m_uiVertexSize; }

  /// \brief Return the number of vertices, with which AllocateStreams() was called.
  xiiUInt32 GetVertexCount() const { return m_uiVertexCount; }

  /// \brief Returns the number of primitives that the array holds.
  xiiUInt32 GetPrimitiveCount() const;

  /// \brief Returns whether 16 or 32 Bit indices are to be used.
  bool Uses32BitIndices() const { return m_uiVertexCount > 0xFFFF; }

  /// \brief Returns whether an index buffer is available.
  bool HasIndexBuffer() const { return !m_IndexBufferData.IsEmpty(); }

  /// \brief Calculates the bounds using the data from the position stream
  xiiBoundingBoxSphere ComputeBounds() const;

  /// \brief Returns the primitive topology
  xiiEnum<xiiGALPrimitiveTopology> GetTopology() const { return m_Topology; }

  xiiResult RecomputeNormals();

private:
  xiiEnum<xiiGALPrimitiveTopology>                      m_Topology;
  xiiUInt32                                             m_uiVertexSize;
  xiiUInt32                                             m_uiVertexCount;
  xiiInputLayoutInfo                                    m_InputLayout;
  xiiDynamicArray<xiiUInt8, xiiAlignedAllocatorWrapper> m_VertexStreamData;
  xiiDynamicArray<xiiUInt8, xiiAlignedAllocatorWrapper> m_IndexBufferData;
};

class XII_GRAPHICSCORE_DLL xiiMeshBufferResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshBufferResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshBufferResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshBufferResource, xiiMeshBufferResourceDescriptor);

public:
  xiiMeshBufferResource() :
    xiiResource(DoUpdate::OnAnyThread, 1)
  {
  }

  ~xiiMeshBufferResource();

  XII_ALWAYS_INLINE xiiUInt32 GetPrimitiveCount() const { return m_uiPrimitiveCount; }

  XII_ALWAYS_INLINE xiiGALBufferHandle GetVertexBuffer() const { return m_hVertexBuffer; }

  XII_ALWAYS_INLINE xiiGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  XII_ALWAYS_INLINE xiiEnum<xiiGALPrimitiveTopology> GetTopology() const { return m_Topology; }

  /// \brief Returns the vertex declaration used by this mesh buffer.
  const xiiInputLayoutInfo& GetInputLayout() const { return m_InputLayout; }

  /// \brief Returns the bounds of the mesh
  const xiiBoundingBoxSphere& GetBounds() const { return m_Bounds; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiBoundingBoxSphere             m_Bounds;
  xiiInputLayoutInfo               m_InputLayout;
  xiiUInt32                        m_uiPrimitiveCount = 0;
  xiiGALBufferHandle               m_hVertexBuffer;
  xiiGALBufferHandle               m_hIndexBuffer;
  xiiEnum<xiiGALPrimitiveTopology> m_Topology;
};
