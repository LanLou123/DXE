#ifndef MESH_VOXELIZER_H
#define MESH_VOXELIZER_H

#include "Utilities/Utils.h"
#include "Utilities/Timer.h"

struct MeshVoxelizerData {
	DirectX::XMFLOAT4X4 mVoxelView = MathUtils::Identity4x4();
	DirectX::XMFLOAT4X4 mVoxelProj = MathUtils::Identity4x4();

};

class MeshVoxelizer {
public:
	MeshVoxelizer(ID3D12Device* _device, UINT _x, UINT _y, UINT _z);
	MeshVoxelizer(const MeshVoxelizer& rhs) = delete;
	MeshVoxelizer& operator=(const MeshVoxelizer& rhs) = delete;
	~MeshVoxelizer() = default;

	ID3D12Resource* getResourcePtr();
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4SRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4SRV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4UAV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4UAV() const;

	void SetupCPUGPUDescOffsets(
		D3D12_CPU_DESCRIPTOR_HANDLE hCPUSrv,
		D3D12_GPU_DESCRIPTOR_HANDLE hGPUSrv,
		D3D12_CPU_DESCRIPTOR_HANDLE hCPUUav,
		D3D12_GPU_DESCRIPTOR_HANDLE hGPUUav
	);

	void init3DVoxelTexture();
	void OnResize(UINT newX, UINT newY, UINT newZ);
	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;
	MeshVoxelizerData& getUniformData();

private:

	void BuildDescriptors();
	void BuildResources();
	void PopulateUniformData();

private:
	ID3D12Device* device;
	UINT mX, mY, mZ;
	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUsrv;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUsrv;
	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUuav;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUuav;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	Microsoft::WRL::ComPtr<ID3D12Resource> m3DTexture;
	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;
	MeshVoxelizerData mData;
};

#endif // !MESH_VOXLIZER_H
