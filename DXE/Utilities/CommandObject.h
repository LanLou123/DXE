#ifndef COMMAND_OBJECT_H
#define COMMAND_OBJECT_H

#include "Utils.h"

class CommandObject {
public:

	CommandObject();

	CommandObject(ID3D12Device* _device);

	CommandObject(const CommandObject& rhs) = delete;
	CommandObject& operator=(const CommandObject& rhs) = delete;

	void FlushCommandQueue();

	void endCommandRecording();

	void resetCommandList();

	void initCommandObject();

	ID3D12CommandQueue* getQueuePtr();
	ID3D12CommandQueue** getQueuePPtr();

	ID3D12CommandAllocator* getAllocatorPtr();
	ID3D12CommandAllocator** getAllocatorPPtr();

	ID3D12GraphicsCommandList* getCommandListPtr();
	ID3D12GraphicsCommandList** getCommandListPPtr();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> cpyQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cpyAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cpyCmdList;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	UINT64 fenceValue;
	ID3D12Device* md3dDevice;
};

#endif // !COMMAND_OBJECT_H
