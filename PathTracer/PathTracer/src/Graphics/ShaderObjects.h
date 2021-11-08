#pragma once

#include <Oblivion.h>
#include "../Common/Limits.h"
#include "BoundingBox.h"

struct Sphere {
	DirectX::XMFLOAT3 Position;
	float Radius;
	DirectX::XMFLOAT4 Color;
	DirectX::XMFLOAT2 LightProperties; // Reflection & Transparency
	DirectX::XMFLOAT2 pad = DirectX::XMFLOAT2(0.0f, 0.0f);

	Sphere() = default;
	Sphere(const DirectX::XMFLOAT3& position, float radius, const DirectX::XMFLOAT4& color, const DirectX::XMFLOAT2& lightProperties) :
		Position(position), Radius(radius), Color(color), LightProperties(lightProperties) { }

};

struct Line {
	DirectX::XMFLOAT3 Start;
	float pad = 0.0f;
	DirectX::XMFLOAT3 End;
	float pad2 = 0.0f;
	DirectX::XMFLOAT4 Color;

	Line() = default;
	Line(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const DirectX::XMFLOAT4& color) :
		Start(start), End(end), Color(color) { };

	friend std::ostream& operator << (std::ostream& stream, const Line& line) {
		stream << "\n{";
		stream << "Start: " << line.Start << ", ";
		stream << "End: " << line.End << ", ";
		stream << "Color: " << line.Color;
		stream << "}";
		return stream;
	}
};

struct Light {
	DirectX::XMFLOAT3 Position;
	float Radius;
	DirectX::XMFLOAT4 Emissive;

	Light() = default;
	Light(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& emissive, float radius) :
		Position(position), Emissive(emissive), Radius(radius) {};
};

struct SpheresCB {
	Sphere spheres[MAX_SPHERES];
	unsigned int numSpheres;
	DirectX::XMFLOAT3 pad;
};

struct LinesCB {
	Line lines[MAX_LINES];
	unsigned int numLines;
	DirectX::XMFLOAT3 pad;
};

struct LightsCB {
	Light lights[MAX_LIGHTS];
	unsigned int numLights;
	DirectX::XMFLOAT3 pad;
};

OBLIVION_ALIGN(16) struct BVHTreeNode {
	DirectX::XMFLOAT3 minAABB;
	unsigned int primitiveOffset;
	
	DirectX::XMFLOAT3 maxAABB;
	unsigned int secondChildOffset;
	
	unsigned int numberOfPrimitives;
	unsigned int axis;
	unsigned int nodeType;
	unsigned int additionalInfo;

	friend std::ostream& operator << (std::ostream& stream, const BVHTreeNode& bvh) {
		stream << "\n{";
		stream << "minAABB: " << bvh.minAABB << ", ";
		stream << "maxAABB: " << bvh.maxAABB << ", ";
		stream << "primitiveOffset: " << bvh.primitiveOffset << ", ";
		stream << "secondChildOffset: " << bvh.secondChildOffset << ", ";
		stream << "numberOfPrimitives: " << bvh.numberOfPrimitives << ", ";
		stream << "axis: " << bvh.axis << ", ";
		stream << "nodeType: " << bvh.nodeType << ", ";
		stream << "additionalInfo: " << bvh.additionalInfo;
		stream << "}";
		return stream;
	}
};

OBLIVION_ALIGN(16) struct TraceVertex {
	DirectX::XMFLOAT3 position;
	float pad;
	DirectX::XMFLOAT3 normal;
	unsigned int materialIndex;
	DirectX::XMFLOAT4 texCoords;

	friend std::ostream& operator << (std::ostream& stream, const TraceVertex& v) {
		stream << "\n{";
		stream << "position: " << v.position << ", ";
		stream << "normal: " << v.normal << ", ";
		stream << "materialIndex: " << v.materialIndex << ", ";
		stream << "texCoords: " << v.texCoords;
		stream << "}";
		return stream;
	}
};

OBLIVION_ALIGN(16) struct TraceModelPrimitive {
	unsigned int index0;
	unsigned int index1;
	unsigned int index2;
	float pad0;
};

OBLIVION_ALIGN(16) struct TraceScenePrimitive {
	DirectX::XMFLOAT3 minAABB;
	unsigned int modelOffset;
	DirectX::XMFLOAT3 maxAABB;
	float pad1;
};

enum MaterialType : int {
	Diffuse, Specular
};

OBLIVION_ALIGN(16) struct Material {
	DirectX::XMFLOAT4 diffuseColor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT4 emissiveColor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

	float metallic = 0.0f;
	float roughness = 1.0f;
	float ior = 1.45f;
	float unused;

	int textureIndex = -1;
	int materialType = MaterialType::Diffuse;
	DirectX::XMFLOAT2 unused2;
	
	friend std::ostream& operator << (std::ostream& stream, const Material& m) {
		stream << "\n{";
		stream << "Material type: " << m.materialType << ", ";
		stream << "Diffuse Color: " << m.diffuseColor<< ", ";
		stream << "Emissive Color: " << m.emissiveColor << ", ";
		stream << "metallic: " << m.metallic << ", ";
		stream << "roughness: " << m.roughness << ", ";
		stream << "ior: " << m.ior << ", ";
		stream << "textureIndex: " << std::hex << "0x" << m.textureIndex << std::dec;
		stream << "}";
		return stream;
	}
};
