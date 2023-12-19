#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>

class xiiDocumentNodeManager;

class xiiVisualShaderCodeGenerator
{
public:
  xiiVisualShaderCodeGenerator();

  xiiStatus GenerateVisualShader(const xiiDocumentNodeManager* pNodeMaanger, xiiStringBuilder& out_sCheckPerms);

  const char* GetFinalShaderCode() const { return m_sFinalShaderCode; }

  void DetermineConfigFileDependencies(const xiiDocumentNodeManager* pNodeManager, xiiSet<xiiString>& out_cfgFiles);

private:
  struct NodeState
  {
    NodeState()
    {
      m_uiNodeId       = 0;
      m_bCodeGenerated = false;
      m_bInProgress    = false;
    }

    xiiUInt16 m_uiNodeId;
    bool      m_bCodeGenerated;
    bool      m_bInProgress;
  };

  struct OutputPinState
  {
    OutputPinState() { m_bCodeGenerated = false; }

    bool      m_bCodeGenerated;
    xiiString m_sCodeAtPin;
  };


  xiiStatus GatherAllNodes(const xiiDocumentObject* pRootObj);
  xiiUInt16 DeterminePinId(const xiiDocumentObject* pOwner, const xiiPin& pin) const;
  xiiStatus GenerateNode(const xiiDocumentObject* pNode);
  xiiStatus GenerateInputPinCode(xiiArrayPtr<const xiiUniquePtr<const xiiPin>> pins);
  xiiStatus CheckPropertyValues(const xiiDocumentObject* pNode, const xiiVisualShaderNodeDescriptor* pDesc);
  xiiStatus InsertPropertyValues(const xiiDocumentObject* pNode, const xiiVisualShaderNodeDescriptor* pDesc, xiiStringBuilder& sString);
  xiiStatus GenerateOutputPinCode(const xiiDocumentObject* pOwnerNode, const xiiPin& pinSource);

  xiiStatus   ReplaceInputPinsByCode(const xiiDocumentObject* pOwnerNode, const xiiVisualShaderNodeDescriptor* pNodeDesc, xiiStringBuilder& sInlineCode, xiiStringBuilder& sCodeForPlacingDefines);
  void        SetPinDefines(const xiiDocumentObject* pOwnerNode, xiiStringBuilder& sInlineCode);
  static void AppendStringIfUnique(xiiStringBuilder& inout_String, const char* szAppend);

  const xiiDocumentObject*                    m_pMainNode;
  const xiiVisualShaderTypeRegistry*          m_pTypeRegistry;
  const xiiDocumentNodeManager*               m_pNodeManager;
  const xiiRTTI*                              m_pNodeBaseRtti;
  xiiMap<const xiiDocumentObject*, NodeState> m_Nodes;
  xiiMap<const xiiPin*, OutputPinState>       m_OutputPins;
  xiiMap<xiiInt8, xiiSet<xiiString>>          m_UsedUniqueValues;

  xiiStringBuilder m_sShaderPixelDefines;
  xiiStringBuilder m_sShaderPixelIncludes;
  xiiStringBuilder m_sShaderPixelConstants;
  xiiStringBuilder m_sShaderPixelSamplers;
  xiiStringBuilder m_sShaderPixelBody;
  xiiStringBuilder m_sShaderVertexDefines;
  xiiStringBuilder m_sShaderVertex;
  xiiStringBuilder m_sShaderGeometryDefines;
  xiiStringBuilder m_sShaderGeometry;
  xiiStringBuilder m_sShaderMaterialParam;
  xiiStringBuilder m_sShaderMaterialCB;
  xiiStringBuilder m_sShaderRenderState;
  xiiStringBuilder m_sShaderPermutations;
  xiiStringBuilder m_sFinalShaderCode;
};
