#pragma once


#include <Oblivion.h>
#include "Optimizations/Interfaces/AccelerableStructure.h"
#include "ShaderObjects.h"

namespace Assimp {
    // =======================================================================
    // Public interface to Assimp
    class Importer;
} //! namespace Assimp

struct ModelPrimitive {
	std::array<unsigned int, 3> indices;
	Oblivion::BoundingBox boundingBox;


	Oblivion::BoundingBox GetBoundingBox();

	operator TraceModelPrimitive() {
		TraceModelPrimitive mp;
		mp.index0 = indices[0];
		mp.index1 = indices[1];
		mp.index2 = indices[2];

		return mp;
	}
};

class Model : public AccelerableStructure<ModelPrimitive> {
public:

	Model() = default;
	Model(const std::string& path);

	// Inherited via AccelerableStructure
	virtual std::vector<std::shared_ptr<ModelPrimitive>>& GetPrimitives() override;
	virtual void ReorderPrimitives(std::vector<std::shared_ptr<ModelPrimitive>>&) override;

	const std::vector<Line>& GetRenderLines() const;
	std::vector<TraceVertex>& GetVertices();

	unsigned int GetIndexCount();
	unsigned int GetVertexCount();

private:
	unsigned int CopyVertices(const struct aiMesh* mesh);
	void BuildPrimitives(const struct aiMesh* mesh, unsigned int indexOffset);

private:
	void ProcessMesh(const struct aiScene* scene, const struct aiNode* rootNode, std::function<void(const struct aiMesh*)> callback);

private:
	std::vector<std::shared_ptr<ModelPrimitive>> mPrimitives;

	std::vector<TraceVertex> mVertices;

	std::vector<Line> mRenderLines;

};

