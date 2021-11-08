#pragma once


#include <Oblivion.h>
#include "../GraphicsObject.h"
#include "../Interfaces/IHeapObject.h"

template <typename T>
class UploadBuffer : public GraphicsObject, public IHeapObject {

public:
    UploadBuffer(unsigned int elementCount, bool isConstantBuffer = false) :
        mIsConstantBuffer(isConstantBuffer), mNumElements(elementCount), mElementSize(sizeof(T)) {

        if (mIsConstantBuffer) {
            mElementSize = (unsigned int)Math::AlignUp(sizeof(T), 256);
        }

        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
        auto defaultBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mElementSize * mNumElements);

        ThrowIfFailed(mDevice->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &defaultBufferDesc,
            D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&mBuffer)
        ));

        ThrowIfFailed(mBuffer->Map(0, nullptr, &mData));
    }

    void CopyData(unsigned int elementIndex, const T& data) {
        memcpy_s(mData, mElementSize, &data, mElementSize);
    }

    void CopyData(const T* data, unsigned int numElements = std::numeric_limits<unsigned int>::max()) {
        if (numElements > mNumElements) {
            numElements = mNumElements;
        }
        memcpy_s(mData, mElementSize * mNumElements, data, mElementSize * numElements);
    }

    unsigned int GetElementSize() const {
        return mElementSize;
    }

    unsigned int GetTotalElements() const {
        return mNumElements;
    }

    unsigned int GetTotalSize() const {
        return mNumElements * mElementSize;
    }

    void CreateViewInHeap(ID3D12DescriptorHeap* heap, std::size_t offset) override {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = mBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = GetTotalSize();

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap->GetCPUDescriptorHandleForHeapStart());
        cpuHandle.Offset((INT)offset);

        mDevice->CreateConstantBufferView(&cbvDesc, cpuHandle);
    };
    std::size_t GetHeapUsedSize() const override {
        return mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    };

    ~UploadBuffer() {
        mBuffer->Unmap(0, nullptr);
        mData = nullptr;
    }

private:
    ComPtr<ID3D12Resource1> mBuffer;

    unsigned int mDescriptorHeapUsedOffset = 0;

    unsigned int mElementSize;
    unsigned int mNumElements;
    bool mIsConstantBuffer;

    void* mData;

};

