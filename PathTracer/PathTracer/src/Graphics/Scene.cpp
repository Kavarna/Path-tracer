#include "Scene.h"

Oblivion::BoundingBox ScenePrimitive::GetBoundingBox() {
	return boundingBox;
}

Scene::Scene(std::vector<std::shared_ptr<BvhTree>>& treeModels) {

	unsigned int totalNodes = 0;
	for (const auto& it : treeModels) {
		totalNodes += (unsigned int)it->GetNodes().size();
	}

	mModels.reserve(totalNodes);
	mPrimitives.reserve(treeModels.size());
	for (unsigned int i = 0; i < treeModels.size(); ++i) {
		unsigned int currentOffset = (unsigned int)mModels.size();

		auto& nodes = treeModels[i]->GetNodes();
		for (auto& node : nodes) {
			if (node.secondChildOffset != 0) {
				node.secondChildOffset += (unsigned int)mModels.size();
			}
		}
		std::copy(nodes.begin(), nodes.end(), std::back_inserter(mModels));

		mPrimitives.push_back(std::make_shared<ScenePrimitive>(currentOffset, treeModels[i]->GetBoundingBox()));
	}

}

std::vector<std::shared_ptr<ScenePrimitive>>& Scene::GetPrimitives() {
	return mPrimitives;
}

void Scene::ReorderPrimitives(std::vector<std::shared_ptr<ScenePrimitive>>& orderedPrimitives) {
	EVALUATE(orderedPrimitives.size() == mPrimitives.size(), "Expected new primitives to be exactly the same size (",
			 mPrimitives.size(), "), but they aren't (", orderedPrimitives.size(), ")");

	mPrimitives = std::move(orderedPrimitives);
}

std::vector<BVHTreeNode>& Scene::GetModelTree() {
	return mModels;
}
