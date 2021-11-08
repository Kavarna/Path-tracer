#include "QueueManager.h"

QueueManager::QueueManager(D3D12_COMMAND_LIST_TYPE queueType, uint32_t nodeMask) :
    mFenceValue(0), mNodeMask(nodeMask), mQueueType(queueType) {

    D3D12_COMMAND_QUEUE_DESC queueInfo = {  };
    queueInfo.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueInfo.NodeMask = mNodeMask;
    queueInfo.Priority = 0;
    queueInfo.Type = queueType;

    ThrowIfFailed(mDevice->CreateCommandQueue(&queueInfo, IID_PPV_ARGS(&mQueue)));
    ThrowIfFailed(mDevice->CreateFence(mFenceValue, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

QueueManager::~QueueManager() {
    Flush();
}

uint64_t QueueManager::Signal() {
    uint64_t fenceValue = ++mFenceValue;
    mQueue->Signal(mFence.Get(), fenceValue);
    return fenceValue;
}

void QueueManager::WaitForFenceValue(uint64_t fenceValue) const {

    if (!IsCompleteFence(fenceValue)) {
        HANDLE eventToWait = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        EVALUATE(eventToWait != nullptr , "Unable to create event");
        mFence->SetEventOnCompletion(fenceValue, eventToWait);
        WaitForSingleObject(eventToWait, INFINITE);
    }

}

bool QueueManager::IsCompleteFence(uint64_t fenceValue) const {
    return mFence->GetCompletedValue() >= fenceValue;
}

void QueueManager::Flush() {
    WaitForFenceValue(Signal());
}

ComPtr<ID3D12CommandQueue> QueueManager::GetQueue() const {
    return mQueue;
}

ComPtr<ID3D12GraphicsCommandList5> QueueManager::GetCommandList(ComPtr<ID3D12PipelineState> pipelineState) {
    ComPtr<ID3D12GraphicsCommandList5> commandList;
    ComPtr<ID3D12CommandAllocator> commandAllocator;

    if (!mCommandAllocatorEntries.empty() && IsCompleteFence(mCommandAllocatorEntries.front().fenceValue)) {
        commandAllocator = mCommandAllocatorEntries.front().allocator;
        mCommandAllocatorEntries.pop();

        ThrowIfFailed(commandAllocator->Reset());
    } else {
        commandAllocator = CreateCommandAllocator();
    }

    if (!mCommandLists.empty()) {
        commandList = mCommandLists.front();
        mCommandLists.pop();

        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));
    } else {
        commandList = CreateCommandList(commandAllocator.Get(), pipelineState);
    }

    ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

    return commandList;
}

uint64_t QueueManager::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList5> commandList) {
    
    ThrowIfFailed(commandList->Close());

    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator);

    ID3D12CommandList* cmdLists[] = {
        commandList.Get()
    };

    mQueue->ExecuteCommandLists(ARRAYSIZE(cmdLists), cmdLists);

    auto fenceValue = Signal();

    mCommandAllocatorEntries.emplace(fenceValue, commandAllocator);
    mCommandLists.push(commandList);

    commandAllocator->Release();
    return fenceValue;
}

ComPtr<ID3D12CommandAllocator> QueueManager::CreateCommandAllocator() const {
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(mDevice->CreateCommandAllocator(mQueueType, IID_PPV_ARGS(&commandAllocator)));
    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList5> QueueManager::CreateCommandList(ID3D12CommandAllocator* commandAllocator, ComPtr<ID3D12PipelineState> pipelineState) const {
    ComPtr<ID3D12GraphicsCommandList5> commandList;
    ThrowIfFailed(mDevice->CreateCommandList(mNodeMask, mQueueType, commandAllocator, pipelineState.Get(),
                                             IID_PPV_ARGS(&commandList)));
    return commandList;
}

