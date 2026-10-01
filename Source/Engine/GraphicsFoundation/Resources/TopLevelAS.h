/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

/// This describes the hit group binding mode used by the top level acceleration structure.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALHitGroupBindingMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    PerGeometry = 0U,                 ///< Each geometry in every instance may use a unique hit shader group. In this mode, space is reserved for each geometry in every instance in the top level acceleration structure and uses the most memory.
    PerInstance,                      ///< Each instance may use a unique hit shader group. In this mode, one slot is reserved for each instance irrespective of how many geometries it contains, so it uses less memory.
    PerTopLevelAccelerationStructure, ///< All instances in each top level acceleration structure will use the same hit group. In this mode, a single slot is reserved for one hit group for each top level acceleration structure.
    UserDefined,                      ///< The user must specify the contribution to the hit group index, and only bind hit groups by index.

    ENUM_COUNT,

    Default = PerGeometry
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALHitGroupBindingMode);

/// Portable ray tracing instance flags. Vulkan and D3D12 intentionally assign
/// identical bit values to these four semantics.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingInstanceFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None                          = 0U,
    TriangleCullDisable           = XII_BIT(0),
    TriangleFrontCounterClockwise = XII_BIT(1),
    ForceOpaque                   = XII_BIT(2),
    ForceNonOpaque                = XII_BIT(3),

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

/// Backend-independent 64-byte TLAS instance record.
///
/// Its binary layout matches VkAccelerationStructureInstanceKHR and
/// D3D12_RAYTRACING_INSTANCE_DESC: row-major float3x4 transform, two packed
/// 24:8 words, and the BLAS device address. This lets GraphicsCore stream one
/// instance buffer on both backends without including native API headers.
struct XII_GRAPHICSFOUNDATION_DLL alignas(16) xiiGALTLASInstanceData
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] xiiMat4 GetTransform() const;
  void                  SetTransform(xiiMat4 transform);

  [[nodiscard]] xiiUInt32 GetInstanceID() const { return m_uiInstanceIDAndMask & 0x00FFFFFFU; }
  void                    SetInstanceID(xiiUInt32 uiInstanceID);

  [[nodiscard]] xiiUInt8 GetMask() const { return static_cast<xiiUInt8>(m_uiInstanceIDAndMask >> 24U); }
  void                   SetMask(xiiUInt8 uiMask);

  [[nodiscard]] xiiUInt32 GetHitGroupContribution() const { return m_uiHitGroupContributionAndFlags & 0x00FFFFFFU; }
  void                    SetHitGroupContribution(xiiUInt32 uiContribution);

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

static_assert(sizeof(xiiGALTLASInstanceData) == 64U, "TLAS instance records must match the native Vulkan and D3D12 ABI.");

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALTLASInstanceData);

/// This describes the top level acceleration state that was used in the last build.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTopLevelASBuildDescription : public xiiHashableStruct<xiiGALTopLevelASBuildDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                          m_uiInstanceCount                    = 0U;                                     ///< The number of instances. The default is 0.
  xiiUInt32                          m_uiHitGroupStride                   = 0U;                                     ///< The number of hit shader groups. The default is 0.
  xiiEnum<xiiGALHitGroupBindingMode> m_BindingMode                        = xiiGALHitGroupBindingMode::PerGeometry; ///< The hit group binding mode. The default is xiiGALHitGroupBindingMode::PerGeometry.
  xiiUInt32                          m_uiFirstContributionToHitGroupIndex = 0U;                                     ///< The first hit group location. The default is 0
  xiiUInt32                          m_uiLastContributionToHitGroupIndex  = 0U;                                     ///< The last hit group location. The default is 0.
};

/// This describes the top level acceleration structure instance.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTopLevelASInstanceDescription : public xiiHashableStruct<xiiGALTopLevelASInstanceDescription>
{
  xiiSharedPtr<xiiGALBottomLevelAS> m_pBottomLevelAS;                     ///< The reference-counted pointer to the bottom-level acceleration structure.
  xiiUInt32                         m_uiContributionToHitGroupIndex = 0U; ///< The index that corresponds to the one specified in the TLAS build instance contribution to the hit group index.
  xiiUInt32                         m_uiInstanceIndex               = 0U; ///< The autogenerated index of the instance.
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

  /// This returns the instance description that can be used in the shader binding table.
  ///
  /// \param sName - The instance name that is specified in the xiiGALTLASBuildInstanceData.
  ///
  /// \return The top level acceleration structure instance description, see xiiGALTopLevelASInstanceDescription. If the instance does not exist, then the contribution to hit group index and instance index are set to xiiInvalidIndex.
  ///
  /// \note Access to the top level acceleration structure must be externally synchronized.
  [[nodiscard]] virtual xiiGALTopLevelASInstanceDescription GetInstanceDescription(xiiStringView sName) const = 0;

  /// This returns the top level acceleration structure state after the last build or update operation.
  ///
  /// \return The top level acceleration structure build description, see xiiGALTopLevelASBuildDescription.
  ///
  /// \note Access to the top level acceleration structure must be externally synchronized.
  [[nodiscard]] virtual xiiGALTopLevelASBuildDescription GetBuildDescription() const = 0;

  /// This returns the scratch buffer information for the current acceleration structure.
  ///
  /// \return The scratch buffer size description, see xiiGALScratchBufferSizeDescription.
  [[nodiscard]] virtual xiiGALScratchBufferSizeDescription GetScratchBufferSizeDescription() const = 0;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALTopLevelAS(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTopLevelASCreationDescription& creationDescription);

  virtual ~xiiGALTopLevelAS();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALTopLevelASCreationDescription m_Description;
};
