#include "BvhTree.h"

#include <boost/algorithm/string.hpp>

#include "../Model.h"
#include "../Scene.h"

struct BVHPrimitiveInfo {
	unsigned int index;
	Oblivion::BoundingBox boundingBox;

	BVHPrimitiveInfo(unsigned int index, Oblivion::BoundingBox boudingBox) :
		index(index), boundingBox(std::move(boudingBox)) { 	}

	BVHPrimitiveInfo& operator=(const BVHPrimitiveInfo& rhs) {
		index = rhs.index;
		boundingBox = rhs.boundingBox;

		return *this;
	}
};

struct BVHNode {

	std::unique_ptr<BVHNode> children[2];
	Oblivion::BoundingBox boundingBox;
	Math::Axis splitAxis;
	int firstIndex, numberOfPrimitives;
	unsigned int index;

	void InitAsLeaf(const Oblivion::BoundingBox& bb, int first, int nPrimitives) {
		firstIndex = first;
		numberOfPrimitives = nPrimitives;
		boundingBox = bb;
		children[0] = nullptr;
		children[1] = nullptr;
		Oblivion::DebugPrintLine("Created leaf with ", nPrimitives, " primitives");
	}

	void InitAsInterior(Math::Axis splitAxis_, std::unique_ptr<BVHNode>&& children0, std::unique_ptr<BVHNode>&& children1) {
		children[0] = std::move(children0);
		children[1] = std::move(children1);
		splitAxis = splitAxis_;

		boundingBox = children[0]->boundingBox | children[1]->boundingBox;
	}
};

struct BucketInfo {
	int count;
	Oblivion::BoundingBox bb;
};

std::optional<BvhTree::SplitMethod> BvhTree::GetSplitMethodByName(const std::string& str) {
	if (boost::iequals(str, "SAH")) {
		return SplitMethod::SAH;
	} else if (boost::iequals(str, "MiddlePoint")) {
		return SplitMethod::MiddlePoint;
	} else if (boost::iequals(str, "EqualCount")) {
		return SplitMethod::EqualCounts;
	} else if (boost::iequals(str, "HLBVH")) {
		return SplitMethod::HLBVH;
	}
	return std::nullopt;
}

const std::vector<Line>& BvhTree::GetRenderLines() const {
	return mRenderLines;
}

Oblivion::BoundingBox BvhTree::GetBoundingBox() const {
	// Node 0 = root = the bounding box that includes the whole subtree
	return Oblivion::BoundingBox(mNodes[0].minAABB, mNodes[0].maxAABB);
}

std::vector<BVHTreeNode>& BvhTree::GetNodes() {
	return mNodes;
}

template <unsigned int nodeType>
int BvhTree::FlattenBVH(const std::unique_ptr<struct BVHNode>& root, int& offset, int maxPrimitivesInLeaf) {

	BVHTreeNode& currentNode = mNodes.at(offset);
	currentNode.minAABB = root->boundingBox.minPoint;
	currentNode.maxAABB = root->boundingBox.maxPoint;
	currentNode.nodeType = nodeType;

	int myOffset = offset++;

	if (root->numberOfPrimitives > 0) {
		// This must be a leaf
		EVALUATE(root->children[0] == nullptr && root->children[1] == nullptr, "Can't have a leaf node with children!");
		EVALUATE(root->numberOfPrimitives <= maxPrimitivesInLeaf, "Too many primitives (", root->numberOfPrimitives, ") in a leaf node. Maximum allowed = ", maxPrimitivesInLeaf);

		currentNode.numberOfPrimitives = root->numberOfPrimitives;
		currentNode.primitiveOffset = root->firstIndex;

	} else {
		currentNode.axis = static_cast<unsigned int>(root->splitAxis);
		currentNode.numberOfPrimitives = 0;

		FlattenBVH<nodeType>(root->children[0], offset, maxPrimitivesInLeaf);
		currentNode.secondChildOffset = FlattenBVH<nodeType>(root->children[1], offset, maxPrimitivesInLeaf);
	}


	return myOffset;
}

template <typename primitiveType>
std::vector<BVHPrimitiveInfo> BvhTree::BuildPrimitives(const std::vector<primitiveType>& primitives) {
	std::vector<BVHPrimitiveInfo> primitivesInfo;
	primitivesInfo.reserve(primitives.size());

	for (unsigned int i = 0; i < primitives.size(); ++i) {
		primitivesInfo.emplace_back(i, primitives[i]->GetBoundingBox());
	}

	return primitivesInfo;
}

template <typename primitiveType>
std::unique_ptr<struct BVHNode> BvhTree::RecursiveBuild(std::vector<BVHPrimitiveInfo>& primitiveInfo, SplitMethod splitMethod, unsigned int maxPrimitiveInNodes,
														const std::vector<primitiveType>& primitives, std::vector<primitiveType>& newPrimitives,
														int start, int end, unsigned int* numNodes) {
	EVALUATE(end > start,  "Cannot pass a start larger than the end: ", start, " >= ", end);
	
	if (numNodes) {
		(*numNodes)++;
	}

	Oblivion::BoundingBox bb;
	for (int i = start; i < end; ++i) {
		bb |= primitiveInfo[i].boundingBox;
	}

	std::unique_ptr<BVHNode> currentNode = std::make_unique<BVHNode>();

	int nPrimitives = end - start;
	if (nPrimitives == 1) {
		int indexStart = (int)newPrimitives.size();
		for (int i = start; i < end; ++i) {
			auto primitiveIndex = primitiveInfo[i].index;
			newPrimitives.push_back(primitives[primitiveIndex]);
		}
		currentNode->InitAsLeaf(bb, indexStart, nPrimitives);
	} else {
		Oblivion::BoundingBox centroidsBB;
		for (int i = start; i < end; ++i) {
			centroidsBB |= primitiveInfo[i].boundingBox.Center();
		}

		auto axis = centroidsBB.MaximumExtent();
		if (fabs(Math::GetValueOnAxis(centroidsBB.minPoint, axis) - Math::GetValueOnAxis(centroidsBB.maxPoint, axis)) < Math::EPSILON) {
			int indexStart = (int)newPrimitives.size();
			for (int i = start; i < end; ++i) {
				auto primitiveIndex = primitiveInfo[i].index;
				newPrimitives.push_back(primitives[primitiveIndex]);
			}
			currentNode->InitAsLeaf(bb, indexStart, nPrimitives);
		} else {
			int mid;
			switch (splitMethod) {
				case BvhTree::SplitMethod::MiddlePoint: {
					auto midPoint = std::partition(primitiveInfo.begin() + start, primitiveInfo.begin() + end,
												   [&](const BVHPrimitiveInfo& info) {
													   return Math::GetValueOnAxis(info.boundingBox.Center(), axis) < Math::GetValueOnAxis(centroidsBB.Center(), axis);
												   });
					mid = int(midPoint - primitiveInfo.begin());
					if (midPoint != primitiveInfo.begin() + start && midPoint != primitiveInfo.begin() + end) break;
					Oblivion::DebugPrintLine("Middle point did not give a good enough solution. Trying EqualCounts");
				}
				__fallthrough;
				case BvhTree::SplitMethod::EqualCounts: {
					mid = int((start + end) * 0.5f);
					std::nth_element(primitiveInfo.begin() + start, primitiveInfo.begin() + mid, primitiveInfo.begin() + end,
									 [&](const BVHPrimitiveInfo& lhs, const BVHPrimitiveInfo& rhs) {
										 return Math::GetValueOnAxis(lhs.boundingBox.Center(), axis) < Math::GetValueOnAxis(rhs.boundingBox.Center(), axis);
									 });
					break;
				}
				case BvhTree::SplitMethod::HLBVH: 
					EVALUATE(false, "HLBVH split is not ready yet");
					break;
				case BvhTree::SplitMethod::SAH:
				default:
					if (nPrimitives <= 2) {
						mid = int((start + end) * 0.5f);
						std::nth_element(primitiveInfo.begin() + start, primitiveInfo.begin() + mid, primitiveInfo.begin() + end,
										 [&](const BVHPrimitiveInfo& lhs, const BVHPrimitiveInfo& rhs) {
											 return Math::GetValueOnAxis(lhs.boundingBox.Center(), axis) < Math::GetValueOnAxis(rhs.boundingBox.Center(), axis);
										 });
					} else {
						constexpr const unsigned int numberOfBuckets = 12;
						BucketInfo buckets[numberOfBuckets] = {};
						for (int i = start; i < end; ++i) {
							int bucket = int(numberOfBuckets *
											 Math::GetValueOnAxis(centroidsBB.Offset(primitiveInfo[i].boundingBox.Center()), axis));

							if (bucket == numberOfBuckets) {
								bucket -= 1;
							}

							buckets[bucket].count++;
							buckets[bucket].bb |= primitiveInfo[i].boundingBox;
						}

						float cost[numberOfBuckets - 1];
						for (int i = 0; i < numberOfBuckets - 1; ++i) {
							Oblivion::BoundingBox b0, b1;
							int count0 = 0, count1 = 0;
							for (int j = 0; j <= i; ++j) {
								b0 |= buckets[j].bb;
								count0 += buckets[j].count;
							}
							for (int j = i + 1; j < numberOfBuckets; ++j) {
								b1 |= buckets[j].bb;
								count1 += buckets[j].count;
							}

							cost[i] = 1.0f + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / bb.SurfaceArea();
						}

						auto minBucket = std::min_element(std::begin(cost), std::end(cost)) - cost;
						auto minCost = cost[minBucket];

						float leafCost = (float)nPrimitives;
						if (nPrimitives > (int)maxPrimitiveInNodes || minCost < leafCost) {
							auto midPoint = std::partition(primitiveInfo.begin() + start, primitiveInfo.begin() + end,
														   [&](const BVHPrimitiveInfo& primitiveInfo) {
															   int bucket = int(numberOfBuckets *
																				Math::GetValueOnAxis(centroidsBB.Offset(primitiveInfo.boundingBox.Center()), axis));
															   if (bucket == numberOfBuckets)
																   bucket -= 1;
															   EVALUATE(bucket >= 0 && bucket < (int)numberOfBuckets, "Something went really wrong? x2 :))");
															   return bucket <= minBucket;
														   });
							mid = int(midPoint - primitiveInfo.begin());
						} else {
							int indexStart = (int)newPrimitives.size();
							for (int i = start; i < end; ++i) {
								auto primitiveIndex = primitiveInfo[i].index;
								newPrimitives.push_back(primitives[primitiveIndex]);
							}
							currentNode->InitAsLeaf(bb, indexStart, nPrimitives);
							return currentNode;
						}
					}
			}
			currentNode->InitAsInterior(axis,
										RecursiveBuild(primitiveInfo, splitMethod, maxPrimitiveInNodes,
													   primitives, newPrimitives, start, mid, numNodes),
										RecursiveBuild(primitiveInfo, splitMethod, maxPrimitiveInNodes,
													   primitives, newPrimitives, mid, end, numNodes));
		}
	}
	return currentNode;
}

void RenderBVHTree(const std::unique_ptr<BVHNode>& root, std::function<void(const Oblivion::BoundingBox& bb)> renderBoundingBox) {

	// renderBoundingBox(root->boundingBox);

	/*if (root->children[0] != nullptr) {
		RenderBVHTree(root->children[0], renderBoundingBox);
	}
	if (root->children[1] != nullptr) {
		RenderBVHTree(root->children[1], renderBoundingBox);
	}*/

	std::queue<std::pair<const std::unique_ptr<BVHNode>&, unsigned int>> boundingBoxes;
	boundingBoxes.push({ root, 0 });

	constexpr const unsigned int MaxDepth = 3;
	while (!boundingBoxes.empty()) {
		auto front = boundingBoxes.front();
		boundingBoxes.pop();
		auto depth = front.second;
		if (depth >= MaxDepth)
			break;

		if (front.first) {
			renderBoundingBox(front.first->boundingBox);
			boundingBoxes.push({ front.first->children[0], depth + 1 });
			boundingBoxes.push({ front.first->children[1], depth + 1 });
		}
	}


}

template <typename primitiveType>
std::shared_ptr<BvhTree> BvhTree::Create(AccelerableStructure<primitiveType>* accelerableStructure, SplitMethod splitType,
										 unsigned int maxPrimitiveInNodes, unsigned int maxPrimitivesInLeaf) {
	auto tree = std::shared_ptr<BvhTree>(new BvhTree);

	auto structurePrimitives = accelerableStructure->GetPrimitives();

	auto primitives = tree->BuildPrimitives(structurePrimitives);

	unsigned int totalNodes = 0;
	decltype(structurePrimitives) newPrimitives;
	newPrimitives.reserve(structurePrimitives.size());
	auto root = tree->RecursiveBuild(primitives, splitType, maxPrimitiveInNodes, structurePrimitives, newPrimitives,
									 0, (int)primitives.size(), &totalNodes);

	int offset = 0;
	tree->mNodes.resize(totalNodes);
	if constexpr (std::is_same_v<primitiveType, ModelPrimitive>) {
		tree->FlattenBVH<MODEL_PRIMITIVE_TYPE>(root, offset, maxPrimitivesInLeaf);
	} else if constexpr (std::is_same_v<primitiveType, ScenePrimitive>) {
		tree->FlattenBVH<SCENE_PRIMITIVE_TYPE>(root, offset, maxPrimitivesInLeaf);
	} else {
		static_assert(false, "Unknown primitive type");
	}

	accelerableStructure->ReorderPrimitives(newPrimitives);

	auto RenderBoundingBox = [&](const Oblivion::BoundingBox& bb) {

		float minX = bb.minPoint.x;
		float minY = bb.minPoint.y;
		float minZ = bb.minPoint.z;

		float maxX = bb.maxPoint.x;
		float maxY = bb.maxPoint.y;
		float maxZ = bb.maxPoint.z;

		// float colorMultiplier = float(ab) / MaxDepth;
		// float colorMultiplier = 1.0f;

		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(minX, minY, minZ), DirectX::XMFLOAT3(maxX, minY, minZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(minX, minY, minZ), DirectX::XMFLOAT3(minX, minY, maxZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(minX, minY, minZ), DirectX::XMFLOAT3(minX, maxY, minZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(maxX, maxY, maxZ), DirectX::XMFLOAT3(minX, maxY, maxZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(maxX, maxY, maxZ), DirectX::XMFLOAT3(maxX, minY, maxZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(maxX, maxY, maxZ), DirectX::XMFLOAT3(maxX, maxY, minZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(minX, maxY, maxZ), DirectX::XMFLOAT3(minX, maxY, minZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(minX, maxY, maxZ), DirectX::XMFLOAT3(minX, minY, maxZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(minX, minY, maxZ), DirectX::XMFLOAT3(maxX, minY, maxZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(maxX, minY, minZ), DirectX::XMFLOAT3(maxX, minY, maxZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(maxX, minY, minZ), DirectX::XMFLOAT3(maxX, maxY, minZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
		tree->mRenderLines.emplace_back(DirectX::XMFLOAT3(maxX, maxY, minZ), DirectX::XMFLOAT3(minX, maxY, minZ), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
	};


	RenderBVHTree(root, RenderBoundingBox);

	return tree;
}


template std::shared_ptr<BvhTree> BvhTree::Create(AccelerableStructure<ModelPrimitive>* accelerableStructure,
												  BvhTree::SplitMethod splitType, unsigned int maxPrimitiveInNodes, unsigned int maxPrimitivesInLeaf);
template std::shared_ptr<BvhTree> BvhTree::Create(AccelerableStructure<ScenePrimitive>* accelerableStructure,
												  BvhTree::SplitMethod splitType, unsigned int maxPrimitiveInNodes, unsigned int maxPrimitivesInLeaf);
