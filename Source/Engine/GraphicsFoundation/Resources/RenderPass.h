#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Size.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Resource.h>

/// \brief A special constant used to indicate that the render pass is unused.
#define XII_GAL_ATTACHMENT_UNUSED 0xFFFFFFFFU

/// \brief A special sub pass index value expanding synchronization scope outside a sub pass.
#define XII_GAL_SUBPASS_EXTERNAL 0xFFFFFFFFU

/// \brief This describes the render pass load operation.
///
/// Vulkan counterpart: [VkAttachmentLoadOp](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkAttachmentLoadOp).
/// D3D12 counterpart: [D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE](https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_render_pass_beginning_access_type).
struct XII_GRAPHICSFOUNDATION_DLL xiiGALAttachmentLoadOperation
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Load = 0U, ///< The previous contents of the texture within the render area will be preserved.
    Clear,     ///< The contents within the render area will be cleared to a uniform value, which is specified when a render pass instance is begun.
    Discard,   ///< The previous contents within the area need not be preserved; the contents of the attachment will be undefined inside the render area.

    ENUM_COUNT,

    Default = Load
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALAttachmentLoadOperation);

/// \brief This describes the render pass store operation.
///
/// Vulkan counterpart: [VkAttachmentStoreOp](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkAttachmentStoreOp).
/// D3D12 counterpart: [D3D12_RENDER_PASS_ENDING_ACCESS_TYPE](https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_render_pass_ending_access_type).
struct XII_GRAPHICSFOUNDATION_DLL xiiGALAttachmentStoreOperation
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Store = 0U, ///< The contents generated during the render pass and within the render area are written to memory.
    Discard,    ///< The contents within the render area are not needed after rendering, and may be discarded; the contents of the attachment will be undefined inside the render area.

    ENUM_COUNT,

    Default = Store
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALAttachmentStoreOperation);

/// \brief This describes the render pass attachment creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRenderPassAttachmentDescription : public xiiHashableStruct<xiiGALRenderPassAttachmentDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALTextureFormat>            m_Format                = xiiGALTextureFormat::Unknown;          ///< The format of the texture view that will be used for the attachment. The default is xiiGALTextureFormat::Unknown.
  xiiUInt8                                m_uiSampleCount         = 0U;                                    ///< The number of samples in the texture. The default is 0.
  xiiEnum<xiiGALAttachmentLoadOperation>  m_LoadOperation         = xiiGALAttachmentLoadOperation::Load;   ///< The load operation that specifies how the contents of color and depth components of the attachment are treated at the beginning of the sub pass where it is first used. The default is xiiGALAttachmentLoadOperation::Load.
  xiiEnum<xiiGALAttachmentStoreOperation> m_StoreOperation        = xiiGALAttachmentStoreOperation::Store; ///< The store operation how the contents of color and depth components of the attachment are treated at the end of the sub pass where it is last used. The default is xiiGALAttachmentStoreOperation::Store.
  xiiEnum<xiiGALAttachmentLoadOperation>  m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Load;   ///< The load operation that specifies how the contents of the stencil component of the attachment is treated at the beginning of the sub pass where it is first used. This value is ignored when the format does not have stencil component. The default is xiiGALAttachmentLoadOperation::Load.
  xiiEnum<xiiGALAttachmentStoreOperation> m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store; ///< The store operation how the contents of the stencil component of the attachment is treated at the end of the sub pass where it is last used. This value is ignored when the format does not have stencil component. The default is xiiGALAttachmentStoreOperation::Store.
  xiiBitflags<xiiGALResourceStateFlags>   m_InitialStateFlags     = xiiGALResourceStateFlags::Unknown;     ///< The state the attachment texture sub-resource will be in when a render pass instance begins. The default is xiiGALResourceStateFlags::Unknown.
  xiiBitflags<xiiGALResourceStateFlags>   m_FinalStateFlags       = xiiGALResourceStateFlags::Unknown;     ///< The state the attachment texture sub-resource will be transitioned to when a render pass instance ends. The default is xiiGALResourceStateFlags::Unknown.
};

/// \brief This describes the render pass attachment reference.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALAttachmentReferenceDescription : public xiiHashableStruct<xiiGALAttachmentReferenceDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                             m_uiAttachmentIndex  = 0U;                                ///< Either an integer value identifying an attachment at the corresponding index in the xiiGALRenderPassCreationDescription, or XII_GAL_ATTACHMENT_UNUSED to signify that this attachment is not used. The default is 0.
  xiiBitflags<xiiGALResourceStateFlags> m_ResourceStateFlags = xiiGALResourceStateFlags::Unknown; ///< The state of the attachment during the sub pass.
};

/// \brief This describes the shading rate attachment.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateAttachmentDescription : public xiiHashableStruct<xiiGALShadingRateAttachmentDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiGALAttachmentReferenceDescription m_AttachmentReference;           ///< The shading rate attachment reference.
  xiiSizeU32                           m_TileSize = xiiSizeU32(0U, 0U); ///< Each texel in the attachment contains shading rate for the whole tile. The size must be a power-of-two value between xiiGALShadingRateProperties::MinTileSize and xiiGALShadingRateProperties::MaxTileSize. Keep zero (default) to use the default tile size.
};

/// \brief This describes the render pass sub pass.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSubPassDescription : public xiiHashableStruct<xiiGALSubPassDescription>
{
  XII_DECLARE_POD_TYPE();

  const xiiGALAttachmentReferenceDescription*   m_pInputAttachments             = nullptr; ///< The pointer to the array of input attachments.
  xiiUInt32                                     m_uiInputAttachmentCount        = 0U;      ///< The number of input attachments the sub pass uses.
  const xiiGALAttachmentReferenceDescription*   m_pRenderTargetAttachments      = nullptr; ///< The pointer to the array of color render target attachments. Each element of the m_pRenderTargetAttachments array corresponds to an output in the pixel shader, i.e. if the shader declares an output variable decorated with a render target index X, then it uses the attachment provided in m_pRenderTargetAttachments[X]. If the attachment index is XII_GAL_ATTACHMENT_UNUSED, writes to this render target are ignored.
  xiiUInt32                                     m_uiRenderTargetAttachmentCount = 0U;      ///< The number of color render target attachments.
  const xiiGALAttachmentReferenceDescription*   m_pResolveAttachments           = nullptr; ///< The pointer to the array of resolve attachments. If m_pResolveAttachments is not nullptr, each of its elements corresponds to a render target attachment (the element in m_pRenderTargetAttachments at the same index), and a multisample resolve operation is defined for each attachment. At the end of each sub pass, multisample resolve operations read the sub pass's color attachments, and resolve the samples for each pixel within the render area to the same pixel location in the corresponding resolve attachments, unless the resolve attachment index is XII_GAL_ATTACHMENT_UNUSED.
  const xiiGALAttachmentReferenceDescription*   m_pDepthStencilAttachments      = nullptr; ///< The pointer to the depth-stencil attachment.
  const xiiUInt32*                              m_pPreserveAttachments          = nullptr; ///< The pointer to the array of preserve attachments.
  xiiUInt32                                     m_uiPreserveAttachmentCount     = 0U;      ///< The number of preserve attachments.
  const xiiGALShadingRateAttachmentDescription* m_pShadingRateAttachment        = nullptr; ///< The pointer to the shading rate attachment.
};

/// \brief This describes the sub pass dependency.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSubPassDependencyDescription : public xiiHashableStruct<xiiGALSubPassDependencyDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiSourceSubPass      = 0U; ///< The sub pass index of the first sub pass in the dependency, or XII_GAL_SUBPASS_EXTERNAL.
  xiiUInt32 m_uiDestinationSubPass = 0U; ///< The sub pass index of the second sub pass in the dependency, or XII_GAL_SUBPASS_EXTERNAL.
};

/// \brief This describes the render pass creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRenderPassCreationDescription : public xiiHashableStruct<xiiGALRenderPassCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                                m_sName;                       ///< Resource name. The default is an empty string view.
  const xiiGALRenderPassAttachmentDescription* m_pAttachments      = nullptr; ///< The pointer to the array of sub pass attachments.
  xiiUInt32                                    m_uiAttachmentCount = 0U;      ///< The number of attachments used by the render pass.
  const xiiGALSubPassDescription*              m_pSubPasses        = nullptr; ///< The pointer to the array of sub pass descriptions.
  xiiUInt32                                    m_uiSubPassCount    = 0U;      ///< The number of sub passes in the render pass.
  const xiiGALSubPassDependencyDescription*    m_pDependencies     = nullptr; ///< The pointer to the array of sub pass dependencies.
  xiiUInt32                                    m_uiDependencyCount = 0U;      ///< The number of memory dependencies between pairs of sub passes.
};

/// \brief Interface that defines methods to manipulate a render pass object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALRenderPass : public xiiGALResource<xiiGALRenderPassCreationDescription>
{
public:
protected:
  friend class xiiGALDevice;

  xiiGALRenderPass(const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPass();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsFoundation/Resources/Implementation/RenderPass_inl.h>
