#include "MeshVoxelizer.h"

RadianceMipMapedVolumeTexture::RadianceMipMapedVolumeTexture(ID3D12Device* _device, UINT _x, UINT _y, UINT _z) : device(_device), mX(_x), mY(_y), mZ(_z) {
	mNumMipLevels = d3dUtil::GetNumMipmaps(mX / 2, mY / 2, mZ / 2);
	mNumDescriptors = 2 * mNumMipLevels;

}

void RadianceMipMapedVolumeTexture::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	texDesc.Alignment = 0;
	texDesc.Width = (mX / 2) * 6; // each anistropic mipmaping voxel needs 6 directions in total
	texDesc.Height = mY / 2;
	texDesc.DepthOrArraySize = mZ / 2;
	texDesc.MipLevels = mNumMipLevels;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m3DTexture.GetAddressOf())
	));
}

void RadianceMipMapedVolumeTexture::BuildSRVDescriptors() {
	for (int i = 0; i < mNumMipLevels; ++i)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mSRVFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = i;
		srvDesc.Texture3D.MipLevels = 1;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		device->CreateShaderResourceView(m3DTexture.Get(), &srvDesc, mhCPUsrvs[i]);
	}
}

void RadianceMipMapedVolumeTexture::BuildUAVDescriptors() {
	for (int i = 0; i < mNumMipLevels; ++i) {
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = mUAVFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.MipSlice = i;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = -1;
		device->CreateUnorderedAccessView(m3DTexture.Get(), nullptr, &uavDesc, mhCPUuavs[i]);
	}
}

ID3D12Resource* RadianceMipMapedVolumeTexture::getResourcePtr() {
	return m3DTexture.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE RadianceMipMapedVolumeTexture::getCPUHandle4UAV(int mipLevel) const {
	return mhCPUuavs[mipLevel];
}

D3D12_GPU_DESCRIPTOR_HANDLE RadianceMipMapedVolumeTexture::getGPUHandle4UAV(int mipLevel) const {
	return mhGPUuavs[mipLevel];
}

UINT RadianceMipMapedVolumeTexture::getNumDescriptors() {
	return mNumDescriptors;
}

void RadianceMipMapedVolumeTexture::SetupUAVCPUGPUDescOffsets(mDescriptorHeap* heapPtr) {
	for (int i = 0; i < mNumMipLevels; ++i) {
		auto curCPUHandle = heapPtr->mCPUHandle(heapPtr->getCurrentOffsetRef());
		auto curGPUHandle = heapPtr->mGPUHandle(heapPtr->getCurrentOffsetRef());
		mhCPUuavs.push_back(curCPUHandle);
		mhGPUuavs.push_back(curGPUHandle);
		heapPtr->incrementCurrentOffset();
	}
	BuildUAVDescriptors();
}

void RadianceMipMapedVolumeTexture::SetupSRVCPUGPUDescOffsets(mDescriptorHeap* heapPtr) {
	for (int i = 0; i < mNumMipLevels; ++i) {
		auto curCPUHandle = heapPtr->mCPUHandle(heapPtr->getCurrentOffsetRef());
		auto curGPUHandle = heapPtr->mGPUHandle(heapPtr->getCurrentOffsetRef());
		mhCPUsrvs.push_back(curCPUHandle);
		mhGPUsrvs.push_back(curGPUHandle);
		heapPtr->incrementCurrentOffset();
	}
	BuildSRVDescriptors();
}

void RadianceMipMapedVolumeTexture::Init() {
	BuildResources();
}
void RadianceMipMapedVolumeTexture::OnResize(UINT newX, UINT newY, UINT newZ) {
	if ((mX != newX) || (mY != newY) || (mZ != newZ)) {
		mX = newX;
		mY = newY;
		mZ = newZ;

		BuildResources();
		BuildUAVDescriptors();
	}
}



VolumeTexture::VolumeTexture(ID3D12Device* _device, UINT _x, UINT _y, UINT _z) :device(_device), mX(_x), mY(_y), mZ(_z) {
	mNumDescriptors = 2;
	
	mViewPort = { 0.0f, 0.0f, (float)mX, (float)mY, 0.0f, 1.0f };
	mScissorRect = { 0,0,(int)mX, (int)mY };
}

void VolumeTexture::Init() {
	BuildResources();
}

ID3D12Resource* VolumeTexture::getResourcePtr() {
	return m3DTexture.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE VolumeTexture::getCPUHandle4UAV() const {
	return mhCPUuav;
}
D3D12_GPU_DESCRIPTOR_HANDLE VolumeTexture::getGPUHandle4UAV() const {
	return mhGPUuav;
}

UINT VolumeTexture::getNumDescriptors() {
	return mNumDescriptors;
}

void VolumeTexture::SetupUAVCPUGPUDescOffsets(
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUUav,
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUUav
) {

	mhCPUuav = hCPUUav;
	mhGPUuav = hGPUUav;
	BuildUAVDescriptors();
}

void VolumeTexture::SetupSRVCPUGPUDescOffsets(
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUSrv,
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUSrv
) {
	mhCPUsrv = hCPUSrv;
	mhGPUsrv = hGPUSrv;
	BuildSRVDescriptors();
}

void VolumeTexture::OnResize(UINT newX, UINT newY, UINT newZ) {
	if ((mX != newX) || (mY != newY) || (mZ != newZ)) {
		mX = newX;
		mY = newY;
		mZ = newZ;
		mViewPort = { 0.0f, 0.0f, (float)mX, (float)mY, 0.0f, 1.0f };
		mScissorRect = { 0,0,(int)mX, (int)mY };
		BuildResources();
		BuildUAVDescriptors();
	}
}

void VolumeTexture::BuildSRVDescriptors() {

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mSRVFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MostDetailedMip = 0;
	srvDesc.Texture3D.MipLevels = 1;
	srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(m3DTexture.Get(), &srvDesc, mhCPUsrv);

}

void VolumeTexture::BuildUAVDescriptors() {

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = mUAVFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = -1;
	device->CreateUnorderedAccessView(m3DTexture.Get(), nullptr, &uavDesc, mhCPUuav);
}

void VolumeTexture::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	texDesc.Alignment = 0;
	texDesc.Width = mX;
	texDesc.Height = mY;
	texDesc.DepthOrArraySize = mZ;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m3DTexture.GetAddressOf())
	));
}

D3D12_VIEWPORT VolumeTexture::Viewport()const {
	return mViewPort;
}

D3D12_RECT VolumeTexture::ScissorRect()const {
	return mScissorRect;
}

MeshVoxelizer::MeshVoxelizer(ID3D12Device* _device, UINT _x, UINT _y, UINT _z):device(_device), mX(_x), mY(_y), mZ(_z) {
	mNumDescriptors = 0; 
	mViewPort = { 0.0f, 0.0f, (float)mX, (float)mY, 0.0f, 1.0f };
	mScissorRect = { 0,0,(int)mX, (int)mY };
	PopulateUniformData();
}


void MeshVoxelizer::initVoxelizer() {

	auto V_Albedo = std::make_unique<VolumeTexture>(device, mX, mY, mZ);
	auto V_Normal = std::make_unique<VolumeTexture>(device, mX, mY, mZ);
	auto V_Emissive = std::make_unique<VolumeTexture>(device, mX, mY, mZ);
	auto V_Radiance = std::make_unique<VolumeTexture>(device, mX, mY, mZ);
	mVolumeTextures[VOLUME_TEXTURE_TYPE::ALBEDO] = std::move(V_Albedo);
	mVolumeTextures[VOLUME_TEXTURE_TYPE::NORMAL] = std::move(V_Normal);
	mVolumeTextures[VOLUME_TEXTURE_TYPE::EMISSIVE] = std::move(V_Emissive);
	mVolumeTextures[VOLUME_TEXTURE_TYPE::RADIANCE] = std::move(V_Radiance);

	for (auto& m : mVolumeTextures) {
		m.second->Init();
		mNumDescriptors += m.second->getNumDescriptors();
	}

	mRadianceMipMapedTexture = std::make_unique<RadianceMipMapedVolumeTexture>(device, mX, mY, mZ);
	mRadianceMipMapedTexture->Init();
	mNumDescriptors += mRadianceMipMapedTexture->getNumDescriptors();
}

RadianceMipMapedVolumeTexture* MeshVoxelizer::getRadianceMipMapedVolumeTexture() {
	return mRadianceMipMapedTexture.get();
}

void MeshVoxelizer::OnResize(UINT newX, UINT newY, UINT newZ) {
	for (auto& m : mVolumeTextures) {
		m.second->OnResize(newX, newY, newZ);
	}
}

VolumeTexture* MeshVoxelizer::getVolumeTexture(VOLUME_TEXTURE_TYPE _type) {
	return mVolumeTextures[_type].get();
}

std::unordered_map<VOLUME_TEXTURE_TYPE, std::unique_ptr<VolumeTexture>>& MeshVoxelizer::getVoxelTexturesMap() {
	return mVolumeTextures;
}

D3D12_VIEWPORT MeshVoxelizer::Viewport()const {
	return mViewPort;
}

D3D12_RECT MeshVoxelizer::ScissorRect()const {
	return mScissorRect;
}

void MeshVoxelizer::PopulateUniformData() {
	float sceneRadius = 300.0f;
	DirectX::XMFLOAT3 eyePos = DirectX::XMFLOAT3(0.0f, sceneRadius / 2.0f, -sceneRadius);
	DirectX::XMFLOAT3 tartPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);


	DirectX::XMMATRIX VoxelView = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&eyePos), DirectX::XMLoadFloat3(&tartPos),
		DirectX::XMLoadFloat3(&up));
	DirectX::XMMATRIX VoxelProj = DirectX::XMMatrixOrthographicLH(sceneRadius, sceneRadius, 0.0f, 1000.0f);

	DirectX::XMStoreFloat4x4(&mData.mVoxelView, VoxelView);
	DirectX::XMStoreFloat4x4(&mData.mVoxelProj, VoxelProj);
}

void MeshVoxelizer::setUpDescriptors4Voxels(mDescriptorHeap* heapPtr) {

	for (auto& voxelTex : getVoxelTexturesMap()) {
		auto meshVoxelizerCPUUavHandle = heapPtr->mCPUHandle(heapPtr->getCurrentOffsetRef());
		auto meshVoxelizerGPUUavHandle = heapPtr->mGPUHandle(heapPtr->getCurrentOffsetRef());
		heapPtr->incrementCurrentOffset();
		voxelTex.second->SetupUAVCPUGPUDescOffsets(meshVoxelizerCPUUavHandle, meshVoxelizerGPUUavHandle);
	}
	for (auto& voxelTex : getVoxelTexturesMap()) {
		auto meshVoxelizerCPUUavHandle = heapPtr->mCPUHandle(heapPtr->getCurrentOffsetRef());
		auto meshVoxelizerGPUUavHandle = heapPtr->mGPUHandle(heapPtr->getCurrentOffsetRef());
		heapPtr->incrementCurrentOffset();
		voxelTex.second->SetupSRVCPUGPUDescOffsets(meshVoxelizerCPUUavHandle, meshVoxelizerGPUUavHandle);
	}

}

void MeshVoxelizer::setUpDescriptors4RadianceMipMaped(mDescriptorHeap* heapPtr) {
	mRadianceMipMapedTexture->SetupUAVCPUGPUDescOffsets(heapPtr);
	mRadianceMipMapedTexture->SetupSRVCPUGPUDescOffsets(heapPtr);
}

void MeshVoxelizer::Clear3DTexture(
	ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* rootSig,
	ID3D12PipelineState* pso) {

	cmdList->SetPipelineState(pso);
	cmdList->SetComputeRootSignature(rootSig);
	cmdList->SetComputeRootDescriptorTable(0, mVolumeTextures[VOLUME_TEXTURE_TYPE::ALBEDO]->getGPUHandle4UAV());

	cmdList->Dispatch(mX / 8.0, mY / 8.0, mZ / 8.0);

	std::vector<D3D12_RESOURCE_BARRIER> barriers;
	for (auto& m : mVolumeTextures) {
		barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(getVolumeTexture(m.first)->getResourcePtr()));
	}
	cmdList->ResourceBarrier(barriers.size(), barriers.data());

}


UINT MeshVoxelizer::getDimensionX() {
	return mX;
}

UINT MeshVoxelizer::getDimensionY() {
	return mY;
}
UINT MeshVoxelizer::getDimensionZ() {
	return mZ;
}

UINT MeshVoxelizer::getNumDescriptors() {
	return mNumDescriptors;
}


MeshVoxelizerData& MeshVoxelizer::getUniformData() {
	return mData;
}