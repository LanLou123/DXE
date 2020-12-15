#include "CommandObject.h"

CommandObject::CommandObject(){}

CommandObject::CommandObject(ID3D12Device* _device) : md3dDevice(_device), fenceValue(0) {
    initCommandObject();
}


void CommandObject::endCommandRecording() {
    cpyCmdList->Close();
    ID3D12CommandList* commandLists[] = { cpyCmdList.Get() };
    cpyQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void CommandObject::resetCommandList() {
    ThrowIfFailed(cpyCmdList->Reset(cpyAllocator.Get(), nullptr));
}

void CommandObject::initCommandObject() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(cpyQueue.GetAddressOf())));
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cpyAllocator.GetAddressOf())));
    ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cpyAllocator.Get(), NULL, IID_PPV_ARGS(cpyCmdList.GetAddressOf())));

    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr) {
        std::cout << "fence event creation failed" << std::endl;
        return;
    }
    fenceValue = 0;


    cpyCmdList->Close();
}

void CommandObject::FlushCommandQueue() {
    fenceValue++;
    ThrowIfFailed(cpyQueue->Signal(fence.Get(), fenceValue));
    if (fence->GetCompletedValue() < fenceValue) {

        HANDLE fenceEventhandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEventhandle));
        WaitForSingleObject(fenceEventhandle, INFINITE);
        CloseHandle(fenceEventhandle);
    }
}

ID3D12CommandQueue* CommandObject::getQueuePtr() {
    return cpyQueue.Get();
}
ID3D12CommandQueue** CommandObject::getQueuePPtr() {
    return cpyQueue.GetAddressOf();
}

ID3D12CommandAllocator* CommandObject::getAllocatorPtr() {
    return cpyAllocator.Get();
}
ID3D12CommandAllocator** CommandObject::getAllocatorPPtr() {
    return cpyAllocator.GetAddressOf();
}

ID3D12GraphicsCommandList* CommandObject::getCommandListPtr() {
    return cpyCmdList.Get();
}
ID3D12GraphicsCommandList** CommandObject::getCommandListPPtr() {
    return cpyCmdList.GetAddressOf();
}