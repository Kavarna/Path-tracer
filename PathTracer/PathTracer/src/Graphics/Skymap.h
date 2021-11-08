#pragma once


#include "./GraphicsObject.h"
#include "./Interfaces/IHeapObject.h"

#include <DirectXTex.h>


class Skymap : public GraphicsObject, public IHeapObject {
    // TODO: Add mipmaps
public:
    Skymap() = delete;
    Skymap(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
            ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState);
    ~Skymap();

public:
    void ResetIntermediaryBuffer();

public:
    void CreateViewInHeap(ID3D12DescriptorHeap* heap, std::size_t offset) override;
    std::size_t GetHeapUsedSize() const override;

private:
    void InitFromFile(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
                      ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState);

private:
    void CreateShaderResourceView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset);

private:
    unsigned int mDescriptorHeapUsedOffset = 0;

    ComPtr<ID3D12Resource> mResource = nullptr;

    D3D12_RESOURCE_DESC mResourceDesc = {};
    D3D12_RESOURCE_FLAGS mResourceFlags = {};

    std::size_t mHeapUsedSize = 0;

    // Temporary objects that need to hold memory
    ComPtr<ID3D12Resource> mUploadBufferResource = nullptr;
};

