#pragma once


#include <Oblivion.h>
#include "Utils/UploadBuffer.h"
#include "GraphicsObject.h"
#include "ShaderObjects.h"
#include "Skymap.h"
#include "Scene.h"
#include "Model.h"
#include "Texture.h"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/algorithm/string/predicate.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class SceneLoader : public GraphicsObject {

public:
    SceneLoader(const std::vector<std::string>& inputFiles);

public:
    void Load(ComPtr<ID3D12GraphicsCommandList> cmdList);
    void ResetIntermediaryBuffer();

    std::shared_ptr<UploadBuffer<SpheresCB>> GetSpheresCB() const;
    std::shared_ptr<UploadBuffer<LinesCB>> GetLinesCB() const;
    std::shared_ptr<Skymap> GetSkybox() const;

    void BindScene(ID3D12DescriptorHeap* heap, std::size_t& offset);

private:
    void LoadFile(const std::string& path, ComPtr<ID3D12GraphicsCommandList> cmdList);

    void LoadJSON(const std::string& path, ComPtr<ID3D12GraphicsCommandList> cmdList);

private:
    void LoadSkybox(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList);
    void LoadSpheres(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList);
    void LoadLines(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList);
    void LoadLights(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList);
    void LoadMaterials(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList);
    void LoadModels(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList);

private:
    void CentralizeModels();
    void BuildBuffers(ComPtr<ID3D12GraphicsCommandList> cmdList);
    void BuildTextures(ComPtr<ID3D12GraphicsCommandList> cmdList);

private:
    struct AcceleratedStructureInfo {
        std::string path;
        BvhTree::SplitMethod splitMethod;
        unsigned int maxPrimitivesInNode;
        bool wireframeRender;
        bool bvhRender;
        std::string usedMaterialName;
        AcceleratedStructureInfo(std::string path, BvhTree::SplitMethod splitMethod, unsigned int maxPrimitivesInNode,
                                 bool wireframeRender, bool bvhRender, std::string usedMaterialName) :
            path(std::move(path)), splitMethod(splitMethod), maxPrimitivesInNode(maxPrimitivesInNode),
            wireframeRender(wireframeRender), bvhRender(bvhRender), usedMaterialName(std::move(usedMaterialName)) {};
    };


private:
    std::vector<std::string> mInputFiles;
    
    std::vector<Sphere> mSpheres;
    std::vector<Line> mLines;
    std::vector<Light> mLights;

    std::vector<TraceVertex> mVertexBuffer;
    std::unique_ptr<Texture> mVertexBufferTexture;

    std::vector<AcceleratedStructureInfo> mModelsInfo;
    BvhTree::SplitMethod mSceneSplit = BvhTree::SplitMethod::SAH;

    std::vector<TraceModelPrimitive> mModelPrimitives;
    std::unique_ptr<Texture> mModelPrimitivesTexture;
    std::vector<TraceScenePrimitive> mScenePrimitives;
    std::unique_ptr<Texture> mScenePrimitivesTexture;

    std::vector<BVHTreeNode> mSceneTree;
    std::unique_ptr<Texture> mSceneTreeTexture;
    std::vector<BVHTreeNode> mModelTrees;
    std::unique_ptr<Texture> mModelTreesTexture;

    std::unordered_map<std::string, unsigned int> mMaterialNameToMaterialIndex;
    std::vector<Material> mMaterials;
    std::unique_ptr<Texture> mMaterialsTexture;

    std::vector<std::string> mTexturesToLoad;
    std::unique_ptr<Texture> mTextures;

    std::shared_ptr<UploadBuffer<SpheresCB>> mSpheresCB;
    std::shared_ptr<UploadBuffer<LinesCB>> mLinesCB;
    std::shared_ptr<UploadBuffer<LightsCB>> mLightsCB;

    std::shared_ptr<Skymap> mSkybox;

    unsigned int mVersion = 0;
};

