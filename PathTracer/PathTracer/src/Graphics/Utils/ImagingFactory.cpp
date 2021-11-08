#include "ImagingFactory.h"


ComPtr<IWICImagingFactory> CreateImagingFactory() {
    ComPtr<IWICImagingFactory> imaginingFactory;
    ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&imaginingFactory)));

    return imaginingFactory;
}


ImagingFactory::ImagingFactory() {
    mImagingFactory = CreateImagingFactory();
}

ComPtr<IWICImagingFactory> ImagingFactory::GetImagingFactory() {
    return mImagingFactory;
}
