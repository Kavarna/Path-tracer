#pragma once


#include "./GraphicsObject.h"
#include "./Interfaces/IHeapObject.h"

#include <DirectXTex.h>


class Texture : public GraphicsObject, public IHeapObject {
    // TODO: Add mipmaps
public:
    Texture() = delete;
    Texture(unsigned int width, unsigned int height, DXGI_FORMAT, D3D12_RESOURCE_FLAGS flags,
            D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12GraphicsCommandList> commandList = nullptr, unsigned char* data = nullptr);
    Texture(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
            ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState);
    Texture(const std::vector<std::string>& paths, D3D12_RESOURCE_FLAGS flags,
            ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState);
    ~Texture();

public:
    void ResetIntermediaryBuffer();

public:
    void CreateViewInHeap(ID3D12DescriptorHeap* heap, std::size_t offset) override;
    std::size_t GetHeapUsedSize() const override;

    void Transition(ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState);
    void Clear(ComPtr<ID3D12GraphicsCommandList> commandList);
public:
    unsigned long long GetWidth() const;
    unsigned long long GetHeight() const;

private:
    void InitFromArgs(unsigned int width, unsigned int height, DXGI_FORMAT, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES resourceState,
                      ComPtr<ID3D12GraphicsCommandList> commandList, unsigned char* data);
    void InitFromFile(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
                      ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState);
    void InitFromMultipleFiles(const std::vector<std::string>& paths, D3D12_RESOURCE_FLAGS flags,
                               ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState);

private:
    void CreateShaderResourceView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset);
    void CreateUnorderedAccessView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset);
    void CreateRenderTargetView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset);

    void LoadImageFromFile(const std::string& path, _Out_ DirectX::TexMetadata& texMetadata, _Out_ DirectX::ScratchImage& scratchImage);

private:
    unsigned int mDescriptorHeapUsedOffset = 0;

    D3D12_RESOURCE_STATES mCurrentResourceState;

    D3D12_RESOURCE_DESC mResourceDesc = {};
    D3D12_RESOURCE_FLAGS mResourceFlags = {};

    ComPtr<ID3D12Resource> mResource = nullptr;

    std::size_t mHeapUsedSize = 0;

    unsigned int mArraySize = 0;

    // Temporary objects that need to hold memory
    ComPtr<ID3D12Resource> mUploadBufferResource = nullptr;

};

