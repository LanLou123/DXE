#ifndef MESH_VOXELIZER_H
#define MESH_VOXELIZER_H

#include "Utilities/Utils.h"
#include "Utilities/Timer.h"
#include "Utilities/DescriptorHeap.h"

struct MeshVoxelizerData {
	DirectX::XMFLOAT4X4 mVoxelView = MathUtils::Identity4x4();
	DirectX::XMFLOAT4X4 mVoxelProj = MathUtils::Identity4x4();

};

struct MipVoxelData {
	MipVoxelData(int _dim) : mipDim(_dim) {}
	int mipDim;
};

enum class VOLUME_TEXTURE_TYPE : int { ALBEDO = 0, NORMAL, EMISSIVE, RADIANCE, FLAG, COUNT };

class VolumeTexture {
public:
	VolumeTexture(ID3D12Device* _device, UINT _x, UINT _y, UINT _z);
	VolumeTexture(const VolumeTexture& rhs) = delete;
	VolumeTexture& operator=(const VolumeTexture& rhs) = delete;
	~VolumeTexture() = default;

	ID3D12Resource* getResourcePtr();
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4UAV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4UAV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4SRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4SRV() const;
	UINT getNumDescriptors();
	void SetupUAVCPUGPUDescOffsets(
		D3D12_CPU_DESCRIPTOR_HANDLE hCPUUav,
		D3D12_GPU_DESCRIPTOR_HANDLE hGPUUav
	);
	void SetupSRVCPUGPUDescOffsets(
		D3D12_CPU_DESCRIPTOR_HANDLE hCPUSrv,
		D3D12_GPU_DESCRIPTOR_HANDLE hGPUSrv
	);

	void Init();
	void OnResize(UINT newX, UINT newY, UINT newZ);
	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;


private:

	void BuildUAVDescriptors();
	void BuildSRVDescriptors();
	void BuildResources();


protected:

	ID3D12Device* device;
	UINT mX, mY, mZ;
	UINT mNumDescriptors;


	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUuav;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUuav;
	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUsrv;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUsrv;

	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	DXGI_FORMAT mSRVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mUAVFormat = DXGI_FORMAT_R32_UINT;
	Microsoft::WRL::ComPtr<ID3D12Resource> m3DTexture;
	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;
};

class RadianceMipMapedVolumeTexture {
public:

	RadianceMipMapedVolumeTexture(ID3D12Device* _device, UINT _x, UINT _y, UINT _z);
	RadianceMipMapedVolumeTexture(const RadianceMipMapedVolumeTexture& rhs) = delete;
	RadianceMipMapedVolumeTexture& operator=(const RadianceMipMapedVolumeTexture& rhs) = delete;
	~RadianceMipMapedVolumeTexture() = default;

	// getters 
	ID3D12Resource* getResourcePtr();
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4UAV(int mipLevel) const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4UAV(int mipLevel) const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4SRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4SRV() const;
	UINT getNumDescriptors();
	UINT getNumMipLevels();

	// setters
	void SetupUAVCPUGPUDescOffsets(mDescriptorHeap* heapPtr);
	void SetupSRVCPUGPUDescOffsets(mDescriptorHeap* heapPtr);


	void Init();
	void OnResize(UINT newX, UINT newY, UINT newZ);

private:

	void BuildUAVDescriptors();
	void BuildSRVDescriptors();
	void BuildResources();

private:

	ID3D12Device* device;
	UINT mX, mY, mZ;
	UINT mNumDescriptors;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mhCPUuavs;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> mhGPUuavs;
	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUsrv;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUsrv;


	UINT mNumMipLevels;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	DXGI_FORMAT mSRVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mUAVFormat = DXGI_FORMAT_R32_UINT;
	Microsoft::WRL::ComPtr<ID3D12Resource> m3DTexture;

};

class MeshVoxelizer {
public:
	MeshVoxelizer(ID3D12Device* _device, UINT _x, UINT _y, UINT _z);
	MeshVoxelizer(const MeshVoxelizer& rhs) = delete;
	MeshVoxelizer& operator=(const MeshVoxelizer& rhs) = delete;
	~MeshVoxelizer() = default;

	void initVoxelizer();
	void OnResize(UINT newX, UINT newY, UINT newZ);
	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;

	MeshVoxelizerData& getUniformData();
	UINT getNumDescriptors();
	UINT getDimensionX();
	UINT getDimensionY();
	UINT getDimensionZ();
	VolumeTexture* getVolumeTexture(VOLUME_TEXTURE_TYPE _type);
	RadianceMipMapedVolumeTexture* getRadianceMipMapedVolumeTexture();
	std::unordered_map<VOLUME_TEXTURE_TYPE, std::unique_ptr<VolumeTexture>>& getVoxelTexturesMap();


	void Clear3DTexture(ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSig,
		ID3D12PipelineState* pso);


	void setUpDescriptors4Voxels(mDescriptorHeap* heapPtr);
	void setUpDescriptors4RadianceMipMaped(mDescriptorHeap* heapPtr);




private:

	void PopulateUniformData();

private:
	ID3D12Device* device;
	UINT mX, mY, mZ;
	UINT mNumDescriptors;

	std::unordered_map<VOLUME_TEXTURE_TYPE, std::unique_ptr<VolumeTexture>> mVolumeTextures;
	std::unique_ptr<RadianceMipMapedVolumeTexture> mRadianceMipMapedTexture;

	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;
	MeshVoxelizerData mData;
};

#endif // !MESH_VOXLIZER_H
