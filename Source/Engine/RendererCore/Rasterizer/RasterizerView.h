#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ArrayPtr.h>
#include <RendererCore/RendererCoreDLL.h>

class Rasterizer;
class xiiRasterizerObject;
class xiiColorLinearUB;
class xiiCamera;
class xiiSimdBBox;

class XII_RENDERERCORE_DLL xiiRasterizerView final
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRasterizerView);

public:
  xiiRasterizerView();
  ~xiiRasterizerView();

  /// \brief Changes the resolution of the view. Has to be called at least once before starting to render anything.
  void SetResolution(xiiUInt32 uiWidth, xiiUInt32 uiHeight, float fAspectRatio);

  xiiUInt32 GetResolutionX() const { return m_uiResolutionX; }
  xiiUInt32 GetResolutionY() const { return m_uiResolutionY; }

  /// \brief Prepares the view to rasterize a new scene.
  void BeginScene();

  /// \brief Finishes rasterizing the scene. Visibility queries only work after this.
  void EndScene();

  /// \brief Writes an RGBA8 representation of the depth values to targetBuffer.
  ///
  /// The buffer must be large enough for the chosen resolution.
  void ReadBackFrame(xiiArrayPtr<xiiColorLinearUB> targetBuffer) const;

  /// \brief Sets the camera from which to extract the rendering position, direction and field-of-view.
  void SetCamera(const xiiCamera* pCamera)
  {
    m_pCamera = pCamera;
  }

  /// \brief Adds an object as an occluder to the scene. Once all occluders have been rasterized, visibility queries can be done.
  void AddObject(const xiiRasterizerObject* pObject, const xiiTransform& transform)
  {
    auto& inst       = m_Instances.ExpandAndGetRef();
    inst.m_pObject   = pObject;
    inst.m_Transform = transform;
  }

  /// \brief Checks whether a box would be visible, or is fully occluded by the existing scene geometry.
  ///
  /// Note: This only works after EndScene().
  bool IsVisible(const xiiSimdBBox& aabb) const;

  /// \brief Wether any occluder was actually added and also rasterized. If not, no need to do any visibility checks.
  bool HasRasterizedAnyOccluders() const
  {
    return m_bAnyOccludersRasterized;
  }

private:
  void SortObjectsFrontToBack();
  void RasterizeObjects(xiiUInt32 uiMaxObjects);
  void UpdateViewProjectionMatrix();
  void ApplyModelViewProjectionMatrix(const xiiTransform& modelTransform);

  bool                     m_bAnyOccludersRasterized = false;
  const xiiCamera*         m_pCamera                 = nullptr;
  xiiUInt32                m_uiResolutionX           = 0;
  xiiUInt32                m_uiResolutionY           = 0;
  float                    m_fAspectRation           = 1.0f;
  xiiUniquePtr<Rasterizer> m_pRasterizer;

  struct Instance
  {
    xiiTransform               m_Transform;
    const xiiRasterizerObject* m_pObject;
  };

  xiiDeque<Instance> m_Instances;
  xiiMat4            m_mViewProjection;
};

class xiiRasterizerViewPool
{
public:
  xiiRasterizerView* GetRasterizerView(xiiUInt32 uiWidth, xiiUInt32 uiHeight, float fAspectRatio);
  void               ReturnRasterizerView(xiiRasterizerView* pView);

private:
  struct PoolEntry
  {
    bool              m_bInUse = false;
    xiiRasterizerView m_RasterizerView;
  };

  xiiMutex            m_Mutex;
  xiiDeque<PoolEntry> m_Entries;
};
