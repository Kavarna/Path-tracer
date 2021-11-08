#pragma once


#include <Oblivion.h>
#include "Utils/QueueManager.h"


class Direct3D : public ISingletone<Direct3D> {
    friend class GraphicsObject;
    MAKE_SINGLETONE_CAPABLE(Direct3D);
private:
    Direct3D();
    ~Direct3D();

public:
    bool HasTearingSupport();
    unsigned int GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType);


    ComPtr<IDXGISwapChain4> CreateSwapchain(HWND hWnd, unsigned int bufferCount, bool tearingSupport, IUnknown* queueToExecute);
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    ComPtr<ID3D12RootSignature> CreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc);
    ComPtr<ID3D12PipelineState> CreateGraphicsPipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineState);
    ComPtr<ID3D12PipelineState> CreateComputePipeline(D3D12_COMPUTE_PIPELINE_STATE_DESC& pipelineState);

    void TransitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_STATES finalState);

public: // Getters
    inline ID3D12Device6* GetD3D12Device() { return mDevice.Get(); };

private:
    ComPtr<IDXGIFactory7> CreateFactory();
    ComPtr<IDXGIAdapter4> CreateAdapter(bool useWarp = false);
    ComPtr<ID3D12Device6> CreateDevice();

private:
    void EnableDebugLayer();

private:
    ComPtr<IDXGIFactory7> mFactory;
    ComPtr<IDXGIAdapter4> mAdapter;
    ComPtr<ID3D12Device6> mDevice;
};

