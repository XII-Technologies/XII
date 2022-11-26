#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

class xiiPhysicsWorldModuleInterface;
class xiiVolumeCollection;

namespace xiiProcGenInternal
{
  class PlacementTask final : public xiiTask
  {
  public:
    PlacementTask(PlacementData* pData, const char* szName);
    ~PlacementTask();

    void Clear();

    xiiArrayPtr<const PlacementPoint>     GetInputPoints() const { return m_InputPoints; }
    xiiArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    virtual void Execute() override;

    void FindPlacementPoints();
    void ExecuteVM();

    xiiProcessingStream MakeInputStream(const xiiHashedString& sName, xiiUInt32 uiOffset)
    {
      return xiiProcessingStream(sName, m_InputPoints.GetByteArrayPtr().GetSubArray(uiOffset), xiiProcessingStream::DataType::Float, sizeof(PlacementPoint));
    }

    PlacementData* m_pData = nullptr;

    xiiDynamicArray<PlacementPoint, xiiAlignedAllocatorWrapper>     m_InputPoints;
    xiiDynamicArray<PlacementTransform, xiiAlignedAllocatorWrapper> m_OutputTransforms;
    xiiDynamicArray<float>                                          m_TempData;
    xiiDynamicArray<xiiUInt32>                                      m_ValidPoints;

    xiiExpressionVM m_VM;
  };
} // namespace xiiProcGenInternal
