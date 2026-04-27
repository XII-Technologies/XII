#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <Foundation/Math/Color.h>

struct xiiMsgExtractRenderData;

// ============================================================
//  Atom Sphere Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiAtomSphereRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAtomSphereRenderData, xiiRenderData);
public:
  xiiGALBufferHandle m_hAtomBuffer;     ///< GPU buffer of xiiMDAtom records.
  xiiUInt32          m_uiAtomCount = 0;
  float              m_fRadiusScale = 1.0f;
  bool               m_bColorByElement = true;
  bool               m_bColorByVelocity= false;
  bool               m_bUseImpostors   = true; ///< Sphere impostor quads (faster than geometry).
};

using xiiAtomSphereComponentManager = xiiComponentManager<class xiiAtomSphereComponent, xiiBlockStorageType::Compact>;

/// \brief Renders N atoms as GPU-instanced sphere impostors or icosphere geometry.
///
/// Atom positions are sourced from a GPU structured buffer updated each simulation frame.
/// Supports coloring by element (CPK), charge, kinetic energy, or velocity magnitude.
class XII_GRAPHICSCORE_DLL xiiAtomSphereComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAtomSphereComponent, xiiRenderComponent, xiiAtomSphereComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiAtomSphereComponent(); ~xiiAtomSphereComponent();

  void SetAtomBuffer(xiiGALBufferHandle hBuf, xiiUInt32 uiCount);
  void SetRadiusScale(float f);             float GetRadiusScale() const { return m_fRadiusScale; }
  void SetColorByElement(bool b);           bool  GetColorByElement()  const { return m_bColorByElement; }
  void SetColorByVelocity(bool b);          bool  GetColorByVelocity() const { return m_bColorByVelocity; }
  void SetUseImpostors(bool b);             bool  GetUseImpostors()    const { return m_bUseImpostors; }
  void SetCustomBounds(const xiiBoundingBoxSphere& b);

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  xiiGALBufferHandle   m_hAtomBuffer;
  xiiBoundingBoxSphere m_CustomBounds = xiiBoundingBoxSphere::MakeZero();
  xiiUInt32            m_uiAtomCount   = 0;
  float                m_fRadiusScale  = 1.0f;
  bool                 m_bColorByElement  = true;
  bool                 m_bColorByVelocity = false;
  bool                 m_bUseImpostors    = true;
  bool                 m_bHasCustomBounds = false;
};

// ============================================================
//  Bond Cylinder Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiBondCylinderRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBondCylinderRenderData, xiiRenderData);
public:
  xiiGALBufferHandle m_hBondBuffer;   ///< {atomA, atomB, order} per bond.
  xiiGALBufferHandle m_hAtomBuffer;   ///< Position source for bond endpoint lookup.
  xiiUInt32          m_uiBondCount = 0;
  float              m_fRadius = 0.1f;
};

using xiiBondCylinderComponentManager = xiiComponentManager<class xiiBondCylinderComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiBondCylinderComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiBondCylinderComponent, xiiRenderComponent, xiiBondCylinderComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiBondCylinderComponent(); ~xiiBondCylinderComponent();

  void SetBondBuffer(xiiGALBufferHandle hBonds, xiiGALBufferHandle hAtoms, xiiUInt32 uiCount);
  void SetRadius(float f);   float GetRadius() const { return m_fRadius; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  xiiGALBufferHandle m_hBondBuffer;
  xiiGALBufferHandle m_hAtomBuffer;
  xiiUInt32          m_uiBondCount = 0;
  float              m_fRadius     = 0.1f;
};

// ============================================================
//  Isosurface Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiIsosurfaceRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIsosurfaceRenderData, xiiRenderData);
public:
  xiiTexture3DResourceHandle m_hDensityField;
  xiiMaterialResourceHandle  m_hMaterial;
  float                      m_fIsovalue    = 0.5f;
  xiiUInt32                  m_uiGridRes[3] = {64, 64, 64};
};

using xiiIsosurfaceComponentManager = xiiComponentManager<class xiiIsosurfaceComponent, xiiBlockStorageType::Compact>;

/// \brief Marching-Cubes / Dual-Contouring GPU isosurface from a 3D density field texture.
class XII_GRAPHICSCORE_DLL xiiIsosurfaceComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiIsosurfaceComponent, xiiRenderComponent, xiiIsosurfaceComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiIsosurfaceComponent(); ~xiiIsosurfaceComponent();

  void          SetDensityFieldFile(xiiStringView s);  xiiStringView GetDensityFieldFile() const;
  void          SetMaterialFile(xiiStringView s);      xiiStringView GetMaterialFile()     const;
  void          SetIsovalue(float f);                  float         GetIsovalue()         const { return m_fIsovalue; }
  void          SetGridResolution(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z);

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  xiiTexture3DResourceHandle m_hDensityField;
  xiiMaterialResourceHandle  m_hMaterial;
  float     m_fIsovalue    = 0.5f;
  xiiUInt32 m_uiGridRes[3] = {64, 64, 64};
};

// ============================================================
//  Trajectory Line Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiTrajectoryLineRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTrajectoryLineRenderData, xiiRenderData);
public:
  xiiGALBufferHandle m_hPositionRing; ///< Circular ring buffer of Vec3 positions.
  xiiUInt32          m_uiCapacity   = 0;
  xiiUInt32          m_uiWriteHead  = 0;  ///< Current ring buffer write position.
  xiiUInt32          m_uiLiveCount  = 0;
  float              m_fLineWidth   = 1.0f;
  xiiColor           m_StartColor   = xiiColor::White;
  xiiColor           m_EndColor     = xiiColor::Black;
};

using xiiTrajectoryLineComponentManager = xiiComponentManager<class xiiTrajectoryLineComponent, xiiBlockStorageType::Compact>;

/// \brief Renders atom/robot trajectory history as GPU polylines from a ring buffer.
class XII_GRAPHICSCORE_DLL xiiTrajectoryLineComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTrajectoryLineComponent, xiiRenderComponent, xiiTrajectoryLineComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiTrajectoryLineComponent(); ~xiiTrajectoryLineComponent();

  void SetHistoryLength(xiiUInt32 uiFrames); xiiUInt32 GetHistoryLength() const { return m_uiHistoryLength; }
  void AppendPosition(const xiiVec3& pos);
  void SetLineWidth(float f);  float    GetLineWidth() const { return m_fLineWidth; }
  void SetStartColor(const xiiColor& c); const xiiColor& GetStartColor() const { return m_StartColor; }
  void SetEndColor(const xiiColor& c);   const xiiColor& GetEndColor()   const { return m_EndColor; }
  void Clear();

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  xiiGALBufferHandle m_hPositionRing;
  xiiUInt32          m_uiHistoryLength = 1024;
  xiiUInt32          m_uiWriteHead     = 0;
  xiiUInt32          m_uiLiveCount     = 0;
  float              m_fLineWidth      = 1.0f;
  xiiColor           m_StartColor      = xiiColor::White;
  xiiColor           m_EndColor        = xiiColor::Black;
};

// ============================================================
//  Scalar Field Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiScalarFieldRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScalarFieldRenderData, xiiRenderData);
public:
  xiiTexture3DResourceHandle m_hField;
  xiiTexture2DResourceHandle m_hTransferFunction; ///< 1D transfer function as 2D Nx1 texture.
  float                      m_fDensityScale  = 1.0f;
  float                      m_fStepSize      = 0.01f;
  xiiUInt16                  m_uiMaxSteps     = 512;
};

using xiiScalarFieldComponentManager = xiiComponentManager<class xiiScalarFieldComponent, xiiBlockStorageType::Compact>;

/// \brief Adaptive ray-march renderer for 3D scalar volumes (temperature, pressure, electron density).
class XII_GRAPHICSCORE_DLL xiiScalarFieldComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiScalarFieldComponent, xiiRenderComponent, xiiScalarFieldComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiScalarFieldComponent(); ~xiiScalarFieldComponent();

  void SetFieldFile(xiiStringView s);               xiiStringView GetFieldFile()          const;
  void SetTransferFunctionFile(xiiStringView s);    xiiStringView GetTransferFunctionFile()const;
  void SetDensityScale(float f);                    float         GetDensityScale()        const { return m_fDensityScale; }
  void SetStepSize(float f);                        float         GetStepSize()            const { return m_fStepSize; }
  void SetMaxSteps(xiiUInt16 n);                    xiiUInt16     GetMaxSteps()            const { return m_uiMaxSteps; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  xiiTexture3DResourceHandle m_hField;
  xiiTexture2DResourceHandle m_hTransferFunction;
  float     m_fDensityScale = 1.0f;
  float     m_fStepSize     = 0.01f;
  xiiUInt16 m_uiMaxSteps    = 512;
};

// ============================================================
//  Vector Field Component
// ============================================================

class XII_GRAPHICSCORE_DLL xiiVectorFieldRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVectorFieldRenderData, xiiRenderData);
public:
  xiiTexture3DResourceHandle m_hField;         ///< RGB32F vector field texture.
  xiiUInt32                  m_uiArrowDensity = 16; ///< Arrows per unit length.
  float                      m_fArrowScale    = 1.0f;
  xiiColor                   m_BaseColor      = xiiColor::White;
};

using xiiVectorFieldComponentManager = xiiComponentManager<class xiiVectorFieldComponent, xiiBlockStorageType::Compact>;

/// \brief GPU-instanced arrow renderer for 3D vector fields (force, velocity, EM).
class XII_GRAPHICSCORE_DLL xiiVectorFieldComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVectorFieldComponent, xiiRenderComponent, xiiVectorFieldComponentManager);
public:
  virtual void SerializeComponent(xiiWorldWriter& s) const override;
  virtual void DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiVectorFieldComponent(); ~xiiVectorFieldComponent();

  void SetFieldFile(xiiStringView s);      xiiStringView GetFieldFile()     const;
  void SetArrowDensity(xiiUInt32 n);       xiiUInt32     GetArrowDensity()  const { return m_uiArrowDensity; }
  void SetArrowScale(float f);             float         GetArrowScale()    const { return m_fArrowScale; }
  void SetBaseColor(const xiiColor& c);    const xiiColor& GetBaseColor()   const { return m_BaseColor; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const;
  xiiTexture3DResourceHandle m_hField;
  xiiUInt32                  m_uiArrowDensity = 16;
  float                      m_fArrowScale    = 1.0f;
  xiiColor                   m_BaseColor      = xiiColor::White;
};
