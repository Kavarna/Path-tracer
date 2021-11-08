#pragma once


#include <Oblivion.h>
#include "../GraphicsObject.h"


class QueueManager sealed : public GraphicsObject {
public:
    QueueManager(D3D12_COMMAND_LIST_TYPE queueType, uint32_t nodeMask = 0);
    ~QueueManager();

public:
    uint64_t Signal();
    void WaitForFenceValue(uint64_t fenceValue) const;
    bool IsCompleteFence(uint64_t fenceValue) const;
    void Flush();

    ComPtr<ID3D12CommandQueue> GetQueue() const;

    ComPtr<ID3D12GraphicsCommandList5> GetCommandList(ComPtr<ID3D12PipelineState> pipelineState = nullptr);

    uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList5> commandList);

private:
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator() const;
    ComPtr<ID3D12GraphicsCommandList5> CreateCommandList(ID3D12CommandAllocator* commandAllocator, ComPtr<ID3D12PipelineState> pipelineState) const;

private:
    uint32_t mNodeMask;

    ComPtr<ID3D12CommandQueue> mQueue;
    ComPtr<ID3D12Fence1> mFence;

    uint64_t mFenceValue;

    struct CommandAllocatorEntry {
        uint64_t fenceValue;
        ComPtr<ID3D12CommandAllocator> allocator;

        CommandAllocatorEntry() = default;

        CommandAllocatorEntry(uint64_t fenceValue, ComPtr<ID3D12CommandAllocator> allocator) :
            fenceValue(fenceValue), allocator(allocator) { };

        bool operator < (const CommandAllocatorEntry& rhs) {
            return fenceValue < rhs.fenceValue;
        }
    };

    std::queue<CommandAllocatorEntry> mCommandAllocatorEntries;
    std::queue<ComPtr<ID3D12GraphicsCommandList5>> mCommandLists;

    D3D12_COMMAND_LIST_TYPE mQueueType;
};

