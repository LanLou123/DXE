#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "Utilities/Utils.h"
#include "Utilities/Timer.h"

using namespace DirectX;

struct ShadowMapData {
	//shadow map
	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = MathUtils::Identity4x4();
	XMFLOAT4X4 mLightProj = MathUtils::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathUtils::Identity4x4();
	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.2f, -0.4f, 0.1f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 mRotatedLightDirections[3];
};




class ShadowMap {

public:

	ShadowMap();
	ShadowMap(ID3D12Device* device, UINT width, UINT height);

	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;
	~ShadowMap() = default;

	UINT Width()const;
	UINT Height()const;
	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;
	ID3D12Resource* getResourcePtr();
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4SRV()const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4DSV()const;



	void SetupCPUGPUDescOffsets(
		D3D12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		D3D12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		D3D12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

	void OnResize(UINT newWidth, UINT newHeight);
	void Update(const Timer& gt);

	bool initShadowMap();

	ShadowMapData mShadowMapData;

private:

	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUSrv;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUSrv;
	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;
};


#endif // !SHADOW_MAP_H
