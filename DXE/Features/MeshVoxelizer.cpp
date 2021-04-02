#include "MeshVoxelizer.h"

MeshVoxelizer::MeshVoxelizer(ID3D12Device* _device, UINT _x, UINT _y, UINT _z):device(_device), mX(_x), mY(_y), mZ(_z) {
	mViewPort = { 0.0f, 0.0f, (float)mX, (float)mY, 0.0f, 1.0f };
	mScissorRect = { 0,0,(int)mX, (int)mY };
	PopulateUniformData();
}

ID3D12Resource* MeshVoxelizer::getResourcePtr() {
	return m3DTexture.Get();
}
D3D12_CPU_DESCRIPTOR_HANDLE MeshVoxelizer::getCPUHandle4SRV() const {
	return mhCPUsrv;
}
D3D12_GPU_DESCRIPTOR_HANDLE MeshVoxelizer::getGPUHandle4SRV() const {
	return mhGPUsrv;
}
D3D12_CPU_DESCRIPTOR_HANDLE MeshVoxelizer::getCPUHandle4UAV() const {
	return mhCPUuav;
}
D3D12_GPU_DESCRIPTOR_HANDLE MeshVoxelizer::getGPUHandle4UAV() const {
	return mhGPUuav;
}

void MeshVoxelizer::SetupCPUGPUDescOffsets(
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUSrv,
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUSrv,
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUUav,
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUUav
) {
	mhCPUsrv = hCPUSrv;
	mhGPUsrv = hGPUSrv;
	mhCPUuav = hCPUUav;
	mhGPUuav = hGPUUav;
	BuildDescriptors();
}

void MeshVoxelizer::init3DVoxelTexture() {
	BuildResources();
}

void MeshVoxelizer::OnResize(UINT newX, UINT newY, UINT newZ) {
	if ((mX != newX) || (mY != newY) || (mZ != newZ)) {
		mX = newX;
		mY = newY;
		mZ = newZ;
		mViewPort = { 0.0f, 0.0f, (float)mX, (float)mY, 0.0f, 1.0f };
		mScissorRect = { 0,0,(int)mX, (int)mY };
		BuildResources();
		BuildDescriptors();
	}
}

void MeshVoxelizer::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MostDetailedMip = 0;
	srvDesc.Texture3D.MipLevels = 1;
	srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(m3DTexture.Get(), &srvDesc, mhCPUsrv);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = mFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = -1;
	device->CreateUnorderedAccessView(m3DTexture.Get(), nullptr, &uavDesc, mhCPUuav);
}

void MeshVoxelizer::BuildResources() {
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


void MeshVoxelizer::Clear3DTexture(ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* rootSig,
	ID3D12PipelineState* pso) {
	cmdList->SetPipelineState(pso);
	cmdList->SetComputeRootSignature(rootSig);
	cmdList->SetComputeRootDescriptorTable(0, getGPUHandle4UAV());
	cmdList->Dispatch(mX / 8.0, mY / 8.0, mZ / 8.0);
}

MeshVoxelizerData& MeshVoxelizer::getUniformData() {
	return mData;
}