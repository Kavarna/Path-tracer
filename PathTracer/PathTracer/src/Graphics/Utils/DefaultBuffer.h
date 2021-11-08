#pragma once



#include <Oblivion.h>
#include "../GraphicsObject.h"


template <class T>
class DefaultBuffer : public GraphicsObject {

public:
    DefaultBuffer(ComPtr<ID3D12GraphicsCommandList> commandList, const T& data) {
        Init(commandList, &data, 1);
    }

    DefaultBuffer(ComPtr<ID3D12GraphicsCommandList> commandList, const T* data, unsigned int numElements) {
        Init(commandList, data, numElements);
    }

    void ResetIntermediaryBuffer() {
        mUploadBuffer.Reset();
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
        return mBuffer->GetGPUVirtualAddress();
    }

private:
    void Init(ComPtr<ID3D12GraphicsCommandList> commandList, const T* data, unsigned int numElements) {

        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * numElements);
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

        ThrowIfFailed(mDevice->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&mBuffer)
        ));

        ThrowIfFailed(mDevice->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&mUploadBuffer)
        ));

        D3D12_SUBRESOURCE_DATA subresourceData = {};

        subresourceData.pData = data;
        subresourceData.RowPitch = sizeof(T) * numElements;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources<1>(commandList.Get(), mBuffer.Get(), mUploadBuffer.Get(), 0, 0, 1, &subresourceData);
    }


private:
    ComPtr<ID3D12Resource1> mBuffer;
    ComPtr<ID3D12Resource1> mUploadBuffer;

};
