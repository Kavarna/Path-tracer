#include <WicUtils.h>
#include "Texture.h"
#include "Direct3D.h"
#include "Utils/ImagingFactory.h"
#include "../Common/Limits.h"


Texture::Texture(unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags,
                 D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12GraphicsCommandList> commandList, unsigned char* data) :
    mResourceFlags(flags) {

    InitFromArgs(width, height, dxgiFormat, flags, resourceState,  commandList, data);
}

Texture::Texture(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
                 ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState) :
    mResourceFlags(flags) {

    InitFromFile(path, flags, commandList, resourceState);
}

Texture::Texture(const std::vector<std::string>& paths, D3D12_RESOURCE_FLAGS flags,
                 ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState) :
    mResourceFlags(flags) {
    
    InitFromMultipleFiles(paths, flags, commandList, resourceState);
}

Texture::~Texture() {
}

void Texture::ResetIntermediaryBuffer() {
    mUploadBufferResource.Reset();
}

void Texture::CreateViewInHeap(ID3D12DescriptorHeap* heap, std::size_t offset) {
    
    auto heapDesc = heap->GetDesc();
    if (heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        mHeapUsedSize = 0;
        if (!(mResourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) {
            CreateShaderResourceView(heap, offset + mHeapUsedSize);
            mHeapUsedSize += Direct3D::Get()->GetDescriptorSize(heapDesc.Type);
        }
        if (mResourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {
            CreateUnorderedAccessView(heap, offset + mHeapUsedSize);
            mHeapUsedSize += Direct3D::Get()->GetDescriptorSize(heapDesc.Type);
        }
    } else if (heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
        if (mResourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
            CreateRenderTargetView(heap, offset + mHeapUsedSize);
            mHeapUsedSize += Direct3D::Get()->GetDescriptorSize(heapDesc.Type);
        }
    }

}

std::size_t Texture::GetHeapUsedSize() const {
    return mHeapUsedSize;
}

void Texture::Transition(ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState) {
    if (resourceState != mCurrentResourceState) {
        Direct3D::Get()->TransitionResource(commandList.Get(), mResource.Get(), mCurrentResourceState, resourceState);
        mCurrentResourceState = resourceState;
    }
}

void Texture::Clear(ComPtr<ID3D12GraphicsCommandList> commandList) {
    auto stateBefore = mCurrentResourceState;
    Transition(commandList.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
    
    auto resourceDesc = mResource->GetDesc();
    EVALUATE(resourceDesc.Format == DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, "Clear() works only on textures with format = DXGI_FORMAT_R32G32B32A32_FLOAT");

    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);

    unsigned long long copySize = 0;
    mDevice->GetCopyableFootprints(&mResourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &copySize);
    auto bufferProperties = CD3DX12_RESOURCE_DESC::Buffer(copySize);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
        &bufferProperties,
        D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&mUploadBufferResource)
    ));


    std::vector<DirectX::XMFLOAT4> emptyPixels;
    emptyPixels.resize(resourceDesc.Width * resourceDesc.Height);

    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = emptyPixels.data();
    subresourceData.RowPitch = resourceDesc.Width* sizeof(float[4]);
    subresourceData.SlicePitch = subresourceData.RowPitch * resourceDesc.Height;
    UpdateSubresources<1>(commandList.Get(), mResource.Get(), mUploadBufferResource.Get(), 0, 0, 1, &subresourceData);

    Transition(commandList.Get(), stateBefore);
}

unsigned long long Texture::GetWidth() const {
    return mResourceDesc.Width;
}

unsigned long long Texture::GetHeight() const {
    return mResourceDesc.Height;
}

void Texture::InitFromArgs(unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags,
                           D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12GraphicsCommandList> commandList, unsigned char* data) {

    mResourceDesc = {};
    mResourceDesc.Alignment = 0;
    mResourceDesc.DepthOrArraySize = 1;
    mResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    mResourceDesc.Flags = flags;
    mResourceDesc.Format = dxgiFormat;
    mResourceDesc.Width = width;
    mResourceDesc.Height = height;
    mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
    mResourceDesc.MipLevels = 1;
    mResourceDesc.SampleDesc = { 1,0 };

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
        &mResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&mResource)
    ));
    if (data) {
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
        
        unsigned long long copySize = 0;
        mDevice->GetCopyableFootprints(&mResourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &copySize);
        auto bufferProperties = CD3DX12_RESOURCE_DESC::Buffer(copySize);

        
        ThrowIfFailed(mDevice->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &bufferProperties,
            D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&mUploadBufferResource)
        ));


        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = data;
        subresourceData.RowPitch = width * sizeof(float[4]);
        subresourceData.SlicePitch = subresourceData.RowPitch * height;
        UpdateSubresources<1>(commandList.Get(), mResource.Get(), mUploadBufferResource.Get(), 0, 0, 1, &subresourceData);

        mResource->SetName(L"Texture initialized with data from memory");
    } else {
        mResource->SetName(L"EMPTY TEXTURE");
    }

    mCurrentResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
    if (commandList) {
        Transition(commandList, resourceState);
    }
}

void Texture::InitFromFile(const std::string_view& path, D3D12_RESOURCE_FLAGS flags,
                           ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState) {
    
    using namespace DirectX;

    DirectX::TexMetadata texMetadata;
    DirectX::ScratchImage scratchImage;
    LoadImageFromFile(path.data(), texMetadata, scratchImage);

    switch (texMetadata.dimension) {
        case TEX_DIMENSION_TEXTURE1D:
            mResourceDesc = CD3DX12_RESOURCE_DESC::Tex1D(
                texMetadata.format, (UINT64)texMetadata.width, (UINT16)texMetadata.arraySize
            );
            break;
        case TEX_DIMENSION_TEXTURE2D:
            mResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                texMetadata.format, (UINT64)texMetadata.width, (UINT)texMetadata.height,
                (UINT16)texMetadata.arraySize
            );
            break;
        case TEX_DIMENSION_TEXTURE3D:
            mResourceDesc = CD3DX12_RESOURCE_DESC::Tex3D(
                texMetadata.format, (UINT64)texMetadata.width, (UINT)texMetadata.height,
                (UINT16)texMetadata.depth
            );
            break;
        default:
            break;
    }


    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
        &mResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&mResource)
    ));
    mResource->SetName(Conversions::s2ws(path.data()).c_str());

    unsigned long long copySize;
    mDevice->GetCopyableFootprints(&mResourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &copySize);
    auto bufferProperties = CD3DX12_RESOURCE_DESC::Buffer(copySize);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
        &bufferProperties, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&mUploadBufferResource)
    ));

    const Image* sourceImage = scratchImage.GetImage(0, 0, 0);
    
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = sourceImage->pixels;
    subresourceData.RowPitch = sourceImage->rowPitch;
    subresourceData.SlicePitch = sourceImage->slicePitch;

    UpdateSubresources<1>(commandList.Get(), mResource.Get(), mUploadBufferResource.Get(), 0, 0, 1, &subresourceData);

    mCurrentResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
    Transition(commandList, resourceState);
}

void Texture::InitFromMultipleFiles(const std::vector<std::string>& paths, D3D12_RESOURCE_FLAGS flags,
                                    ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES resourceState) {
    using namespace DirectX;

    EVALUATE(paths.size() > 0, "More than 0 textures are needed in order to create this type of texture");
    EVALUATE(paths.size() < MAX_TEXTURES_IN_TEXTURE_ARRAY, "You cannot have ", paths.size(), " total textures. ",
             MAX_TEXTURES_IN_TEXTURE_ARRAY, " is the maximum");

    std::vector<TexMetadata> textureMetadatas(paths.size());
    std::vector<ScratchImage> scratchImages(paths.size());

    unsigned int size = 0;

    LoadImageFromFile(paths[0], textureMetadatas[0], scratchImages[0]);
    for (unsigned int i = 1; i < paths.size(); ++i) {
        LoadImageFromFile(paths[i], textureMetadatas[i], scratchImages[i]);

        EVALUATE(textureMetadatas[i].width == textureMetadatas[i - 1].width, "All textures must have the same width");
        EVALUATE(textureMetadatas[i].height == textureMetadatas[i - 1].height, "All textures must have the same height");
        EVALUATE(textureMetadatas[i].dimension == textureMetadatas[i - 1].dimension, "All textures must have the same dimension");
        EVALUATE(textureMetadatas[i].format == textureMetadatas[i - 1].format, "All textures must have the same format");
    }

    switch (textureMetadatas[0].dimension) {
        case TEX_DIMENSION_TEXTURE1D:
            mResourceDesc = CD3DX12_RESOURCE_DESC::Tex1D(
                textureMetadatas[0].format, (UINT64)textureMetadatas[0].width, (UINT16)paths.size()
            );
            break;
        case TEX_DIMENSION_TEXTURE2D:
            mResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                textureMetadatas[0].format, (UINT64)textureMetadatas[0].width, (UINT)textureMetadatas[0].height,
                (UINT16)paths.size(), 1
            );
            break;
        default:
            EVALUATE(false, "Texture dimension ", textureMetadatas[0].dimension, " is not suitable for a texture array");
            break;
    }

    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
        &mResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&mResource)
    ));
    mResource->SetName(L"Texture array");

    unsigned long long copySize;
    mDevice->GetCopyableFootprints(&mResourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &copySize);
    auto bufferProperties = CD3DX12_RESOURCE_DESC::Buffer(paths.size() * copySize);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
        &bufferProperties, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&mUploadBufferResource)
    ));

    std::vector<D3D12_SUBRESOURCE_DATA> subresourceData(paths.size());
    for (unsigned int i = 0; i < paths.size(); ++i) {
        const Image* image = scratchImages[i].GetImage(0, 0, 0);
        subresourceData[i].pData = image->pixels;
        subresourceData[i].RowPitch = image->rowPitch;
        subresourceData[i].SlicePitch = image->slicePitch;

    }
    UpdateSubresources(commandList.Get(), mResource.Get(), mUploadBufferResource.Get(), 0, 0, (unsigned int)paths.size(), subresourceData.data());

    mCurrentResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
    Transition(commandList, resourceState);

    mArraySize = (unsigned int)paths.size();
}

void Texture::CreateShaderResourceView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mResourceDesc.Format;
    srvDesc.ViewDimension = ResourceDimensionToViewDimension<D3D12_SRV_DIMENSION>(mResourceDesc.Dimension, mArraySize > 0);
    if (mArraySize > 0) {
        srvDesc.Texture2DArray.ArraySize = mArraySize;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.PlaneSlice = 0;
        srvDesc.Texture2DArray.ResourceMinLODClamp = 0;
    } else {
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    cpuHandle.Offset((INT)heapOffset);
    mDevice->CreateShaderResourceView(mResource.Get(), &srvDesc, cpuHandle);
}

void Texture::CreateUnorderedAccessView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = mResourceDesc.Format;
    uavDesc.ViewDimension = ResourceDimensionToViewDimension<D3D12_UAV_DIMENSION>(mResourceDesc.Dimension, mArraySize > 0);
    if (mArraySize > 0) {
        uavDesc.Texture2DArray.ArraySize = mArraySize;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.MipSlice = 0;
        uavDesc.Texture2DArray.PlaneSlice = 0;
    } else {
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.Texture2D.PlaneSlice = 0;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    cpuHandle.Offset((INT)heapOffset);
    mDevice->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, cpuHandle);
}

void Texture::CreateRenderTargetView(ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::size_t heapOffset) {
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = mResourceDesc.Format;
    rtvDesc.ViewDimension = ResourceDimensionToViewDimension<D3D12_RTV_DIMENSION>(mResourceDesc.Dimension, mArraySize > 0);
    if (mArraySize > 0) {
        rtvDesc.Texture2DArray.ArraySize = mArraySize;
        rtvDesc.Texture2DArray.FirstArraySlice = 0;
        rtvDesc.Texture2DArray.MipSlice = 0;
        rtvDesc.Texture2DArray.PlaneSlice = 0;
    } else {
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    cpuHandle.Offset((INT)heapOffset);
    mDevice->CreateRenderTargetView(mResource.Get(), &rtvDesc, cpuHandle);
}

void Texture::LoadImageFromFile(const std::string& path, _Out_ DirectX::TexMetadata& texMetadata, _Out_ DirectX::ScratchImage& scratchImage) {
    using namespace DirectX;

    std::filesystem::path filePath(path);

    auto extension = filePath.extension();

    auto wideCharPath = Conversions::s2ws(path.data());

    if (extension == ".dds") {
        ThrowIfFailed(
            LoadFromDDSFile(wideCharPath.data(), DDS_FLAGS::DDS_FLAGS_NONE,
                            &texMetadata, scratchImage)
        );
    } else if (extension == ".hdr") {
        ThrowIfFailed(
            LoadFromHDRFile(wideCharPath.data(), &texMetadata, scratchImage)
        );
    } else if (extension == ".tga") {
        ThrowIfFailed(
            LoadFromTGAFile(wideCharPath.data(), &texMetadata, scratchImage)
        );
    } else {
        ThrowIfFailed(
            LoadFromWICFile(wideCharPath.data(), WIC_FLAGS::WIC_FLAGS_NONE,
                            &texMetadata, scratchImage)
        );
    }

}
