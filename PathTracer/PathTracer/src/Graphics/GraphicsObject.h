#pragma once



#include <Oblivion.h>


class GraphicsObject {
public:
    GraphicsObject();
    ~GraphicsObject();

protected:
    ComPtr<ID3D12Device6> mDevice;
};

