#pragma once


#include <Oblivion.h>


class IHeapObject {
public:
    virtual void CreateViewInHeap(ID3D12DescriptorHeap* heap, std::size_t offset) = 0;
    virtual std::size_t GetHeapUsedSize() const = 0;
};
