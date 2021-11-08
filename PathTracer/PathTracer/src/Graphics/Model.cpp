#include "Model.h"

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

Oblivion::BoundingBox ModelPrimitive::GetBoundingBox() {
	return boundingBox;
}

Model::Model(const std::string& path) {
	Oblivion::DebugPrintLine("Loading model from ", path);

	auto objectImporter = std::make_shared<Assimp::Importer>();
	auto scene = objectImporter->ReadFile(path,
										  // aiPostProcessSteps::aiProcess_CalcTangentSpace |
										  aiPostProcessSteps::aiProcess_GenNormals |
										  aiPostProcessSteps::aiProcess_FlipWindingOrder |
										  aiPostProcessSteps::aiProcess_MakeLeftHanded |
										  aiPostProcessSteps::aiProcess_Triangulate | aiProcess_SortByPType | 
										  aiPostProcessSteps::aiProcess_OptimizeMeshes);
	EVALUATE(scene, "Unable to load scene from ", path);

	scene->mRootNode;

	ProcessMesh(scene, scene->mRootNode,
				[&](const aiMesh* currentMesh) {
					auto indexOffset = CopyVertices(currentMesh);
					BuildPrimitives(currentMesh, indexOffset);
					
					Oblivion::DebugPrintLine("Processing mesh ", currentMesh->mName.C_Str());

					for (unsigned int i = 0; i < currentMesh->mNumFaces; ++i) {
						auto currentFace = currentMesh->mFaces[i];
						mRenderLines.emplace_back(currentMesh->mVertices[currentFace.mIndices[0]],
												  currentMesh->mVertices[currentFace.mIndices[1]], DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
						mRenderLines.emplace_back(currentMesh->mVertices[currentFace.mIndices[1]],
												  currentMesh->mVertices[currentFace.mIndices[2]], DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
						mRenderLines.emplace_back(currentMesh->mVertices[currentFace.mIndices[0]],
												  currentMesh->mVertices[currentFace.mIndices[2]], DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
					}

				});

}

unsigned int Model::CopyVertices(const struct aiMesh* mesh) {

	unsigned int initialSize = (unsigned int)mVertices.size();

	mVertices.reserve(initialSize + mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		TraceVertex newVertex;
		newVertex.position = mesh->mVertices[i];
		newVertex.normal = mesh->mNormals[i];
		newVertex.materialIndex = mesh->mMaterialIndex;

		DirectX::XMFLOAT4 texCoords;
		if (mesh->HasTextureCoords(0)) {
			texCoords = mesh->mTextureCoords[0][i];
		} else {
			if (mesh->HasVertexColors(0)) {
				texCoords = mesh->mColors[0][i];
			} else {
				texCoords = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
			}
		}
		newVertex.texCoords = texCoords;

		mVertices.push_back(newVertex);
	}

	return initialSize;
}

void Model::BuildPrimitives(const struct aiMesh* mesh, unsigned int indexOffset) {

	auto initialSize = mPrimitives.size();
	mPrimitives.reserve(initialSize + mesh->mNumFaces);

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		auto& currentFace = mesh->mFaces[i];
		EVALUATE(currentFace.mNumIndices == 3, "Invalid number of indices found in a model");

		if (currentFace.mNumIndices == 3) {
			// Normal case
			auto primitive = std::make_shared<ModelPrimitive>();
			for (unsigned int j = 0; j < currentFace.mNumIndices; ++j) {
				auto& currentIndex = currentFace.mIndices[j];

				((ModelPrimitive*)primitive.get())->indices[j] = currentIndex + indexOffset;
				((ModelPrimitive*)primitive.get())->boundingBox |= mesh->mVertices[currentIndex];
			}

			mPrimitives.push_back(primitive);
		}
	}

}

void Model::ProcessMesh(const aiScene* scene, const aiNode* rootNode, std::function<void(const aiMesh*)> callback) {

	for (unsigned int i = 0; i < rootNode->mNumMeshes; ++i) {
		callback(scene->mMeshes[rootNode->mMeshes[i]]);
	}

	for (unsigned int i = 0; i < rootNode->mNumChildren; ++i) {
		ProcessMesh(scene, rootNode->mChildren[i], callback);
	}
}

std::vector<std::shared_ptr<ModelPrimitive>>& Model::GetPrimitives() {
	return mPrimitives;
}

void Model::ReorderPrimitives(std::vector<std::shared_ptr<ModelPrimitive>>& orderedPrimitives) {
	EVALUATE(orderedPrimitives.size() == mPrimitives.size(), "Expected new primitives to be exactly the same size (",
			 mPrimitives.size(), "), but they aren't (", orderedPrimitives.size(), ")");

	mPrimitives = std::move(orderedPrimitives);
}

const std::vector<Line>& Model::GetRenderLines() const {
	return mRenderLines;
}

std::vector<TraceVertex>& Model::GetVertices() {
	return mVertices;
}

unsigned int Model::GetIndexCount() {
	return (unsigned int)mPrimitives.size() * 3;
}

unsigned int Model::GetVertexCount() {
	return (unsigned int)mVertices.size();
}
