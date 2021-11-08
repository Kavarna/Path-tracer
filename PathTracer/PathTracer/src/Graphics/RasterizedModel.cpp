#include "RasterizedModel.h"


std::array<D3D12_INPUT_ELEMENT_DESC, RasterizedModel::SVertex::LayoutElementCount> RasterizedModel::SVertex::GetInputElementDesc() {
    std::array<D3D12_INPUT_ELEMENT_DESC, RasterizedModel::SVertex::LayoutElementCount> elementDesc;
    elementDesc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    elementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    elementDesc[0].InputSlot = 0;
    elementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    elementDesc[0].InstanceDataStepRate = 0;
    elementDesc[0].SemanticIndex = 0;
    elementDesc[0].SemanticName = "POSITION";

    elementDesc[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    elementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    elementDesc[1].InputSlot = 0;
    elementDesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    elementDesc[1].InstanceDataStepRate = 0;
    elementDesc[1].SemanticIndex = 0;
    elementDesc[1].SemanticName = "TEXCOORD";

    return elementDesc;
}

RasterizedModel::RasterizedModel(PrimitiveType primitiveType, ComPtr<ID3D12GraphicsCommandList> cmdList) {

    switch (primitiveType) {
        case RasterizedModel::PrimitiveType::Quad:
            InitAsQuad(cmdList);
            break;
        case RasterizedModel::PrimitiveType::Cube:
            InitAsCube(cmdList);
            break;
        default:
            break;
    }

}

void RasterizedModel::ResetIntermediaryBuffer() {
    mVertexBuffer->ResetIntermediaryBuffer();
    mIndexBuffer->ResetIntermediaryBuffer();
}

unsigned int RasterizedModel::GetIndexCount() const {
    return mIndexCount;
}

void RasterizedModel::InitAsQuad(ComPtr<ID3D12GraphicsCommandList> cmdList) {

    float len = 1.0f;

    SVertex vertices[] = {
        { { -len, +len, 0.5f }, { 0.0f, 1.0f } },
        { { +len, +len, 0.5f }, { 1.0f, 1.0f } },
        { { +len, -len, 0.5f }, { 1.0f, 0.0f } },
        { { -len, -len, 0.5f }, { 0.0f, 0.0f } },
    };
    mVertexBufferSizeInBytes = sizeof(SVertex) * ARRAYSIZE(vertices);

    DWORD indices[] = {
        0, 1, 2,
        0, 2, 3,
    };
    mIndexBufferSizeInBytes = sizeof(DWORD) * ARRAYSIZE(indices);
    mIndexCount = ARRAYSIZE(indices);

    mVertexBuffer = std::make_unique<DefaultBuffer<SVertex>>(cmdList, vertices, (unsigned int)ARRAYSIZE(vertices));
    mIndexBuffer = std::make_unique<DefaultBuffer<DWORD>>(cmdList, indices, (unsigned int)ARRAYSIZE(indices));
}

void RasterizedModel::InitAsCube(ComPtr<ID3D12GraphicsCommandList> cmdList) {

    SVertex vertices[] = {
        { {-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f} }, // 0
        { {-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f} }, // 1
        { {1.0f,  1.0f, -1.0f},  {1.0f, 1.0f} }, // 2
        { {1.0f, -1.0f, -1.0f},  {1.0f, 0.0f} }, // 3
        { {-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f} }, // 4
        { {-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f} }, // 5
        { {1.0f,  1.0f,  1.0f},  {1.0f, 1.0f} }, // 6
        { {1.0f, -1.0f,  1.0f},  {1.0f, 0.0f} }  // 7
    };
    mVertexBufferSizeInBytes = sizeof(SVertex) * ARRAYSIZE(vertices);
    DWORD indices[] = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    mIndexBufferSizeInBytes = sizeof(DWORD) * ARRAYSIZE(indices);
    mIndexCount = ARRAYSIZE(indices);

    mVertexBuffer = std::make_unique<DefaultBuffer<SVertex>>(cmdList, vertices, (unsigned int)ARRAYSIZE(vertices));
    mIndexBuffer = std::make_unique<DefaultBuffer<DWORD>>(cmdList, indices, (unsigned int)ARRAYSIZE(indices));
}

void RasterizedModel::Bind(ID3D12GraphicsCommandList* cmdList) const {

    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    vbView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
    vbView.SizeInBytes = mVertexBufferSizeInBytes;
    vbView.StrideInBytes = sizeof(SVertex);
    cmdList->IASetVertexBuffers(0, 1, &vbView);

    D3D12_INDEX_BUFFER_VIEW ibView = {};
    ibView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT;
    ibView.SizeInBytes = mIndexBufferSizeInBytes;
    cmdList->IASetIndexBuffer(&ibView);

    cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
