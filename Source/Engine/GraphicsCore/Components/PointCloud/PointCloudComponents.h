#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <Foundation/Math/Color.h>

struct xiiMsgExtractRenderData;

// ============================================================
//  Point Cloud Component
// ============================================================

struct XII_GRAPHICSCORE_DLL xiiPointCloudPoint
{
  xiiVec3   m_vPosition;
  xiiUInt32 m_uiColorABGR;
  float     m_fIntensity;
  xiiUInt32 m_uiClassification;
  xiiUInt32 m_uiPad;
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiPointCloudPoint) == 24);

class XII_GRAPHICSCORE_DLL xiiPointCloudRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPointCloudRenderData, xiiRenderData);
public:
  xiiGALBufferHandle m_hPointBuffer;
  xiiUInt32          m_uiPointCount  = 0;
  float              m_fSplatSize    = 2.0f;   ///< Screen-space splat size in pixels.
  bool               m_bColorByClass = false;
  bool               m_bColorByIntensity = false;
};

using xiiPointCloudComponentManager = xiiComponentManager<class xiiPointCloudComponent, xiiBlockStorageType::Compact>;

/// \brief Renders up to 100M points as perspective-correct screen-space splats.
///
/// The GPU point buffer holds xiiPointCloudPoint records.  Rendering is done via
/// a mesh shader (one meshlet per point → screen quad) or a geometry shader fallback.
/// An octree-based LOD system reduces the visible point count based on camera distance.
class XII_GRAPHICSCORE_DLL xiiPointCloudComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPointCloudComponent, xiiRenderComponent, xiiPointCloudComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiPointCloudComponent(); ~xiiPointCloudComponent();

  /// \brief Set a pre-populated GPU buffer of xiiPointCloudPoint records.
  void SetPointBuffer(xiiGALBufferHandle hBuf, xiiUInt32 uiCount, const xiiBoundingBoxSphere& bounds);

  /// \brief CPU-write path: returns a mapped pointer for uiCount points.
  xiiPointCloudPoint* BeginWritePoints(xiiUInt32 uiCount);
  void EndWritePoints(const xiiBoundingBoxSphere& bounds);

  void SetSplatSize(float f);        float GetSplatSize()          const { return m_fSplatSize; }
  void SetColorByClass(bool b);      bool  GetColorByClass()       const { return m_bColorByClass; }
  void SetColorByIntensity(bool b);  bool  GetColorByIntensity()   const { return m_bColorByIntensity; }

  xiiUInt32 GetPointCount() const { return m_uiPointCount; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  void EnsureBufferCapacity(xiiUInt32 uiRequired);

  xiiGALBufferHandle   m_hPointBuffer;
  xiiBoundingBoxSphere m_Bounds          = xiiBoundingBoxSphere::MakeZero();
  xiiUInt32            m_uiPointCount    = 0;
  xiiUInt32            m_uiBufferCapacity= 0;
  float                m_fSplatSize      = 2.0f;
  bool                 m_bColorByClass      = false;
  bool                 m_bColorByIntensity  = false;
};

// ============================================================
//  Gaussian Splatting Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiGaussianSplatRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGaussianSplatRenderData, xiiRenderData);
public:
  xiiGALBufferHandle m_hSplatBuffer;   ///< Per-splat: position, covariance, SH coefficients, opacity.
  xiiGALBufferHandle m_hSortedKeys;    ///< Depth-sorted index buffer (updated by compute each frame).
  xiiUInt32          m_uiSplatCount   = 0;
  xiiUInt8           m_uiSHDegree     = 3;  ///< 0=ambient, 1=4 coeffs, 2=9, 3=16 per channel.
};

using xiiGaussianSplattingComponentManager = xiiComponentManager<class xiiGaussianSplattingComponent, xiiBlockStorageType::Compact>;

/// \brief 3D Gaussian Splatting renderer for photorealistic scenes from sensor capture.
///
/// Each splat is an anisotropic 3D Gaussian with:
///   - Position, scale (diagonal of Σ), rotation quaternion.
///   - Spherical harmonic colour coefficients (up to degree 3 = 16 coefficients per channel).
///   - Opacity (sigmoid-activated density).
///
/// Per-frame depth sorting is done by a GPU radix sort pass.
/// Rendering is forward alpha-blended front-to-back using a specialized splat shader.
class XII_GRAPHICSCORE_DLL xiiGaussianSplattingComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiGaussianSplattingComponent, xiiRenderComponent, xiiGaussianSplattingComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiGaussianSplattingComponent(); ~xiiGaussianSplattingComponent();

  void SetSplatFile(xiiStringView s);  // [ property ] — .splat binary asset
  xiiStringView GetSplatFile() const;
  void SetSHDegree(xiiUInt8 n);        xiiUInt8 GetSHDegree() const { return m_uiSHDegree; }

  xiiUInt32 GetSplatCount() const { return m_uiSplatCount; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;

  xiiGALBufferHandle m_hSplatBuffer;
  xiiGALBufferHandle m_hSortedKeys;
  xiiString          m_sSplatFile;
  xiiBoundingBoxSphere m_Bounds   = xiiBoundingBoxSphere::MakeZero();
  xiiUInt32          m_uiSplatCount = 0;
  xiiUInt8           m_uiSHDegree   = 3;
};

// ============================================================
//  LiDAR Point Buffer Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiLiDARPointBufferRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLiDARPointBufferRenderData, xiiRenderData);
public:
  xiiGALBufferHandle m_hCurrentBuffer;  ///< Latest LiDAR return ring-buffer.
  xiiGALBufferHandle m_hPreviousBuffer; ///< Previous frame buffer for temporal accumulation.
  xiiUInt32          m_uiReturnCount  = 0;
  float              m_fMaxRange      = 100.0f;
  float              m_fSplatSize     = 1.5f;
  bool               m_bColorByIntensity = true;
};

using xiiLiDARPointBufferComponentManager = xiiComponentManager<class xiiLiDARPointBufferComponent, xiiBlockStorageType::Compact>;

/// \brief Live-streaming LiDAR return renderer for sensor simulation at up to 1 MHz.
///
/// Double-buffered: the sensor system writes to the back buffer each tick while
/// the renderer reads from the front buffer.  Swap is atomic at frame start.
class XII_GRAPHICSCORE_DLL xiiLiDARPointBufferComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLiDARPointBufferComponent, xiiRenderComponent, xiiLiDARPointBufferComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiLiDARPointBufferComponent(); ~xiiLiDARPointBufferComponent();

  /// \brief Swaps front/back buffers and sets the live return count. Call once per sensor tick.
  void SwapBuffers(xiiUInt32 uiReturnCount);

  /// \brief Returns the back (write) buffer pointer for the sensor to fill.
  xiiGALBufferHandle GetWriteBuffer() const { return m_hBuffers[m_uiWriteIdx]; }

  void SetMaxRange(float f);          float GetMaxRange()         const { return m_fMaxRange; }
  void SetSplatSize(float f);         float GetSplatSize()        const { return m_fSplatSize; }
  void SetMaxCapacity(xiiUInt32 n);   xiiUInt32 GetMaxCapacity()  const { return m_uiCapacity; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  void AllocateBuffers();

  xiiGALBufferHandle m_hBuffers[2];     ///< Double buffer: [0] = A, [1] = B.
  xiiUInt32          m_uiReadIdx    = 0;
  xiiUInt32          m_uiWriteIdx   = 1;
  xiiUInt32          m_uiReturnCount= 0;
  xiiUInt32          m_uiCapacity   = 1000000;
  float              m_fMaxRange    = 100.0f;
  float              m_fSplatSize   = 1.5f;
};
