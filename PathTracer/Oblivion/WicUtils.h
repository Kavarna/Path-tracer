#pragma once

#include <Oblivion.h>

// get the dxgi format equivilent of a wic format
inline DXGI_FORMAT GetDXGIFormatFromWICFormat(const WICPixelFormatGUID& wicFormatGUID) {
    if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

    else return DXGI_FORMAT_UNKNOWN;
}

// get a dxgi compatible wic format from another wic format
inline WICPixelFormatGUID GetConvertToWICFormat(const WICPixelFormatGUID& wicFormatGUID) {
    if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

    else return GUID_WICPixelFormatDontCare;
}

inline int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat) {
    if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

    else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
    else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
    return -1;
}

template <typename ConvertToType>
ConvertToType ResourceDimensionToViewDimension(D3D12_RESOURCE_DIMENSION resourceDimension, bool isArray) {

    switch (resourceDimension) {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            if constexpr (std::is_same_v<ConvertToType, D3D12_SRV_DIMENSION>) {
                 return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_UAV_DIMENSION>) {
                return D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER;
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_RTV_DIMENSION>) {
                return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_BUFFER;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if constexpr (std::is_same_v<ConvertToType, D3D12_SRV_DIMENSION>) {
                if (isArray) {
                    return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                } else {
                    return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
                }
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_UAV_DIMENSION>) {
                if (isArray) {
                    return D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                } else {
                    return D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D;
                }
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_RTV_DIMENSION>) {
                if (isArray) {
                    return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                } else {
                    return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE1D;
                }
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if constexpr (std::is_same_v<ConvertToType, D3D12_SRV_DIMENSION>) {
                if (isArray) {
                    return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                } else {
                    return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
                }
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_UAV_DIMENSION>) {
                if (isArray) {
                    return D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                } else {
                    return D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;
                }
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_RTV_DIMENSION>) {
                if (isArray) {
                    return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                } else {
                    return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;
                }
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            if constexpr (std::is_same_v<ConvertToType, D3D12_SRV_DIMENSION>) {
                return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_UAV_DIMENSION>) {
                return D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D;
            }
            else if constexpr (std::is_same_v<ConvertToType, D3D12_RTV_DIMENSION>) {
                return D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE3D;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_UNKNOWN:
        default:
            break;
    }

    return static_cast<ConvertToType>(0);
}
