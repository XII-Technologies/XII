/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>

/// This describes the occlusion query data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryDataOcclusion : public xiiHashableStruct<xiiGALQueryDataOcclusion>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALQueryType> m_Type          = xiiGALQueryType::Occlusion; ///< Query type.
  xiiUInt64                m_uiSampleCount = 0U;                         ///< The number of samples that passed the depth and stencil tests in between begin / end query.
};

/// This describes the binary occlusion query data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryDataBinaryOcclusion : public xiiHashableStruct<xiiGALQueryDataBinaryOcclusion>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALQueryType> m_Type              = xiiGALQueryType::BinaryOcclusion; ///< Query type.
  bool                     m_bAnySamplesPassed = false;                            ///< Indicates if at least one sample passed depth and stencil testing in between begin / end query.
};

/// This describes the timestamp query data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryDataTimestamp : public xiiHashableStruct<xiiGALQueryDataTimestamp>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALQueryType> m_Type        = xiiGALQueryType::Timestamp; ///< Query type.
  xiiUInt64                m_uiCounter   = 0U;                         ///< The value of a high-frequency counter.
  xiiUInt64                m_uiFrequency = 0U;                         ///< The counter frequency, in Hz (ticks/second). If there was an error while getting the timestamp, this value will be 0.
};

/// This describes the pipeline statistics query data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryDataPipelineStatistics : public xiiHashableStruct<xiiGALQueryDataPipelineStatistics>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALQueryType> m_Type                  = xiiGALQueryType::PipelineStatistics; ///< Query type.
  xiiUInt64                m_uiInputVertices       = 0U;                                  ///< Number of vertices processed by the input assembler stage.
  xiiUInt64                m_uiInputPrimitives     = 0U;                                  ///< Number of primitives processed by the input assembler stage.
  xiiUInt64                m_uiGSPrimitives        = 0U;                                  ///< Number of primitives output by a geometry shader.
  xiiUInt64                m_uiClippingInvocations = 0U;                                  ///< Number of primitives that were sent to the clipping stage.
  xiiUInt64                m_uiClippingPrimitives  = 0U;                                  ///< Number of primitives that were output by the clipping stage and were rendered. This may be larger or smaller than the clipping invocations because after a primitive is clipped sometimes it is either broken up into more than one primitive or completely culled.
  xiiUInt64                m_uiVSInvocations       = 0U;                                  ///< Number of times a vertex shader was invoked.
  xiiUInt64                m_uiGSInvocations       = 0U;                                  ///< Number of times a geometry shader was invoked.
  xiiUInt64                m_uiPSInvocations       = 0U;                                  ///< Number of times a pixel shader was invoked.
  xiiUInt64                m_uiHSInvocations       = 0U;                                  ///< Number of times a hull shader was invoked.
  xiiUInt64                m_uiDSInvocations       = 0U;                                  ///< Number of times a domain shader was invoked.
  xiiUInt64                m_uiCSInvocations       = 0U;                                  ///< Number of times a compute shader was invoked.
};

/// This describes the duration query data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryDataDuration : public xiiHashableStruct<xiiGALQueryDataDuration>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALQueryType> m_Type        = xiiGALQueryType::Duration; ///< Query type.
  xiiUInt64                m_uiDuration  = 0U;                        ///< The number of high-frequency counter ticks between begin / end query.
  xiiUInt64                m_uiFrequency = 0U;                        ///< The counter frequency, in Hz (ticks/second). If there was an error while getting the timestamp, this value will be 0.
};

/// This describes the query creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALQueryCreationDescription : public xiiHashableStruct<xiiGALQueryCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALQueryType> m_Type = xiiGALQueryType::Undefined; ///< Query type.
};

/// Interface that defines methods to manipulate a query object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALQuery : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALQuery, xiiGALDeviceObject);

public:
  enum class QueryState
  {
    Inactive, ///< No query has been initiated yet.
    Querying, ///< A query is currently in progress.
    Ended     ///< The query has completed.
  };

  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALQueryCreationDescription& GetDescription() const { return m_Description; }

  /// This retrieves the query data.
  ///
  /// \param pData           - The pointer to the query data structure. This must be a pointer to one of Occlusion, BinaryOcclusion, Timestamp, PipelineStatistics, and Duration structures. An application may provide nullptr to only check the query status.
  /// \param uiDataSize      - The size of the data structure.
  /// \param bAutoInvalidate - Whether to invalidate the query if the results are available and release associated resources. An application should typically always invalidate completed queries unless it needs to retrieve the same data through GetData() multiple times. A query will not be invalidated if pData is nullptr.
  ///
  /// \return True if the query data is available, false otherwise.
  ///
  /// \note  In Direct3D11 backend timestamp queries will only be available after FinishFrame is called for the frame in which they were collected. If AutoInvalidate is set to true, and the data have been retrieved, an application must not call GetData() until it begins and ends the query again.
  [[nodiscard]] virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) = 0;

  /// This invalidates the query and releases the associated resources.
  virtual void Invalidate();

  /// This retrieves the current query state.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQuery::QueryState GetQueryState() const { return m_QueryState; }

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandList;

  xiiGALQuery(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQuery();

  virtual xiiResult InitPlatform() = 0;

  void OnBeginQuery(xiiGALCommandList* pCommandList);
  void OnEndQuery(xiiGALCommandList* pCommandList);

  void CheckQueryDataPtr(void* pData, xiiUInt32 uiDataSize);

protected:
  xiiGALQueryCreationDescription m_Description;

  xiiGALCommandList* m_pCommandList = nullptr;

  QueryState m_QueryState = QueryState::Inactive;
};
