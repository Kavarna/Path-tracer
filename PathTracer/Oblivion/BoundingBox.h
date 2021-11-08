#pragma once


#include <DirectXMath.h>
#include "CommonMath.h"

#undef min

namespace Oblivion {
	struct BoundingBox {

		DirectX::XMFLOAT3 minPoint;
		DirectX::XMFLOAT3 maxPoint;

		BoundingBox() : minPoint(FLT_MAX, FLT_MAX, FLT_MAX), maxPoint(-FLT_MAX, -FLT_MAX, -FLT_MAX) { };
		BoundingBox(DirectX::XMFLOAT3 minPoint, DirectX::XMFLOAT3 maxPoint) : minPoint(std::move(minPoint)), maxPoint(std::move(maxPoint)) { };
		BoundingBox(const BoundingBox& rhs) = default;
		~BoundingBox() = default;

		DirectX::XMFLOAT3 Center() const {
			return (minPoint + maxPoint) / 2.f;
		}

		DirectX::XMFLOAT3 Diagonal() const {
			return maxPoint - minPoint;
		}

		DirectX::XMFLOAT3 Offset(const DirectX::XMFLOAT3& point) const {
			DirectX::XMFLOAT3 newPoint = point - minPoint;
			if (maxPoint.x > minPoint.x) newPoint.x = newPoint.x / (maxPoint.x - minPoint.x);
			if (maxPoint.y > minPoint.y) newPoint.y = newPoint.y / (maxPoint.y - minPoint.y);
			if (maxPoint.z > minPoint.z) newPoint.z = newPoint.z / (maxPoint.z - minPoint.z);
			return newPoint;
		}

		Math::Axis MaximumExtent() const {
			auto diag = Diagonal();
			if (diag.x > diag.y && diag.x > diag.z) {
				return Math::Axis::X;
			} else if (diag.y > diag.z) {
				return Math::Axis::Y;
			} else {
				return Math::Axis::Z;
			}
		}

		float SurfaceArea() const {
			auto diag = Diagonal();

			return 2.f * (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z);
		}

		BoundingBox operator | (const BoundingBox& rhs) const {
			BoundingBox bb;
			bb.minPoint.x = std::min(minPoint.x, rhs.minPoint.x);
			bb.minPoint.y = std::min(minPoint.y, rhs.minPoint.y);
			bb.minPoint.z = std::min(minPoint.z, rhs.minPoint.z);

			bb.maxPoint.x = std::max(maxPoint.x, rhs.maxPoint.x);
			bb.maxPoint.y = std::max(maxPoint.y, rhs.maxPoint.y);
			bb.maxPoint.z = std::max(maxPoint.z, rhs.maxPoint.z);

			return bb;
		}

		BoundingBox& operator |= (const BoundingBox& rhs)  {
			minPoint.x = std::min(minPoint.x, rhs.minPoint.x);
			minPoint.y = std::min(minPoint.y, rhs.minPoint.y);
			minPoint.z = std::min(minPoint.z, rhs.minPoint.z);

			maxPoint.x = std::max(maxPoint.x, rhs.maxPoint.x);
			maxPoint.y = std::max(maxPoint.y, rhs.maxPoint.y);
			maxPoint.z = std::max(maxPoint.z, rhs.maxPoint.z);

			return *this;
		}

		BoundingBox operator | (const DirectX::XMFLOAT3& rhs) const {
			BoundingBox bb;
			bb.minPoint.x = std::min(minPoint.x, rhs.x);
			bb.minPoint.y = std::min(minPoint.y, rhs.y);
			bb.minPoint.z = std::min(minPoint.z, rhs.z);

			bb.maxPoint.x = std::max(maxPoint.x, rhs.x);
			bb.maxPoint.y = std::max(maxPoint.y, rhs.y);
			bb.maxPoint.z = std::max(maxPoint.z, rhs.z);

			return bb;
		}

		BoundingBox& operator |= (const DirectX::XMFLOAT3& rhs) {
			minPoint.x = std::min(minPoint.x, rhs.x);
			minPoint.y = std::min(minPoint.y, rhs.y);
			minPoint.z = std::min(minPoint.z, rhs.z);

			maxPoint.x = std::max(maxPoint.x, rhs.x);
			maxPoint.y = std::max(maxPoint.y, rhs.y);
			maxPoint.z = std::max(maxPoint.z, rhs.z);

			return *this;
		}

	};

}