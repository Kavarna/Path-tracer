#pragma once


#include <Oblivion.h>
#include "../ShaderObjects.h"
#include "./Interfaces/AccelerableStructure.h"

class BvhTree {

public:
	enum class SplitMethod {
		SAH, MiddlePoint, EqualCounts, HLBVH
	};
	static std::optional<SplitMethod> GetSplitMethodByName(const std::string& str);

public:
	const std::vector<Line>& GetRenderLines() const;
	Oblivion::BoundingBox GetBoundingBox() const;

	std::vector<BVHTreeNode>& GetNodes();

private:
	template <typename primitiveType>
	std::vector<struct BVHPrimitiveInfo> BuildPrimitives(const std::vector<primitiveType>& primitives);
	template <typename primitiveType>
	std::unique_ptr<struct BVHNode> RecursiveBuild(std::vector<struct BVHPrimitiveInfo>& primitiveInfo, SplitMethod splitType, unsigned int maxPrimitiveInNodes,
												   const std::vector<primitiveType>& primitives, std::vector<primitiveType>& newPrimitives,
												   int start, int end, unsigned int* numNodes = nullptr);

	template <unsigned int NodeType>
	int FlattenBVH(const std::unique_ptr<struct BVHNode>& root, int& offset, int maxPrimitivesInLeaf);

public:
	template <typename primitiveType>
	static std::shared_ptr<BvhTree> Create(AccelerableStructure<primitiveType>* accelerableStructure,
										   SplitMethod splitType, unsigned int maxPrimitiveInNodes, unsigned int maxPrimitivesInLeaf);

private:
	std::vector<Line> mRenderLines;

	std::vector<BVHTreeNode> mNodes;

private:
	BvhTree() = default;

};

