#pragma once


#include <Oblivion.h>


class ImagingFactory : public ISingletone<ImagingFactory> {
    MAKE_SINGLETONE_CAPABLE(ImagingFactory);

private:
    ImagingFactory();

public:
    ComPtr<IWICImagingFactory> GetImagingFactory();
private:
    ComPtr<IWICImagingFactory> mImagingFactory;
};


