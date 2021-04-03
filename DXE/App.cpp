#include "App.h"

App::App(HINSTANCE hInstance) : Core(hInstance) {

    NumRTVs = SwapChainBufferCount + static_cast<UINT>(GBUFFER_TYPE::COUNT); // back buffer + gbuffers
    NumDSVs = 2;                    // dsv for swapchain & shadowmap

}

App::~App() {
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

// SRVdescHEAP : tex1, tex2, tex3, tex4 ... ... 
// CPU handle     ch1   ch2   ch3   ch4  ... ... this goes to creation for srv
// GPU handle     gh1   gh2   gh3   gh4  ... ... this goes to command list set up for shader inputs (createRootDescriptorTable)

// unorderedmap of mesh textures
// shadowMap texture
void App::BuildDescriptorHeaps() {


    NumSRVs = mScene->getTexturesMap().size() + 1 + static_cast<UINT>(GBUFFER_TYPE::COUNT) + 2; // + 1 for shadow map + gbuffer + 2 for mesh voxelizer (SRV & UAV)

    auto mMainPassSrvHeap = std::make_unique<mDescriptorHeap>();
    mMainPassSrvHeap->Create(md3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        (UINT)(NumSRVs),
        true);
    mSrvHeaps["MainPass"] = std::move(mMainPassSrvHeap);

    UINT offset = 0;
    for (auto& tex : mScene->getTexturesMap()) {
        auto texture = tex.second->textureBuffer;
        auto textureDesc = tex.second->getSRVDESC();
        md3dDevice->CreateShaderResourceView(texture.Get(), &textureDesc, mSrvHeaps["MainPass"]->mCPUHandle(tex.second->textureID));
        mSrvHeaps["MainPass"]->incrementCurrentOffset();
    }

    // ================================================
    // set descriptor heap addresses for shadowmap
    // ================================================
    auto shadowMapCPUSrvHandle = mSrvHeaps["MainPass"]->mCPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
    auto shadowMapGPUSrvHandle = mSrvHeaps["MainPass"]->mGPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
    mSrvHeaps["MainPass"]->incrementCurrentOffset();

    auto shadowMapCPUDsvHandle = mDsvHeap->mCPUHandle(mDsvHeap->getCurrentOffsetRef());
    mDsvHeap->incrementCurrentOffset();
    mShadowMap->SetupCPUGPUDescOffsets(shadowMapCPUSrvHandle, shadowMapGPUSrvHandle, shadowMapCPUDsvHandle);

    // ================================================
    // set descriptor heap addresses for deferredrenderer
    // ================================================


   
    for (auto& gbuffer : mDeferredRenderer->getGbuffersMap()) {

        auto deferredRendererCPURtvHandle = mRtvHeap->mCPUHandle(mRtvHeap->getCurrentOffsetRef());
        mRtvHeap->incrementCurrentOffset();

        auto deferredRendererCPUSrvHandle = mSrvHeaps["MainPass"]->mCPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
        auto deferredRendererGPUSrvHandle = mSrvHeaps["MainPass"]->mGPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
        gbuffer.second->SetupCPUGPUDescOffsets(deferredRendererCPUSrvHandle, deferredRendererGPUSrvHandle, deferredRendererCPURtvHandle);
        mSrvHeaps["MainPass"]->incrementCurrentOffset();

    }

    // ================================================
    // set descriptor heap addresses for voxelizer
    // ================================================

    auto meshVoxelizerCPUSrvHandle = mSrvHeaps["MainPass"]->mCPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
    auto meshVoxelizerGPUSrvHandle = mSrvHeaps["MainPass"]->mGPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
    mSrvHeaps["MainPass"]->incrementCurrentOffset();
    auto meshVoxelizerCPUUavHandle = mSrvHeaps["MainPass"]->mCPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
    auto meshVoxelizerGPUUavHandle = mSrvHeaps["MainPass"]->mGPUHandle(mSrvHeaps["MainPass"]->getCurrentOffsetRef());
    mMeshVoxelizer->SetupCPUGPUDescOffsets(meshVoxelizerCPUSrvHandle, meshVoxelizerGPUSrvHandle, meshVoxelizerCPUUavHandle, meshVoxelizerGPUUavHandle);
}

void App::BuildRootSignature()
{

    // =================================================
    // main pass root signature 
    // =================================================

    // range means the location in HLSL shader
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //t0
    CD3DX12_DESCRIPTOR_RANGE texTableShadow;
    texTableShadow.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); //t1
    CD3DX12_DESCRIPTOR_RANGE gbufferTable;
    gbufferTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (int)GBUFFER_TYPE::COUNT, 2); //t2, t3 ,t4, t5
    CD3DX12_DESCRIPTOR_RANGE voxelTexTable;
    voxelTexTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0
    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::COUNT];
    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::DIFFUSE_TEX_TABLE].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // mesh textures
    slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::SHADOWMAP_TEX_TABLE].InitAsDescriptorTable(1, &texTableShadow, D3D12_SHADER_VISIBILITY_PIXEL); // shadow map
    slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::G_BUFFER].InitAsDescriptorTable(1, &gbufferTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::VOXEL].InitAsDescriptorTable(1, &voxelTexTable, D3D12_SHADER_VISIBILITY_ALL);
    slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::OBJ_CBV].InitAsConstantBufferView(0); // obj const buffer
    slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::MAINPASS_CBV].InitAsConstantBufferView(1); // pass const buffer
    slotRootParameter[d3dUtil::MAIN_PASS_UNIFORM::MATERIAL_CBV].InitAsConstantBufferView(2); // material const buffer
    auto staticSamplers = GetStaticSamplers();
    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(d3dUtil::MAIN_PASS_UNIFORM::COUNT, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignatures["MainPass"].GetAddressOf())));


    // =================================================
    // compute reset pass root signature 
    // =================================================

    CD3DX12_DESCRIPTOR_RANGE voxelTexTableComp;
    voxelTexTableComp.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0

    CD3DX12_ROOT_PARAMETER slotRootParameterComp[1];
    slotRootParameterComp[0].InitAsDescriptorTable(1, &voxelTexTableComp, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDescComp(1, slotRootParameterComp, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> serializedRootSigComp = nullptr;
    ComPtr<ID3DBlob> errorBlobComp = nullptr;
    hr = D3D12SerializeRootSignature(&rootSigDescComp, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSigComp.GetAddressOf(), errorBlobComp.GetAddressOf());
    if (errorBlobComp != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlobComp->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSigComp->GetBufferPointer(),
        serializedRootSigComp->GetBufferSize(),
        IID_PPV_ARGS(mRootSignatures["CompResetPass"].GetAddressOf())));

}



bool App::Initialize() {

    if (!Core::Initialize()) {
        return false;
    }

    mScene = std::make_unique<Scene>(md3dDevice.Get(), mClientWidth, mClientHeight);
    mScene->initScene();

    mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(), 2048, 2048);
    mShadowMap->initShadowMap();

    mDeferredRenderer = std::make_unique<DeferredRenderer>(md3dDevice.Get(), mClientWidth, mClientHeight);
    mDeferredRenderer->initDeferredRenderer();

    mMeshVoxelizer = std::make_unique<MeshVoxelizer>(md3dDevice.Get(), 256, 256, 256);
    mMeshVoxelizer->init3DVoxelTexture();

    BuildDescriptorHeaps();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildFrameResources();
    BuildPSOs();

    return true;
}

void App::UpdateCBs(const Timer& gt) {

    UpdateObjectCBs(gt);
    UpdateMaterialCBs(gt);
    UpdateMainPassCB(gt);
    UpdateShadowPassCB(gt);

}

void App::Update(const Timer& gt) {

    OnKeyboardInput(gt);
    UpdateCamera(gt);
    if (mShadowMap.get()) {
        mShadowMap->Update(gt);
    }

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % d3dUtil::gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, L"d", false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateCBs(gt);
    UpdateScenePhysics(gt);

}

void App::UpdateCamera(const Timer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
    mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
    mEyePos.y = mRadius * cosf(mPhi);

    // Build the view matrix.
    XMVECTOR pos = DirectX::XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
    XMVECTOR target = DirectX::XMVectorZero();
    XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
    DirectX::XMStoreFloat4x4(&mView, view);
}

void App::UpdateObjectCBs(const Timer& gt) {
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto& e : mScene->getObjectInfos())
    {
        // Only update the cbuffer data if the constants have changed.  
        // This needs to be tracked per frame resource.
        if (e.second->NumFramesDirty > 0)
        {
            XMMATRIX world = DirectX::XMLoadFloat4x4(&e.second->World);
            XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&e.second->texTransform);

            ObjectConstants objConstants;
            DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
            DirectX::XMStoreFloat4x4(&objConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));
            objConstants.Obj2VoxelScale = e.second->Obj2VoxelScale;
            currObjectCB->CopyData(e.second->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            e.second->NumFramesDirty--;
        }
    }
}

void App::UpdateScenePhysics(const Timer& gt) {
   
    DirectX::XMStoreFloat4x4(&mScene->getObjectInfos()["model1"]->World, DirectX::XMMatrixScaling(10.0f, 10.0f, 10.0f) * DirectX::XMMatrixTranslation(10.0f * std::sin(float(gt.TotalTime())), 15.0f, 0.0 ));
    mScene->getObjectInfos()["model1"]->NumFramesDirty = d3dUtil::gNumFrameResources;
}

void App::UpdateMaterialCBs(const Timer& gt) {
    auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
    for (auto& e : mScene->getMaterialMap()) {
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0) {
            XMMATRIX matTransform = DirectX::XMLoadFloat4x4(&mat->MatTransform);

            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            DirectX::XMStoreFloat4x4(&matConstants.MatTransform, DirectX::XMMatrixTranspose(matTransform));

            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

            mat->NumFramesDirty--;
        }
    }
}

void App::UpdateMainPassCB(const Timer& gt)
{
    PassConstants mMainPassCB; // 0 in main pass CB

    //XMMATRIX view = DirectX::XMLoadFloat4x4(&mView);
    //XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);

    XMMATRIX view = mScene->getCamerasMap()["MainCam"]->GetView();
    XMMATRIX proj = mScene->getCamerasMap()["MainCam"]->GetProj();

    XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
    XMMATRIX invView = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(view), view);
    XMMATRIX invProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewProj), viewProj);
    XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowMap->mShadowMapData.mShadowTransform);
    XMMATRIX voxelView = XMLoadFloat4x4(&mMeshVoxelizer->getUniformData().mVoxelView); 
    XMMATRIX voxelProj = XMLoadFloat4x4(&mMeshVoxelizer->getUniformData().mVoxelProj);
    XMMATRIX voxelViewProj = DirectX::XMMatrixMultiply(voxelView, voxelProj);

    // use transpose to make sure we go from row - major on cpu to colume major on gpu
    DirectX::XMStoreFloat4x4(&mMainPassCB.View, DirectX::XMMatrixTranspose(view));
    DirectX::XMStoreFloat4x4(&mMainPassCB.InvView, DirectX::XMMatrixTranspose(invView));
    DirectX::XMStoreFloat4x4(&mMainPassCB.Proj, DirectX::XMMatrixTranspose(proj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.ViewProj, DirectX::XMMatrixTranspose(viewProj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.ShadowTransform, DirectX::XMMatrixTranspose(shadowTransform));
    DirectX::XMStoreFloat4x4(&mMainPassCB.VoxelView, DirectX::XMMatrixTranspose(voxelView));
    DirectX::XMStoreFloat4x4(&mMainPassCB.VoxelProj, DirectX::XMMatrixTranspose(voxelProj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.VoxelViewProj, DirectX::XMMatrixTranspose(voxelViewProj));
    mMainPassCB.EyePosW = mScene->getCamerasMap()["MainCam"]->GetPosition3f();
    mMainPassCB.LightPosW = mShadowMap->mShadowMapData.mLightPosW;
    mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();
    mMainPassCB.camLookDir = mScene->getCamerasMap()["MainCam"]->GetLook3f();
    mMainPassCB.camUpDir = mScene->getCamerasMap()["MainCam"]->GetUp3f();
    mMainPassCB.showVoxel = mShowVoxel;

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void App::UpdateShadowPassCB(const Timer& gt) {

    PassConstants mShadowPassCB; // 1 in main pass CB
    XMMATRIX view = DirectX::XMLoadFloat4x4(&mShadowMap->mShadowMapData.mLightView);
    XMMATRIX proj = DirectX::XMLoadFloat4x4(&mShadowMap->mShadowMapData.mLightProj);

    XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
    XMMATRIX invView = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(view), view);
    XMMATRIX invProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewProj), viewProj);

    UINT w = mShadowMap->Width();
    UINT h = mShadowMap->Height();
    DirectX::XMStoreFloat4x4(&mShadowPassCB.View, DirectX::XMMatrixTranspose(view));
    DirectX::XMStoreFloat4x4(&mShadowPassCB.InvView, DirectX::XMMatrixTranspose(invView));
    DirectX::XMStoreFloat4x4(&mShadowPassCB.Proj, DirectX::XMMatrixTranspose(proj));
    DirectX::XMStoreFloat4x4(&mShadowPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
    DirectX::XMStoreFloat4x4(&mShadowPassCB.ViewProj, DirectX::XMMatrixTranspose(viewProj));
    DirectX::XMStoreFloat4x4(&mShadowPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
    mShadowPassCB.EyePosW = mShadowMap->mShadowMapData.mLightPosW;
    mShadowPassCB.LightPosW = mShadowMap->mShadowMapData.mLightPosW;
    mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
    mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
    mShadowPassCB.NearZ = mShadowMap->mShadowMapData.mLightNearZ;
    mShadowPassCB.FarZ = mShadowMap->mShadowMapData.mLightFarZ;
    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(1, mShadowPassCB);
}
void App::OnResize() {

    Core::OnResize();
    XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathUtils::Pi, AspectRatio(), 1.0f, 1000.0f);
    DirectX::XMStoreFloat4x4(&mProj, P);

    if (mScene.get()) {
        mScene->resize(mClientWidth, mClientHeight);
    }

    if (mDeferredRenderer.get()) {
        mDeferredRenderer->onResize(mClientWidth, mClientHeight);
    }
}



void App::Draw(const Timer& gt) {


    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.

    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    // only need to set SRV CBV UAV here, since RTV and DSV will never be accessed inside a shader (only written to)
    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeaps["MainPass"]->mPtr() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignatures["MainPass"].Get());
    mCommandList->SetGraphicsRootDescriptorTable(d3dUtil::MAIN_PASS_UNIFORM::SHADOWMAP_TEX_TABLE, mShadowMap->getGPUHandle4SRV());
    mCommandList->SetGraphicsRootDescriptorTable(d3dUtil::MAIN_PASS_UNIFORM::G_BUFFER, mDeferredRenderer->getGBuffer(GBUFFER_TYPE::POSITION)->getGPUHandle4SRV());// starting GPU handle location for all gbuffers
    mCommandList->SetGraphicsRootDescriptorTable(d3dUtil::MAIN_PASS_UNIFORM::VOXEL, mMeshVoxelizer->getGPUHandle4UAV());

    DrawScene2GBuffers();
    DrawScene2ShadowMap();
    mMeshVoxelizer->Clear3DTexture(mCommandList.Get(), mRootSignatures["CompResetPass"].Get(), mPSOs["CompReset"].Get());
    VoxelizeMesh();
    DrawScene();

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);


}

void App::OnMouseDown(WPARAM btnState, int x, int y) {
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void App::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void App::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        mScene->getCamerasMap()["MainCam"]->Pitch(dy);
        mScene->getCamerasMap()["MainCam"]->RotateY(dx);

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathUtils::Clamp(mPhi, 0.1f, MathUtils::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathUtils::Clamp(mRadius, 5.0f, 550.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void App::OnKeyboardInput(const Timer& gt)
{
    const float dt = gt.DeltaTime();
    float speed = 30.0f;

    if (GetAsyncKeyState('1') & 0x8000)
        mIsWireframe = true;
    else
        mIsWireframe = false;
    if (GetAsyncKeyState('Q') & 0x8000) {
        mShowVoxel = true;
    }
    else {
        mShowVoxel = false;
    }
    if (GetAsyncKeyState('W') & 0x8000)
        mScene->getCamerasMap()["MainCam"]->Walk(speed * dt);

    if (GetAsyncKeyState('S') & 0x8000)
        mScene->getCamerasMap()["MainCam"]->Walk(-speed * dt);

    if (GetAsyncKeyState('A') & 0x8000)
        mScene->getCamerasMap()["MainCam"]->Strafe(-speed * dt);

    if (GetAsyncKeyState('D') & 0x8000)
        mScene->getCamerasMap()["MainCam"]->Strafe(speed * dt);
    mScene->getCamerasMap()["MainCam"]->UpdateViewMatrix();
}



// overriding function in core
void App::CreateRtvAndDsvDescriptorHeaps()
{
    //D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    //rtvHeapDesc.NumDescriptors = NumRTVs;
    //rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    //rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    //rtvHeapDesc.NodeMask = 0;
    //ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
    //    &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    //// 0 ==== dsv for default swapchain
    //// 1 ==== dsv for shadow map

    //D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    //dsvHeapDesc.NumDescriptors = NumDSVs;
    //dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    //dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    //dsvHeapDesc.NodeMask = 0;
    //ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
    //    &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

    mRtvHeap = std::make_unique<mDescriptorHeap>();
    mRtvHeap->Create(md3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        (UINT)(NumRTVs),
        false);

    mDsvHeap = std::make_unique<mDescriptorHeap>();
    mDsvHeap->Create(md3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        (UINT)(NumDSVs),
        false);

}


void App::BuildShadersAndInputLayout()
{
    mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders/pix.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders/pix.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders/shadow.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders/shadow.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["debugVS"] = d3dUtil::CompileShader(L"Shaders/shadowDebug.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["debugPS"] = d3dUtil::CompileShader(L"Shaders/shadowDebug.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["deferredVS"] = d3dUtil::CompileShader(L"Shaders/deferred.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["deferredPS"] = d3dUtil::CompileShader(L"Shaders/deferred.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["voxelizerVS"] = d3dUtil::CompileShader(L"Shaders/voxelizer.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["voxelizerGS"] = d3dUtil::CompileShader(L"Shaders/voxelizer.hlsl", nullptr, "GS", "gs_5_1");
    mShaders["voxelizerPS"] = d3dUtil::CompileShader(L"Shaders/voxelizer.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["voxelizerCompReset"] = d3dUtil::CompileShader(L"Shaders/voxelizer.hlsl", nullptr, "CompReset", "cs_5_1");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void App::CreatePSO(
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
    ID3DBlob* geometryShader
) {

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc;
    ZeroMemory(&PsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    PsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    PsoDesc.pRootSignature = rootSignature;
    if (vertexShader) {
        PsoDesc.VS = d3dUtil::getShaderBytecode(vertexShader);
    }if (pixelShader) {
        PsoDesc.PS = d3dUtil::getShaderBytecode(pixelShader);
    }if (geometryShader) {
        PsoDesc.GS = d3dUtil::getShaderBytecode(geometryShader);
    }
    PsoDesc.RasterizerState = rasterDesc;
    PsoDesc.BlendState = blendDesc;
    PsoDesc.DepthStencilState = dsState;
    PsoDesc.SampleMask = UINT_MAX;
    PsoDesc.PrimitiveTopologyType = primitiveTopoType;
    PsoDesc.NumRenderTargets = numRenderTargets;
    if (numRenderTargets == 0) PsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    for (unsigned int i = 0; i < numRenderTargets; ++i) {
        PsoDesc.RTVFormats[i] = renderTargetFormat;
    }
    PsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    PsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    PsoDesc.DSVFormat = depthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(pso)));
}

void App::BuildPSOs() {


    // =====================================
    // PSO for Main pass.
    // =====================================

    D3D12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
    CreatePSO(
        mPSOs["opaque"].GetAddressOf(),
        mRootSignatures["MainPass"].Get(),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        rasterDesc,
        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        1,
        mBackBufferFormat,
        mDepthStencilFormat,
        mShaders["standardVS"].Get(),
        mShaders["opaquePS"].Get());

    // =====================================
    // PSO for shadow map pass.
    // =====================================

    auto shadowRasterDesc = rasterDesc;
    shadowRasterDesc.DepthBias = 100000;
    shadowRasterDesc.DepthBiasClamp = 0.0f;
    shadowRasterDesc.SlopeScaledDepthBias = 1.0f;
    CreatePSO(
        mPSOs["shadow_opaque"].GetAddressOf(),
        mRootSignatures["MainPass"].Get(),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        shadowRasterDesc,
        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        0,
        DXGI_FORMAT_UNKNOWN,
        mDepthStencilFormat,
        mShaders["shadowVS"].Get(),
        mShaders["shadowOpaquePS"].Get());

    // =====================================
    // PSO for debug layer.
    // =====================================
    D3D12_DEPTH_STENCIL_DESC dsvDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    dsvDesc.DepthEnable = false;
    dsvDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    CreatePSO(
        mPSOs["debug"].GetAddressOf(),
        mRootSignatures["MainPass"].Get(),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        rasterDesc,
        dsvDesc,
        1,
        mBackBufferFormat,
        DXGI_FORMAT_UNKNOWN,
        mShaders["debugVS"].Get(),
        mShaders["debugPS"].Get());

    // =====================================
    // PSO for opaque wireframe objects.
    // =====================================

    auto wireFrameRasterDesc = rasterDesc;
    wireFrameRasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
    CreatePSO(
        mPSOs["opaque_wireframe"].GetAddressOf(),
        mRootSignatures["MainPass"].Get(),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        wireFrameRasterDesc,
        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        1,
        mBackBufferFormat,
        mDepthStencilFormat,
        mShaders["standardVS"].Get(),
        mShaders["opaquePS"].Get());

    // =====================================
    // PSO for deferred renderer
    // =====================================
    CreatePSO(
        mPSOs["deferred"].GetAddressOf(),
        mRootSignatures["MainPass"].Get(),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        rasterDesc,
        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        (UINT)GBUFFER_TYPE::COUNT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        mDepthStencilFormat,
        mShaders["deferredVS"].Get(),
        mShaders["deferredPS"].Get()
    );

    // =====================================
    // PSO for deferred renderer 2nd pass
    // =====================================
 
    CreatePSO(
        mPSOs["deferredPost"].GetAddressOf(),
        mRootSignatures["MainPass"].Get(),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        rasterDesc,
        dsvDesc,
        1,
        mBackBufferFormat,
        DXGI_FORMAT_UNKNOWN,
        mShaders["standardVS"].Get(),
        mShaders["opaquePS"].Get()
    );

    // =====================================
    // PSO for voxelizer renderer pass
    // =====================================
    auto voxelRasterDesc = rasterDesc;
    voxelRasterDesc.CullMode = D3D12_CULL_MODE_NONE;
    CreatePSO(
        mPSOs["voxelizer"].GetAddressOf(),
        mRootSignatures["MainPass"].Get(),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        voxelRasterDesc,
        dsvDesc,
        0,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        mShaders["voxelizerVS"].Get(),
        mShaders["voxelizerPS"].Get(),
        mShaders["voxelizerGS"].Get()
    );

    // =====================================
    // PSO for compute reset pass
    // =====================================
    D3D12_COMPUTE_PIPELINE_STATE_DESC CompResetDesc = {};
    CompResetDesc.pRootSignature = mRootSignatures["CompResetPass"].Get();
    CompResetDesc.CS = {
        reinterpret_cast<BYTE*>(mShaders["voxelizerCompReset"]->GetBufferPointer()),
        mShaders["voxelizerCompReset"]->GetBufferSize()
    };
    CompResetDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateComputePipelineState(&CompResetDesc, IID_PPV_ARGS(&mPSOs["CompReset"])));
}



void App::BuildFrameResources() {
    for (int i = 0; i < d3dUtil::gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            2,                                          // two main pass cbvs : default main pass & shadowmap main pass
            (UINT)mScene->getObjectInfos().size(),      // this many objects cbvs
            (UINT)mScene->getMaterialMap().size()       // this many material cbvs
            ));
    }
}

void App::DrawScene() {
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage. 
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);
    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(d3dUtil::MAIN_PASS_UNIFORM::MAINPASS_CBV, passCB->GetGPUVirtualAddress());
    mCommandList->SetPipelineState(mPSOs["deferredPost"].Get());
    DrawRenderItems(mCommandList.Get(), mScene->getObjectInfoLayer()[(int)RenderLayer::Gbuffer]);
    mCommandList->SetPipelineState(mPSOs["debug"].Get());
    DrawRenderItems(mCommandList.Get(), mScene->getObjectInfoLayer()[(int)RenderLayer::Debug]);
    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void App::VoxelizeMesh() {

    //mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMeshVoxelizer->getResourcePtr(),
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
    mCommandList->RSSetViewports(1, &mMeshVoxelizer->Viewport());
    mCommandList->RSSetScissorRects(1, &mMeshVoxelizer->ScissorRect());
    mCommandList->OMSetRenderTargets(0, nullptr, false, nullptr);
    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(d3dUtil::MAIN_PASS_UNIFORM::MAINPASS_CBV, passCB->GetGPUVirtualAddress());
    mCommandList->SetPipelineState(mPSOs["voxelizer"].Get());
    DrawRenderItems(mCommandList.Get(), mScene->getObjectInfoLayer()[(int)RenderLayer::Default]);

    //mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMeshVoxelizer->getResourcePtr(),
    //    D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS ));
}

void App::DrawScene2GBuffers() {

    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
    mCommandList->RSSetViewports(1, &mDeferredRenderer->Viewport());
    mCommandList->RSSetScissorRects(1, &mDeferredRenderer->ScissorRect());
    D3D12_CPU_DESCRIPTOR_HANDLE CPUhandleArray[(int)GBUFFER_TYPE::COUNT];
    for (auto& gbuffer : mDeferredRenderer->getGbuffersMap()) {
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gbuffer.second->getResourcePtr(),
            D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
        float clearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        mCommandList->ClearRenderTargetView(gbuffer.second->getCPUHandle4RTV(), clearValue, 0, nullptr);
        CPUhandleArray[(int)gbuffer.second->getType()] = gbuffer.second->getCPUHandle4RTV();
    }
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    mCommandList->OMSetRenderTargets(static_cast<UINT>(GBUFFER_TYPE::COUNT), &CPUhandleArray[0], false, &DepthStencilView());
    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(d3dUtil::MAIN_PASS_UNIFORM::MAINPASS_CBV, passCB->GetGPUVirtualAddress());
    mCommandList->SetPipelineState(mPSOs["deferred"].Get());
    DrawRenderItems(mCommandList.Get(), mScene->getObjectInfoLayer()[(int)RenderLayer::Default]);
    for (auto& gbuffer : mDeferredRenderer->getGbuffersMap()) {
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(gbuffer.second->getResourcePtr(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
    }
}

void App::DrawScene2ShadowMap() {

    mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
    mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

    // Change to DEPTH_WRITE.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->getResourcePtr(),
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
    // Clear the back buffer and depth buffer.
    mCommandList->ClearDepthStencilView(mShadowMap->getCPUHandle4DSV(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    // Set null render target because we are only going to draw to
    // depth buffer.  Setting a null render target will disable color writes.
    // Note the active PSO also must specify a render target count of 0.
    mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->getCPUHandle4DSV());
    // Bind the pass constant buffer for the shadow map pass.
    auto passCB = mCurrFrameResource->PassCB->Resource();
    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
    mCommandList->SetGraphicsRootConstantBufferView(d3dUtil::MAIN_PASS_UNIFORM::MAINPASS_CBV, passCBAddress);
    mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mScene->getObjectInfoLayer()[(int)RenderLayer::Default]);
    // Change back to GENERIC_READ so we can read the texture in a shader.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->getResourcePtr(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void App::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<ObjectInfo*>& objInfos)
{

    // For each obj ...
    for (size_t i = 0; i < objInfos.size(); ++i)
    {
        auto rObject = objInfos[i];

        cmdList->IASetVertexBuffers(0, 1, &rObject->Model->getVertexBufferView());
        cmdList->IASetIndexBuffer(&rObject->Model->getIndexBufferView());
        cmdList->IASetPrimitiveTopology(rObject->PrimitiveType);
        // for each mesh ...
        for (size_t j = 0; j < rObject->mMeshInfos.size(); ++j) {

            auto rMesh = rObject->mMeshInfos[j].get();
            auto rMaterial = mScene->getMaterialMap()[rMesh->Mat].get();


            UINT diffTexIndex = 0;
            UINT objIndex = 0;
            UINT materialIndex = 0;

            if (rMaterial) {
                materialIndex = rMaterial->MatCBIndex;
                diffTexIndex = rMaterial->DiffuseSrvHeapIndex;
            }
            if (rObject) {
                objIndex = rObject->ObjCBIndex;
            }

            cmdList->SetGraphicsRootDescriptorTable(d3dUtil::MAIN_PASS_UNIFORM::DIFFUSE_TEX_TABLE, mSrvHeaps["MainPass"]->mGPUHandle(diffTexIndex));
            cmdList->SetGraphicsRootConstantBufferView(d3dUtil::MAIN_PASS_UNIFORM::OBJ_CBV, mCurrFrameResource->getObjGPUcbAddress(objIndex));
            cmdList->SetGraphicsRootConstantBufferView(d3dUtil::MAIN_PASS_UNIFORM::MATERIAL_CBV, mCurrFrameResource->getMaterialGPUcbAddress(materialIndex));
            cmdList->DrawIndexedInstanced(rMesh->IndexCount, 1, rMesh->StartIndexLocation, rMesh->BaseVertexLocation, 0);
        }
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> App::GetStaticSamplers()
{
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.  

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
        0.0f,                             // mipLODBias
        8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        8);                                // maxAnisotropy

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp };
}
