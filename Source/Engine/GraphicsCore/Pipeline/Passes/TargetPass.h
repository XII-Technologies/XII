#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>

/// \brief A render pipeline pass that presents rendered output to a target such as a swap chain.
///
/// The xiiTargetPass class is responsible for binding colour and depth attachments, acquiring the necessary GPU resources from the pipeline, and executing the final presentation step in the rendering pipeline.
///
/// It extends xiiPresentPipelinePass to support multiple colour attachments and an optional depth/stencil target.
class XII_GRAPHICSCORE_DLL xiiTargetPass : public xiiPresentPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTargetPass, xiiPresentPipelinePass);

public:
  /// \brief Constructs a new target pass.
  ///
  /// \param sName Optional name for this pass (defaults to "TargetPass").
  xiiTargetPass(xiiStringView sName = "TargetPass");

  /// Destructor.
  ~xiiTargetPass();

  /// \brief Provides descriptions of the resources this pass will produce.
  ///
  /// This includes any colour and depth attachments used as outputs by the pass.
  ///
  /// \param view     - The view for which resources are being described.
  /// \param pInputs  - Array of pointers to input resources.
  /// \param pOutputs - Array where output resource descriptions will be written.
  ///
  /// \return Success or failure result.
  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  /// \brief Queries the provider for a resource connected to a specific pin.
  ///
  /// This can be used to dynamically supply GPU resources (such as textures or buffers) based on the pipeline's runtime requests.
  ///
  /// \param pPin    - The node pin requesting the resource.
  /// \param request - Details about the requested resource.
  ///
  /// \return Shared pointer to the provided GPU resource object, or nullptr if unavailable.
  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;

  /// \brief Executes this pass for the given view context.
  ///
  /// Binds render targets, applies any final operations, and presents the result to the target (such as the screen or an off-screen buffer).
  ///
  /// \param renderViewContext - The current view context for rendering.
  /// \param pInputs           - Array of pointers to input pass connections.
  /// \param pOutputs          - Array of pointers to output pass connections.
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  /// @name Colour Attachment Pins
  ///@{
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour0; ///< First colour attachment input.
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour1; ///< Second colour attachment input.
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour2; ///< Third colour attachment input.
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour3; ///< Fourth colour attachment input.
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour4; ///< Fifth colour attachment input.
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour5; ///< Sixth colour attachment input.
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour6; ///< Seventh colour attachment input.
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour7; ///< Eighth colour attachment input.
  ///@}

  /// \brief Depth/Stencil attachment input pin.
  xiiRenderPipelineNodeInputDepthAttachmentProviderPin m_PinDepthStencil;

  /// \brief Pointer to the swap chain associated with this pass (if any).
  xiiGALSwapChain* m_pSwapChain = nullptr;

  /// \brief Cached render targets used during execution.
  xiiRenderTargets m_RenderTargets;
};
