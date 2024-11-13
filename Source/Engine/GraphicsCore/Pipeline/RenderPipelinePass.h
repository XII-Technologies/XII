#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/RenderPipelineNode.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>
#include <GraphicsFoundation/Resources/Texture.h>

struct xiiGALTextureCreationDescription;
class xiiStreamWriter;

/// \brief Passed to xiiRenderPipelinePass::InitRenderPipelinePass to inform about existing connections on each input / output pin index.
struct xiiRenderPipelinePassConnection
{
  xiiRenderPipelinePassConnection() :
    m_pOutput(nullptr)
  {
  }

  xiiGALTextureCreationDescription                   m_TextureDescription;
  xiiGALTextureHandle                                m_TextureHandle;
  const xiiRenderPipelineNodePin*                    m_pOutput; ///< The output pin that this connection spawns from.
  xiiHybridArray<const xiiRenderPipelineNodePin*, 4> m_Inputs;  ///< The various input pins this connection is connected to.
};

class XII_GRAPHICSCORE_DLL xiiRenderPipelinePass : public xiiRenderPipelineNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePass, xiiRenderPipelineNode);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipelinePass);

public:
  xiiRenderPipelinePass(xiiStringView sName, bool bIsStereoAware = false);
  ~xiiRenderPipelinePass();

  /// \brief Sets the name of the pass.
  void SetName(xiiStringView sName);

  /// \brief returns the name of the pass.
  xiiStringView GetName() const;

  /// \brief True if the render pipeline pass can handle stereo cameras correctly.
  bool IsStereoAware() const { return m_bIsStereoAware; }

  /// \brief For a given input pin configuration, provide the output configuration of this node.
  /// Outputs is already resized to the number of output pins.
  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) = 0;

  /// Returns the current texture this node provides at the given *ProviderPin.
  /// This function is called every frame if this node holds a xiiRenderPipelineNodeInputProviderPin or xiiRenderPipelineNodeOutputProviderPin pin. The node can return a valid texture handle, or an invalid handle, in which case the missing texture will be created from the texture pool.
  /// \param pPin - The member pin for which the texture is requested.
  /// \param desc - The format of the texture that should be provided.
  /// \return The texture view to use for this pin's connections. Or invalid, in which case it reverts to a regular input / output pin.
  virtual xiiGALTextureViewHandle QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc) { return {}; }

  /// \brief After GetRenderTargetDescriptions was called successfully for each pass, this function is called
  /// with the inputs and outputs for review. Disconnected pins have a nullptr value in the passed in arrays.
  /// This is the time to create additional resources that are not covered by the pins automatically, e.g. a picking texture or eye
  /// adaptation buffer.
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs);

  /// \brief Render into outputs. Both inputs and outputs are passed in with actual texture handles.
  /// Disconnected pins have a nullptr value in the passed in arrays. You can now create views and render target setups on the fly and
  /// fill the output targets with data.
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) = 0;

  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs);

  /// \brief Allows for the pass to write data back using xiiView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(xiiView* pView);

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream);

  void RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter = xiiRenderDataBatch::Filter());

  XII_ALWAYS_INLINE xiiRenderPipeline* GetPipeline() { return m_pPipeline; }
  XII_ALWAYS_INLINE const xiiRenderPipeline* GetPipeline() const { return m_pPipeline; }

private:
  friend class xiiRenderPipeline;

  bool m_bActive = true;

  const bool      m_bIsStereoAware;
  xiiHashedString m_sName;

  xiiRenderPipeline* m_pPipeline = nullptr;
};
