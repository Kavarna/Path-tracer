#pragma once

#include <istream>

namespace DirectX {
    template <class IStream>
    inline IStream& operator >> (IStream& stream, DirectX::XMFLOAT2& data) {
        char ch;
        stream >> ch >> data.x >> ch >> data.y >> ch;
        while (ch == ' ') {
            stream >> ch;
        }
        return stream;
    }

    template <class IStream>
    inline IStream& operator >> (IStream& stream, DirectX::XMFLOAT3& data) {
        char ch;
        stream >> ch >> data.x >> ch >> data.y >> ch >> data.z >> ch;
        while (ch == ' ') {
            stream >> ch;
        }
        return stream;
    }

    template <class IStream>
    inline IStream& operator >> (IStream& stream, DirectX::XMFLOAT4& data) {
        char ch;
        stream >> ch >> data.x >> ch >> data.y >> ch >> data.z >> ch >> data.w >> ch;
        while (ch == ' ') {
            stream >> ch;
        }
        return stream;
    }
}
