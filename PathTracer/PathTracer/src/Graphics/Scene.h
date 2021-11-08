#pragma once


#include <Oblivion.h>
#include "Optimizations/Interfaces/AccelerableStructure.h"
#include "ShaderObjects.h"
#include "Optimizations/BvhTree.h"

struct ScenePrimitive {
	unsigned int treeOffset;
	Oblivion::BoundingBox boundingBox;

	Oblivion::BoundingBox GetBoundingBox();

	ScenePrimitive(unsigned int treeOffset, const Oblivion::BoundingBox& bb) :
		treeOffset(treeOffset), boundingBox(bb) {
	}

	operator TraceScenePrimitive() {
		TraceScenePrimitive tp;
		
		tp.minAABB = boundingBox.minPoint;
		tp.maxAABB = boundingBox.maxPoint;
		tp.modelOffset = treeOffset;
		
		return tp;
	}
};



class Scene : public AccelerableStructure<ScenePrimitive> {

public:
	Scene(std::vector<std::shared_ptr<BvhTree>>& treeModels);

public: 
	// Inherited via AccelerableStructure
	virtual std::vector<std::shared_ptr<ScenePrimitive>>& GetPrimitives() override;

	virtual void ReorderPrimitives(std::vector<std::shared_ptr<ScenePrimitive>>&) override;

	std::vector<BVHTreeNode>& GetModelTree();

private:
	std::vector<std::shared_ptr<ScenePrimitive>> mPrimitives;

	std::vector<BVHTreeNode> mModels;
};
