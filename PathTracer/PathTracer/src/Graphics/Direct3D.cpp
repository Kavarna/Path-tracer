#include "Direct3D.h"
#include "Utils/ImagingFactory.h"

Direct3D::Direct3D() {

    ThrowIfFailed(CoInitialize(nullptr));

#if DEBUG || _DEBUG
    EnableDebugLayer();
#endif
    mFactory = CreateFactory();
    mAdapter = CreateAdapter();
    mDevice = CreateDevice();

    ImagingFactory::Get();
}

Direct3D::~Direct3D() {
    ImagingFactory::Reset();
    CoUninitialize();
}

bool Direct3D::HasTearingSupport() {
    BOOL allowTearing;
    ThrowIfFailed(mFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)));
    return allowTearing != FALSE;
}

unsigned int Direct3D::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType) {
    return mDevice->GetDescriptorHandleIncrementSize(heapType);
}

ComPtr<IDXGIFactory7> Direct3D::CreateFactory() {
    ComPtr<IDXGIFactory7> factory7;

    int factoryFlags = 0;
#if DEBUG || _DEBUG
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory7)));

    return factory7;
}

ComPtr<IDXGIAdapter4> Direct3D::CreateAdapter(bool useWarp) {
    ComPtr<IDXGIAdapter1> adapter1;
    
    if (useWarp) {
        mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1));
    } else {
        unsigned int numAdapter = 0;
        ComPtr<IDXGIAdapter1> currentAdapter;
        SIZE_T maxDedicatedVideoMemory = 0;
        while (SUCCEEDED(mFactory->EnumAdapters1(numAdapter++, &currentAdapter))) {
            DXGI_ADAPTER_DESC1 adapter1Desc;
            currentAdapter->GetDesc1(&adapter1Desc);

            if (adapter1Desc.DedicatedVideoMemory > maxDedicatedVideoMemory) {
                adapter1 = currentAdapter;
                maxDedicatedVideoMemory = adapter1Desc.DedicatedSystemMemory;
            }
        }
    }

    ComPtr<IDXGIAdapter4> adapter4;
    adapter1.As(&adapter4);
    return adapter4;
}

ComPtr<ID3D12Device6> Direct3D::CreateDevice() {
    ComPtr<ID3D12Device6> result;

    ThrowIfFailed(D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&result)));
    NAME_D3D12_OBJECT(result);
#if DEBUG || _DEBUG

    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(result.As(&infoQueue))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    }

#endif

    return result;
}


ComPtr<IDXGISwapChain4> Direct3D::CreateSwapchain(HWND hWnd, unsigned int bufferCount, bool tearingSupport, IUnknown* queueToExecute) {
    ComPtr<IDXGISwapChain1> swapchain1;
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchainDesc.BufferCount = bufferCount;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.SampleDesc = { 1,0 };
    swapchainDesc.Scaling = DXGI_SCALING::DXGI_SCALING_NONE;
    swapchainDesc.Stereo = FALSE;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.Flags = tearingSupport ? DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;


    ThrowIfFailed(
        mFactory->CreateSwapChainForHwnd(queueToExecute,hWnd, &swapchainDesc, nullptr, nullptr, &swapchain1)
    );

    ThrowIfFailed(mFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ComPtr<IDXGISwapChain4> finalSwapchain;
    ThrowIfFailed(swapchain1.As(&finalSwapchain));

    return finalSwapchain;
}

ComPtr<ID3D12DescriptorHeap> Direct3D::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Flags = flags;
    heapDesc.Type = type;
    ThrowIfFailed(
        mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap))
    );

    return descriptorHeap;
}

ComPtr<ID3D12RootSignature> Direct3D::CreateRootSignature(D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc) {
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, rootSignatureVersion,
                                             &rootSignatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            THROW_ERROR("Unable to compile root signature. Error message: %s", (char*)errorBlob->GetBufferPointer());
        }
        THROW_ERROR("Unable to compile root signature. Error message: Unknown error");
    }

    ThrowIfFailed(
        mDevice->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&rootSignature))
    );

    return rootSignature;
}

ComPtr<ID3D12PipelineState> Direct3D::CreateGraphicsPipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc) {
    ComPtr<ID3D12PipelineState> pipelineState;
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));
    return pipelineState;
}

ComPtr<ID3D12PipelineState> Direct3D::CreateComputePipeline(D3D12_COMPUTE_PIPELINE_STATE_DESC& pipelineStateDesc) {
    ComPtr<ID3D12PipelineState> pipelineState;
    ThrowIfFailed(mDevice->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));
    return pipelineState;
}

void Direct3D::TransitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_STATES finalState) {

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, initialState, finalState);

    commandList->ResourceBarrier(1, &resourceBarrier);
}

void Direct3D::EnableDebugLayer() {
    ComPtr<ID3D12Debug3> d3d12Debug;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug)));
    d3d12Debug->EnableDebugLayer();
}


