/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

/// This describes the ray tracing instance flags used by the top level acceleration structure.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingInstanceFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None                          = 0U,         ///< No instance flags are set. Triangles use clockwise winding for front-facing determination, and ray-specified front/back-face culling is permitted.
    TriangleCullDisable           = XII_BIT(0), ///< Disables front- and back-face culling for triangles in this instance. Ray flags requesting front- or back-facing triangle culling have no effect.
    TriangleFrontCounterClockwise = XII_BIT(1), ///< Treats counter-clockwise triangles as front-facing. By default, clockwise triangles are considered front-facing.
    ForceOpaque                   = XII_BIT(2), ///< Forces all geometries in this instance to be treated as opaque, overriding their per-geometry opaque state unless overridden by a ray flag.
    ForceNonOpaque                = XII_BIT(3), ///< Forces all geometries in this instance to be treated as non-opaque, overriding their per-geometry opaque state unless overridden by a ray flag.

    All = TriangleCullDisable | TriangleFrontCounterClockwise | ForceOpaque | ForceNonOpaque,

    Default = None
  };

  struct Bits
  {
    StorageType TriangleCullDisable : 1;
    StorageType TriangleFrontCounterClockwise : 1;
    StorageType ForceOpaque : 1;
    StorageType ForceNonOpaque : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALRayTracingInstanceFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALRayTracingInstanceFlags);

/// Backend-independent TLAS instance source record.
///
/// Upload these records with xiiGALCommandList::UpdateTLASInstanceBuffer(). The command list converts the portable fields to the native backend representation before a TLAS build consumes them.
struct XII_GRAPHICSFOUNDATION_DLL alignas(16) xiiGALTLASInstanceData
{
  XII_DECLARE_POD_TYPE();

  static constexpr xiiUInt32 s_uiEncodedSize = 64U;

  [[nodiscard]] xiiMat4 GetTransform() const;
  void                  SetTransform(xiiMat4 transform);

  [[nodiscard]] xiiUInt32 GetInstanceID() const { return m_uiInstanceIDAndMask & 0x00FFFFFFU; }
  void                    SetInstanceID(xiiUInt32 uiInstanceID);

  [[nodiscard]] xiiUInt8 GetMask() const { return static_cast<xiiUInt8>(m_uiInstanceIDAndMask >> 24U); }
  void                   SetMask(xiiUInt8 uiMask);

  [[nodiscard]] xiiUInt32 GetHitGroupContribution() const { return m_uiHitGroupContributionAndFlags & 0x00FFFFFFU; }
  void                    SetHitGroupContribution(xiiUInt32 uiContribution);

  [[nodiscard]] xiiUInt64 GetBottomLevelASDeviceAddress() const { return m_uiBottomLevelASDeviceAddress; }
  void                    SetBottomLevelASDeviceAddress(xiiUInt64 uiDeviceAddress) { m_uiBottomLevelASDeviceAddress = uiDeviceAddress; }
  void                    SetBottomLevelAS(const xiiGALBottomLevelAS* pBottomLevelAS);

  [[nodiscard]] xiiBitflags<xiiGALRayTracingInstanceFlags> GetFlags() const
  {
    xiiBitflags<xiiGALRayTracingInstanceFlags> flags;
    flags.SetValue(static_cast<xiiUInt8>(m_uiHitGroupContributionAndFlags >> 24U));
    return flags;
  }
  void                   SetFlags(xiiBitflags<xiiGALRayTracingInstanceFlags> flags);
  [[nodiscard]] xiiUInt8 GetFlagsValue() const { return GetFlags().GetValue(); }
  void                   SetFlagsValue(xiiUInt8 uiFlags)
  {
    xiiBitflags<xiiGALRayTracingInstanceFlags> flags;
    flags.SetValue(uiFlags);
    SetFlags(flags);
  }

  xiiVec4   m_TransformRow0                  = xiiVec4(1.0f, 0.0f, 0.0f, 0.0f);
  xiiVec4   m_TransformRow1                  = xiiVec4(0.0f, 1.0f, 0.0f, 0.0f);
  xiiVec4   m_TransformRow2                  = xiiVec4(0.0f, 0.0f, 1.0f, 0.0f);
  xiiUInt32 m_uiInstanceIDAndMask            = 0xFF000000U;
  xiiUInt32 m_uiHitGroupContributionAndFlags = 0U;
  xiiUInt64 m_uiBottomLevelASDeviceAddress   = 0U;
};

static_assert(sizeof(xiiGALTLASInstanceData) == xiiGALTLASInstanceData::s_uiEncodedSize, "TLAS instance source records must retain their stable encoded size.");

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTLASInstanceData);

/// This describes the top level acceleration state that was used in the last build.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTopLevelASBuildDescription : public xiiHashableStruct<xiiGALTopLevelASBuildDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiInstanceCount = 0U; ///< The number of instances used by the most recent build or copied from the source TLAS.
};

/// This describes the top level acceleration structure creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTopLevelASCreationDescription : public xiiHashableStruct<xiiGALTopLevelASCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                                 m_uiMaxInstanceCount = 0U;                                 ///< The allocated size for the specified number of instances. The default is 0.
  xiiBitflags<xiiGALRayTracingBuildASFlags> m_Flags              = xiiGALRayTracingBuildASFlags::None; ///< The ray tracing build flags. The default is None.
  xiiUInt64                                 m_uiCompactedSize    = 0U;                                 ///< The size returned when writing the TLAS compacted size, if the acceleration structure is going to be the target of a compacted copy command. The default is 0.
  xiiUInt64                                 m_uiCommandQueueMask = XII_BIT(0);                         ///< Defines which command queues are allowed to execute commands that use this top level acceleration structure. The default is the main command queue.
                                                                                                       ///< Only specify the bits that indicate those command queues where the resource will be used, setting unnecessary bits will result in extra overhead.
};

/// Interface that defines methods to manipulate a top level acceleration structure (TLAS) object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALTopLevelAS : public xiiGALResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTopLevelAS, xiiGALResource);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALTopLevelASCreationDescription& GetDescription() const { return m_Description; };

  /// Returns whether a build or copy command has initialized this acceleration structure.
  [[nodiscard]] XII_ALWAYS_INLINE bool IsBuilt() const { return m_bBuilt; }

  /// Returns the instance count used by the most recent build or copied from the source TLAS.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetLastBuildInstanceCount() const { return m_BuildDescription.m_uiInstanceCount; }

  /// This returns the top level acceleration structure state after the last build or update operation.
  ///
  /// \return The top level acceleration structure build description, see xiiGALTopLevelASBuildDescription.
  ///
  /// \note Access to the top level acceleration structure must be externally synchronized.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALTopLevelASBuildDescription GetBuildDescription() const { return m_BuildDescription; }

  /// This returns the scratch buffer information for the current acceleration structure.
  ///
  /// \return The scratch buffer size description, see xiiGALScratchBufferSizeDescription.
  [[nodiscard]] virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const = 0;

protected:
  friend class xiiGALCommandList;
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALTopLevelAS(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelAS();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALTopLevelASCreationDescription m_Description;
  xiiGALTopLevelASBuildDescription    m_BuildDescription;
  bool                                m_bBuilt = false;
};
