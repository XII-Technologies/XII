#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

struct xiiMeshBufferResourceDescriptor;
class xiiVolumeCollection;

namespace xiiProcGenInternal
{
  class VertexColorTask final : public xiiTask
  {
  public:
    VertexColorTask();
    ~VertexColorTask();

    void Prepare(const xiiWorld& world, const xiiMeshBufferResourceDescriptor& mbDesc, const xiiTransform& transform, xiiArrayPtr<xiiSharedPtr<const VertexColorOutput>> outputs, xiiArrayPtr<xiiProcVertexColorMapping> outputMappings, xiiArrayPtr<xiiUInt32> outputVertexColors);

  private:
    virtual void Execute() override;

    xiiHybridArray<xiiSharedPtr<const VertexColorOutput>, 2> m_Outputs;
    xiiHybridArray<xiiProcVertexColorMapping, 2>             m_OutputMappings;

    struct InputVertex
    {
      XII_DECLARE_POD_TYPE();

      xiiVec3  m_vPosition;
      xiiVec3  m_vNormal;
      xiiColor m_Color;
      float    m_fIndex;
    };

    xiiDynamicArray<InputVertex> m_InputVertices;

    xiiDynamicArray<xiiColor> m_TempData;
    xiiArrayPtr<xiiUInt32>    m_OutputVertexColors;

    xiiDeque<xiiVolumeCollection> m_VolumeCollections;
    xiiExpression::GlobalData     m_GlobalData;

    xiiExpressionVM m_VM;
  };
} // namespace xiiProcGenInternal
