#include "ShadowMap.h"

ShadowMap::ShadowMap(){}

ShadowMap::ShadowMap(ID3D12Device* device, UINT width, UINT height)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };


}

bool ShadowMap::initShadowMap() {
	BuildResource();
	return true;
}

UINT ShadowMap::Width()const
{
	return mWidth;
}

UINT ShadowMap::Height()const
{
	return mHeight;
}

ID3D12Resource* ShadowMap::getResourcePtr()
{
	return mShadowMap.Get();
}

D3D12_GPU_DESCRIPTOR_HANDLE ShadowMap::getGPUHandle4SRV()const
{
	return mhGPUSrv;
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowMap::getCPUHandle4DSV()const
{
	return mhCPUDsv;
}

D3D12_VIEWPORT ShadowMap::Viewport()const
{
	return mViewport;
}

D3D12_RECT ShadowMap::ScissorRect()const
{
	return mScissorRect;
}

void ShadowMap::SetupCPUGPUDescOffsets(D3D12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	D3D12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	D3D12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
	// Save references to the descriptors. 
	mhCPUSrv = hCpuSrv;
	mhGPUSrv = hGpuSrv;
	mhCPUDsv = hCpuDsv;

	//  Create the descriptors
	BuildDescriptors();
}

void ShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		mViewport = { 0.0f, 0.0f, (float)mWidth, (float)mHeight, 0.0f, 1.0f };
		mScissorRect = { 0, 0, (int)mWidth, (int)mHeight };

		BuildResource();

		// New resource, so we need new descriptors to that resource.
		BuildDescriptors();
	}
}

void ShadowMap::BuildDescriptors()
{
	// Create SRV to resource so we can sample the shadow map in a shader program.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	md3dDevice->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, mhCPUSrv);

	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mhCPUDsv);
}

void ShadowMap::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mShadowMap)));
}

void ShadowMap::setShadowLightPos(float x, float y, float z, const Timer& gt) {
	mShadowMapData.mRotatedLightDirections = XMFLOAT3(x, y, z);
	Update(gt);
}

void ShadowMap::Update(const Timer& gt) {
	// update light directions
	//mShadowMapData.mLightRotationAngle += 0.06f * gt.DeltaTime();
	//XMMATRIX R = DirectX::XMMatrixRotationY(mShadowMapData.mLightRotationAngle);

	//XMVECTOR lightDir = XMLoadFloat3(&mShadowMapData.mLightPos);
	//lightDir = DirectX::XMVector3TransformNormal(lightDir, R);
	//DirectX::XMStoreFloat3(&mShadowMapData.mRotatedLightDirections, lightDir);
	
	//mShadowMapData.mRotatedLightDirections[0] = DirectX::XMFLOAT3(cos(mShadowMapData.mLightRotationAngle) * 2.0, -0.5, 0.0 );
	// Only the first "main" light casts a shadow.
	XMFLOAT3 sceneCenter(0.0f, 0.0f, 0.0f);
	float sceneRadius = 250.f;

	XMVECTOR lightDir = DirectX::XMLoadFloat3(&mShadowMapData.mRotatedLightDirections);
	lightDir = DirectX::XMVector3Normalize(lightDir);
	XMVECTOR lightPos = -2.0f * sceneRadius * lightDir;
	XMVECTOR targetPos = DirectX::XMLoadFloat3(&sceneCenter);
	XMVECTOR lightUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = DirectX::XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	DirectX::XMStoreFloat3(&mShadowMapData.mLightPosW, lightPos);

	XMFLOAT3 sphereCenterLS;
	DirectX::XMStoreFloat3(&sphereCenterLS, DirectX::XMVector3TransformCoord(targetPos, lightView));

	float depth_scale = 1.0;
	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - sceneRadius;
	float b = sphereCenterLS.y - sceneRadius;
	float n = sphereCenterLS.z - sceneRadius * depth_scale;
	float r = sphereCenterLS.x + sceneRadius;
	float t = sphereCenterLS.y + sceneRadius;
	float f = sphereCenterLS.z + sceneRadius * depth_scale;

	mShadowMapData.mLightNearZ = n;
	mShadowMapData.mLightFarZ = f;
	XMMATRIX lightProj = DirectX::XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
	//lightProj = DirectX::XMMatrixOrthographicLH(200, 200, 0, 1000);
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	XMMATRIX S = lightView * lightProj * T;
	DirectX::XMStoreFloat4x4(&mShadowMapData.mLightView, lightView);
	DirectX::XMStoreFloat4x4(&mShadowMapData.mLightProj, lightProj);
	DirectX::XMStoreFloat4x4(&mShadowMapData.mShadowTransform, S);
}