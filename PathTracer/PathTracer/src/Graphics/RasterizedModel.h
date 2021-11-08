#pragma once


#include <Oblivion.h>
#include "GraphicsObject.h"
#include "Utils/DefaultBuffer.h"


class RasterizedModel : public GraphicsObject {

public:

    struct SVertex {
        static constexpr const unsigned int LayoutElementCount = 2;
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texCoord;

        static std::array<D3D12_INPUT_ELEMENT_DESC, LayoutElementCount> GetInputElementDesc();
    };

    enum class PrimitiveType {
        Quad, Cube
    };
public:
    RasterizedModel() = delete;
    RasterizedModel(PrimitiveType primitiveType, ComPtr<ID3D12GraphicsCommandList> cmdList);

    void ResetIntermediaryBuffer();

    void Bind(ID3D12GraphicsCommandList* cmdList) const;

    unsigned int GetIndexCount() const;

private:
    void InitAsQuad(ComPtr<ID3D12GraphicsCommandList> cmdList);
    void InitAsCube(ComPtr<ID3D12GraphicsCommandList> cmdList);

private:
    std::unique_ptr<DefaultBuffer<SVertex>> mVertexBuffer;
    std::unique_ptr<DefaultBuffer<DWORD>> mIndexBuffer;
    
    unsigned int mVertexBufferSizeInBytes;
    unsigned int mIndexBufferSizeInBytes;

    unsigned int mIndexCount;
};
