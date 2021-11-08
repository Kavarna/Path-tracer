#include <WicUtils.h>
#include "Utils/DDSTextureLoader.h"
#include "Skymap.h"
#include "Direct3D.h"


Skymap::Skymap(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
                 ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState) :
    mResourceFlags(flags) {

    InitFromFile(path, flags, commandList, resourceState);
}

Skymap::~Skymap() { }

void Skymap::ResetIntermediaryBuffer() {

    mUploadBufferResource.Reset();

}

void Skymap::CreateViewInHeap(ID3D12DescriptorHeap* heap, std::size_t offset) {
    mHeapUsedSize = 0;
    if (!(mResourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) {
        CreateShaderResourceView(heap, offset + mHeapUsedSize);
        mHeapUsedSize += Direct3D::Get()->GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

std::size_t Skymap::GetHeapUsedSize() const {
    return mHeapUsedSize;
}

void Skymap::InitFromFile(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
                           ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState) {

    using namespace DirectX;

    auto pathWide = Conversions::s2ws(std::string(path));
    ThrowIfFailed(
        CreateDDSTextureFromFile12(mDevice.Get(), commandList.Get(), pathWide.c_str(),
                                   mResource, mUploadBufferResource)
    );

    mResourceDesc = mResource->GetDesc();

    Direct3D::Get()->TransitionResource(commandList.Get(), mResource.Get(),
                                        D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, resourceState);
}

void Skymap::CreateShaderResourceView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mResourceDesc.Format;
    
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE;
    
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    cpuHandle.Offset((INT)heapOffset);
    mDevice->CreateShaderResourceView(mResource.Get(), &srvDesc, cpuHandle);
}