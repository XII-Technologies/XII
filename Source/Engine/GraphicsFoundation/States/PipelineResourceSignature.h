/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Shader/ShaderByteCode.h>

/// Fixed API capacity used for shader-declared unbounded descriptor arrays. Backends expose the
/// array as a sparse, partially-bound table while shaders retain runtime indexing semantics.
static constexpr xiiUInt32 XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY = 4096U;

/// This describes the pipeline resource property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineResourceFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None             = 0U,               ///< The resource has no special properties.
    NoDynamicBuffers = XII_BIT(0),       ///< This indicates that dynamic buffers will never be bound to the resource variable. This applies to xiiGALShaderResourceType::ConstantBuffer, xiiGALShaderResourceType::BufferUAV, and xiiGALShaderResourceType::BufferSRV resources.
                                         ///
                                         /// \remarks In Vulkan and Direct3D12 graphics implementations, dynamic buffers require extra work at run time. If an application knows it will never bind a dynamic buffer to the variable, it should use xiiGALPipelineResourceFlags::NoDynamicBuffers flag to improve performance.
                                         ///          This flag is not required and non-dynamic buffers will still work even if the flag is not used. It is an error to bind a dynamic buffer to resource that uses xiiGALPipelineResourceFlags::NoDynamicBuffers flag.
    CombinedSampler = XII_BIT(1),        ///< This indicates that a texture SRV will be combined with a sampler. It applies to xiiGALShaderResourceType::TextureSRV resources.
    Formattedbuffer = XII_BIT(2),        ///< This indicates this variable will be used to bind formatted buffers. It applies to xiiGALShaderResourceType::BufferUAV and xiiGALShaderResourceType::BufferSRV resources.
                                         ///
                                         /// \remarks In the Vulkan graphics implementation, formatted buffers require another descriptor type as opposed to structured buffers. If an application will be using formatted buffers with buffer UAVs and SRVs, it must specify the xiiGALPipelineResourceFlags::FormattedBuffer flag.
    RuntimeArray           = XII_BIT(3), ///< This indicates that the resource is a run-time sized array (e.g., an array without a specific size).
    GeneralInputAttachment = XII_BIT(4), ///< This indicates that the resource is an input attachment in general layout, which allows simultaneously reading from the resource through the input attachment and writing to it via color or depth-stencil attachment.
                                         ///
                                         /// \note This flag is only valid in the Vulkan graphics implementation.

    Default = None
  };

  struct Bits
  {
    StorageType NoDynamicBuffers : 1;
    StorageType CombinedSampler : 1;
    StorageType Formattedbuffer : 1;
    StorageType RuntimeArray : 1;
    StorageType GeneralInputAttachment : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALPipelineResourceFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPipelineResourceFlags);

/// This describes an immutable sampler used by a graphics pipeline.
///
/// An immutable sampler is compiled into the pipeline state and cannot be modified. It is generally more efficient than a regular sampler and should be used whenever possible.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALImmutableSamplerDescription : public xiiHashableStruct<xiiGALImmutableSamplerDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiHashedString                  m_SamplerOrTextureName;                     ///< The name of the sampler itself or the name of the texture variable that this immutable sampler is assigned to if combined texture samplers are used.
  xiiBitflags<xiiGALShaderType>    m_ShaderStages = xiiGALShaderType::Unknown; ///< The shader stages that this immutable sampler applies to. More than one shader stage can be specified. The default is xiiGALShaderType::Unknown.
  xiiGALSamplerCreationDescription m_SamplerDescription;                       ///< The sampler creation description. See xiiGALSamplerCreationDescription for details.
};

/// This describes the pipeline resource information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineResourceDescription : public xiiHashableStruct<xiiGALPipelineResourceDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiHashedString               m_sName;                                                                ///< The resource name in the shader.
  xiiBitflags<xiiGALShaderType> m_ShaderStages = xiiGALShaderType::Unknown;                             ///< The shader stages that this resource applies to. When multiple shader stages are specified, all stages will share the same resource. The default is xiiGALShaderType::Unknown.
                                                                                                        ///
                                                                                                        ///  \remarks There may be multiple resources with the same name in different shader stages, but the stages specified for different resources with the same name must not overlap.
  xiiUInt32                                m_uiArraySize           = 0U;                                ///< The resource array size (must be set to 1 for non-array resources). The default is 0.
  xiiUInt32                                m_uiBindSet             = 0U;                                ///< The resource bind set, for descriptor sets.
  xiiUInt32                                m_uiBindSlot            = 0U;                                ///< The resource bind slot in the bind set.
  xiiEnum<xiiGALShaderResourceType>        m_ResourceType          = xiiGALShaderResourceType::Unknown; ///< The resource type, see xiiGALShaderResourceType. The default is xiiGALShaderResourceType::Unknown.
  xiiBitflags<xiiGALPipelineResourceFlags> m_PipelineResourceFlags = xiiGALPipelineResourceFlags::None; ///< Special resource flags, see xiiGALPipelineResourceFlags. The default is xiiGALPipelineResourceFlags::None.
};

/// Describes a push constant range to include in the pipeline layout for this signature.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPushConstantRange
{
  xiiUInt32                     m_uiOffset     = 0U;                        ///< Byte offset of the push constant range.
  xiiUInt32                     m_uiSize       = 0U;                        ///< Size in bytes of the push constant range.
  xiiBitflags<xiiGALShaderType> m_ShaderStages = xiiGALShaderType::Unknown; ///< Shader stages that can access this range.

  XII_ALWAYS_INLINE bool operator==(const xiiGALPushConstantRange& rhs) const = default;
};

/// This describes the pipeline resource signature creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineResourceSignatureCreationDescription
{
  xiiUInt32                                             m_uiBindingIndex = 0U; ///< The binding index that this resource signature uses. Every resource signature must be assign to one signature slot. The total number of slots is given by XII_GAL_MAX_RESOURCE_SIGNATURES_COUNT constant. All resource signatures used by a pipeline state must be assigned to different slots.
  xiiHybridArray<xiiGALPipelineResourceDescription, 2U> m_Resources;           ///< An array of resource descriptions, see xiiGALPipelineResourceDescription for details.
  xiiHybridArray<xiiGALImmutableSamplerDescription, 2U> m_ImmutableSamplers;   ///< An array of immutable samplers, see xiiGALImmutableSamplerDescription for details.
  xiiHybridArray<xiiGALPushConstantRange, 1U>           m_PushConstantRanges;  ///< Push constant ranges for the pipeline layout.

  XII_ALWAYS_INLINE bool operator==(const xiiGALPipelineResourceSignatureCreationDescription& rhs) const = default;
};

/// Interface that defines methods of the pipeline resource signature.
class XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineResourceSignature : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineResourceSignature, xiiGALDeviceObject);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALPipelineResourceSignatureCreationDescription& GetDescription() const { return m_Description; };

  /// Returns true if this pipeline resource signature is compatible with the given pipeline resource signature.
  [[nodiscard]] virtual bool IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALPipelineResourceSignature(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription);

  virtual ~xiiGALPipelineResourceSignature();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALPipelineResourceSignatureCreationDescription m_Description;
};
