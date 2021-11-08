#include "GraphicsObject.h"
#include "Direct3D.h"

GraphicsObject::GraphicsObject() {
    mDevice = Direct3D::Get()->mDevice;
}

GraphicsObject::~GraphicsObject() {
    mDevice.Reset();
}
