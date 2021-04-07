#ifndef APP_H
#define APP_H



#include "Core.h"
#include "Utilities/DescriptorHeap.h"
#include "Utilities/MathUtils.h"
#include "Utilities/Utils.h"
#include "Frames/FrameResource.h"
#include "Assets/Geometry.h"
#include "Assets/Texture.h"
#include "Assets/Scene.h"
#include "Features/ShadowMap.h"
#include "Features/DeferredRenderer.h"
#include "Features/MeshVoxelizer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;


class App : public Core {
public:
    App(HINSTANCE hInstance);
    App(const App& rhs) = delete;
    App& operator=(const App& rhs) = delete;
    ~App();

    virtual bool Initialize()override;

private:

    virtual void CreateRtvAndDsvDescriptorHeaps();
    virtual void OnResize()override;
    virtual void Update(const Timer& gt)override;
    virtual void Draw(const Timer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;



    void OnKeyboardInput(const Timer& gt);
    void UpdateCamera(const Timer& gt);
    void UpdateObjectCBs(const Timer& gt);
    void UpdateMainPassCB(const Timer& gt);
    void UpdateMaterialCBs(const Timer& gt);
    void UpdateShadowPassCB(const Timer& gt);
    void UpdateRadiancePassCB(const Timer& gt);
    void UpdateCBs(const Timer& gt);
    void UpdateScenePhysics(const Timer& gt);

    void BuildDescriptorHeaps();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildPSOs();
    void CreatePSO(
        ID3D12PipelineState** pso,
        ID3D12RootSignature* rootSignature,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopoType,
        D3D12_BLEND_DESC blendDesc,
        D3D12_RASTERIZER_DESC rasterDesc,
        D3D12_DEPTH_STENCIL_DESC dsState,
        UINT numRenderTargets,
        DXGI_FORMAT renderTargetFormat,
        DXGI_FORMAT depthStencilFormat,
        ID3DBlob* vertexShader,
        ID3DBlob* pixelShader,
        ID3DBlob* geometryShader = nullptr
        );

    void BuildFrameResources();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<ObjectInfo*>& objInfos);

    void DrawScene2ShadowMap();
    void DrawScene2GBuffers();
    void VoxelizeMesh();
    void InjectRadiance();
    void FillBaseMip();
    void DrawScene();

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    std::unique_ptr<Scene> mScene;   
    std::unique_ptr<ShadowMap> mShadowMap;
    std::unique_ptr<DeferredRenderer> mDeferredRenderer;
    std::unique_ptr<MeshVoxelizer> mMeshVoxelizer;

    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders; 
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mRootSignatures;
    std::unordered_map<std::string, std::unique_ptr<mDescriptorHeap>> mSrvHeaps;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    // Render items divided by PSO.


    UINT NumRTVs;
    UINT NumDSVs;
    UINT NumSRVs;


    //common camera
    XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4X4 mView = MathUtils::Identity4x4();
    XMFLOAT4X4 mProj = MathUtils::Identity4x4();
    float mTheta = 1.5f * XM_PI;
    float mPhi = 0.2f * XM_PI;
    float mRadius = 85.0f;


    POINT mLastMousePos;

    bool mIsWireframe = false;
    int mShowVoxel = 1;
  
};


#endif // !APP_H
