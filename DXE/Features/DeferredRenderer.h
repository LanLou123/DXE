#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include "Utilities/Utils.h"
#include "Utilities/Timer.h"

enum class GBUFFER_TYPE : int { POSITION = 0, ALBEDO, NORMAL, COUNT };

class GBuffer {
public:

	GBuffer(ID3D12Device* _device, UINT _w, UINT _h, GBUFFER_TYPE _t);

	GBuffer(const GBuffer& rhs) = delete;
	GBuffer& operator=(const GBuffer& rhs) = delete;
	~GBuffer() = default;

	ID3D12Resource* getResourcePtr();
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4SRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle4SRV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle4RTV() const;
	GBUFFER_TYPE getType() const;

	void SetupCPUGPUDescOffsets(
		D3D12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		D3D12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		D3D12_CPU_DESCRIPTOR_HANDLE hCpuRtv
	);

	bool initGBuffer();
	void OnResize(UINT newWidth, UINT newHeight);

private:

	void BuildDescriptors();
	void BuildResources();

private:

	ID3D12Device* device;
	GBUFFER_TYPE Type;
	UINT mWidth = 0;
	UINT mHeight = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUsrv;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUsrv;
	D3D12_CPU_DESCRIPTOR_HANDLE mhCPUrtv;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTexture;
};

class DeferredRenderer {

public:
	DeferredRenderer();
	DeferredRenderer(ID3D12Device* device, UINT width, UINT height);

	void initDeferredRenderer();
	void onResize(UINT newWidth, UINT newHeight);

	DeferredRenderer(const DeferredRenderer& rhs) = delete;
	DeferredRenderer& operator=(const DeferredRenderer& rhs) = delete;
	~DeferredRenderer() = default;

	GBuffer* getGBuffer(GBUFFER_TYPE _type);
	std::unordered_map<GBUFFER_TYPE, std::unique_ptr<GBuffer>>& getGbuffersMap();
	UINT Width()const;
	UINT Height()const;
	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;

private:

	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;

	std::unordered_map<GBUFFER_TYPE, std::unique_ptr<GBuffer>> mGBuffers;

};

#endif // !DEFERRED_RENDERER_H
