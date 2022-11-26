#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>

class xiiProcGenNodeBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGenNodeBase, xiiReflectedClass);

public:
  struct GraphContext
  {
    xiiProcGenInternal::GraphSharedData m_SharedData;
    xiiHybridArray<xiiUInt8, 4>         m_VolumeTagSetIndices;
  };

  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) = 0;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGenOutput : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGenOutput, xiiProcGenNodeBase);

public:
  bool m_bActive = true;

  void Save(xiiStreamWriter& stream);

  xiiString m_sName;

  xiiHybridArray<xiiUInt8, 4> m_VolumeTagSetIndices;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_PlacementOutput : public xiiProcGenOutput
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_PlacementOutput, xiiProcGenOutput);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  void Save(xiiStreamWriter& stream);

  xiiHybridArray<xiiString, 4> m_ObjectsToPlace;

  float m_fFootprint = 1.0f;

  xiiVec3 m_vMinOffset = xiiVec3(0);
  xiiVec3 m_vMaxOffset = xiiVec3(0);

  xiiAngle m_YawRotationSnap = xiiAngle::Radian(0.0f);
  float    m_fAlignToNormal  = 1.0f;

  xiiVec3 m_vMinScale = xiiVec3(1);
  xiiVec3 m_vMaxScale = xiiVec3(1);

  float m_fCullDistance = 30.0f;

  xiiUInt32 m_uiCollisionLayer = 0;

  xiiString m_sSurface;

  xiiString m_sColorGradient;

  xiiEnum<xiiProcPlacementMode> m_PlacementMode;

  xiiRenderPipelineNodeInputPin m_DensityPin;
  xiiRenderPipelineNodeInputPin m_ScalePin;
  xiiRenderPipelineNodeInputPin m_ColorIndexPin;
  xiiRenderPipelineNodeInputPin m_ObjectIndexPin;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_VertexColorOutput : public xiiProcGenOutput
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_VertexColorOutput, xiiProcGenOutput);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  void Save(xiiStreamWriter& stream);

  xiiRenderPipelineNodeInputPin m_RPin;
  xiiRenderPipelineNodeInputPin m_GPin;
  xiiRenderPipelineNodeInputPin m_BPin;
  xiiRenderPipelineNodeInputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_Random : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_Random, xiiProcGenNodeBase);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  xiiInt32 m_iSeed = -1;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  xiiRenderPipelineNodeOutputPin m_OutputValuePin;

private:
  void OnObjectCreated(const xiiAbstractObjectNode& node);

  xiiUInt32 m_uiAutoSeed;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_PerlinNoise : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_PerlinNoise, xiiProcGenNodeBase);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  xiiVec3   m_Scale        = xiiVec3(10);
  xiiVec3   m_Offset       = xiiVec3::ZeroVector();
  xiiUInt32 m_uiNumOctaves = 3;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  xiiRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_Blend : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_Blend, xiiProcGenNodeBase);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  xiiEnum<xiiProcGenBinaryOperator> m_Operator;
  float                             m_fInputValueA = 1.0f;
  float                             m_fInputValueB = 1.0f;
  bool                              m_bClampOutput = false;

  xiiRenderPipelineNodeInputPin  m_InputValueAPin;
  xiiRenderPipelineNodeInputPin  m_InputValueBPin;
  xiiRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_Height : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_Height, xiiProcGenNodeBase);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  float m_fMinHeight = 0.0f;
  float m_fMaxHeight = 1000.0f;
  float m_fLowerFade = 0.2f;
  float m_fUpperFade = 0.2f;

  xiiRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_Slope : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_Slope, xiiProcGenNodeBase);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  xiiAngle m_MinSlope   = xiiAngle::Degree(0.0f);
  xiiAngle m_MaxSlope   = xiiAngle::Degree(30.0f);
  float    m_fLowerFade = 0.0f;
  float    m_fUpperFade = 0.2f;

  xiiRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_MeshVertexColor : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_MeshVertexColor, xiiProcGenNodeBase);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  xiiRenderPipelineNodeOutputPin m_RPin;
  xiiRenderPipelineNodeOutputPin m_GPin;
  xiiRenderPipelineNodeOutputPin m_BPin;
  xiiRenderPipelineNodeOutputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class xiiProcGen_ApplyVolumes : public xiiProcGenNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGen_ApplyVolumes, xiiProcGenNodeBase);

public:
  virtual xiiExpressionAST::Node* GenerateExpressionASTNode(xiiTempHashedString sOutputName, xiiArrayPtr<xiiExpressionAST::Node*> inputs, xiiExpressionAST& out_Ast, GraphContext& context) override;

  float m_fInputValue = 0.0f;

  xiiTagSet m_IncludeTags;

  xiiEnum<xiiProcVolumeImageMode> m_ImageVolumeMode;
  xiiColorGammaUB                 m_RefColor;

  xiiRenderPipelineNodeInputPin  m_InputValuePin;
  xiiRenderPipelineNodeOutputPin m_OutputValuePin;
};
