#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data submitted per-frame by a stereo camera component.
class XII_GRAPHICSCORE_DLL xiiStereoCameraRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStereoCameraRenderData, xiiRenderData);

public:
  xiiMat4 m_mLeftEyeViewProj  = xiiMat4::MakeIdentity();
  xiiMat4 m_mRightEyeViewProj = xiiMat4::MakeIdentity();
  float   m_fEyeSeparation    = 0.064f; ///< Interpupillary distance in metres.
  float   m_fConvergenceDist  = 2.0f;   ///< Convergence plane distance in metres.
};

using xiiStereoCameraComponentManager = xiiComponentManager<class xiiStereoCameraComponent, xiiBlockStorageType::Compact>;

/// \brief Drives a stereo/VR camera with separate per-eye view-projection matrices.
class XII_GRAPHICSCORE_DLL xiiStereoCameraComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiStereoCameraComponent, xiiComponent, xiiStereoCameraComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  xiiStereoCameraComponent();
  ~xiiStereoCameraComponent();

  void  SetEyeSeparation(float f);                            // [ property ]
  float GetEyeSeparation() const { return m_fEyeSeparation; } // [ property ]

  void  SetConvergenceDist(float f);                              // [ property ]
  float GetConvergenceDist() const { return m_fConvergenceDist; } // [ property ]

  /// \brief Called by the VR runtime to push updated per-eye matrices.
  void SetEyeMatrices(const xiiMat4& mLeftViewProj, const xiiMat4& mRightViewProj);

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  float   m_fEyeSeparation   = 0.064f;
  float   m_fConvergenceDist = 2.0f;
  xiiMat4 m_mLeftViewProj    = xiiMat4::MakeIdentity();
  xiiMat4 m_mRightViewProj   = xiiMat4::MakeIdentity();
};
