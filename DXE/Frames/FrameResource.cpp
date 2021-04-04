#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT radianceCount)
{

    objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
    RadianceCB = std::make_unique<UploadBuffer<RadianceConstants>>(device, radianceCount, true);
}

D3D12_GPU_VIRTUAL_ADDRESS FrameResource::getMaterialGPUcbAddress(UINT materialIDX) const {
    D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = MaterialCB->Resource()->GetGPUVirtualAddress() + materialIDX * matCBByteSize;
    return matCBAddress;
}
D3D12_GPU_VIRTUAL_ADDRESS FrameResource::getObjGPUcbAddress(UINT objIDX) const {
    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = ObjectCB->Resource()->GetGPUVirtualAddress() + objIDX * objCBByteSize;
    return objCBAddress;
}

FrameResource::~FrameResource()
{

}