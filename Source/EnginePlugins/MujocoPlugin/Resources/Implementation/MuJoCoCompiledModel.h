/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Foundation/Types/SharedPtr.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>

#include <mujoco/mujoco.h>

#include <memory>

/// Immutable native model storage shared by the resource and its active simulations.
class xiiMuJoCoCompiledModel final : public xiiRefCounted
{
public:
  using ModelPtr = std::unique_ptr<mjModel, decltype(&mj_deleteModel)>;

  explicit xiiMuJoCoCompiledModel(ModelPtr pModel) :
    m_pModel(std::move(pModel))
  {
  }

  using SourceFile = xiiMuJoCoModelResourceDescriptor::SourceFile;
  static xiiSharedPtr<xiiMuJoCoCompiledModel> LoadBinary(xiiArrayPtr<const xiiUInt8> bytes, xiiStringView sSource, xiiDynamicArray<SourceFile> sourceFiles = {});
  static xiiSharedPtr<xiiMuJoCoCompiledModel> Adopt(ModelPtr pModel, xiiStringView sSource);

  const mjModel*                GetModel() const { return m_pModel.get(); }
  xiiArrayPtr<const SourceFile> GetSourceFiles() const { return m_SourceFiles; }

private:
  ModelPtr                    m_pModel;
  xiiDynamicArray<SourceFile> m_SourceFiles;
};
