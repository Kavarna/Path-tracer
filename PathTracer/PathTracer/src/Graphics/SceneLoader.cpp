#include "SceneLoader.h"
#include "Optimizations/BvhTree.h"
#include "../Utils/Threading.h"
#include "../Common/Limits.h"
#include "Direct3D.h"

SceneLoader::SceneLoader(const std::vector<std::string>& inputFiles) : mInputFiles(inputFiles) {
}

void SceneLoader::Load(ComPtr<ID3D12GraphicsCommandList> cmdList) {

    for (const auto& it : mInputFiles) {
        LoadFile(it, cmdList);
    }

    EVALUATE(mSpheres.size() < MAX_SPHERES, "Too many spheres provided (", mSpheres.size(), " >= ", MAX_SPHERES, ")");
    // EVALUATE(mLines.size() < MAX_LINES, "Too many lines provided (%lld >= %d)", mLines.size(), MAX_LINES);

    CentralizeModels();
    BuildBuffers(cmdList);
    BuildTextures(cmdList);
}

void SceneLoader::ResetIntermediaryBuffer() {
    if (mSkybox) {
        mSkybox->ResetIntermediaryBuffer();
    }
    if (mSceneTreeTexture) {
        mSceneTreeTexture->ResetIntermediaryBuffer();
    }
    if (mModelTreesTexture) {
        mModelTreesTexture->ResetIntermediaryBuffer();
    }
    if (mModelPrimitivesTexture) {
        mModelPrimitivesTexture->ResetIntermediaryBuffer();
    }
    if (mScenePrimitivesTexture) {
        mScenePrimitivesTexture->ResetIntermediaryBuffer();
    }
    if (mVertexBufferTexture) {
        mVertexBufferTexture->ResetIntermediaryBuffer();
    }
    if (mMaterialsTexture) {
        mMaterialsTexture->ResetIntermediaryBuffer();
    }
    if (mTextures) {
        mTextures->ResetIntermediaryBuffer();
    }
}

std::shared_ptr<UploadBuffer<LinesCB>> SceneLoader::GetLinesCB() const {
    return mLinesCB;
}

std::shared_ptr<Skymap> SceneLoader::GetSkybox() const {
    return mSkybox;
}

void SceneLoader::BindScene(ID3D12DescriptorHeap* heap, std::size_t& offset) {

    mSpheresCB->CreateViewInHeap(heap, offset);
    offset += mSpheresCB->GetHeapUsedSize();

    mLinesCB->CreateViewInHeap(heap, offset);
    offset += mLinesCB->GetHeapUsedSize();

    mSceneTreeTexture->CreateViewInHeap(heap, offset);
    offset += mSceneTreeTexture->GetHeapUsedSize();

    mModelTreesTexture->CreateViewInHeap(heap, offset);
    offset += mModelTreesTexture->GetHeapUsedSize();

    mScenePrimitivesTexture->CreateViewInHeap(heap, offset);
    offset += mScenePrimitivesTexture->GetHeapUsedSize();

    mModelPrimitivesTexture->CreateViewInHeap(heap, offset);
    offset += mModelPrimitivesTexture->GetHeapUsedSize();

    mVertexBufferTexture->CreateViewInHeap(heap, offset);
    offset += mVertexBufferTexture->GetHeapUsedSize();

    mMaterialsTexture->CreateViewInHeap(heap, offset);
    offset += mMaterialsTexture->GetHeapUsedSize();

    if (mTextures) {
        mTextures->CreateViewInHeap(heap, offset);
        offset += mTextures->GetHeapUsedSize();
    } else {
        offset += Direct3D::Get()->GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    mLightsCB->CreateViewInHeap(heap, offset);
    offset += mLightsCB->GetHeapUsedSize();
}

std::shared_ptr<UploadBuffer<SpheresCB>> SceneLoader::GetSpheresCB() const {
    return mSpheresCB;
}

void SceneLoader::LoadFile(const std::string& path, ComPtr<ID3D12GraphicsCommandList> cmdList) {

    std::string jsonExtension = ".json";
    if (boost::algorithm::ends_with(path, jsonExtension)) {
        LoadJSON(path, cmdList);
    }
}

void SceneLoader::LoadSkybox(const std::string& path, const boost::property_tree::ptree&pt, ComPtr<ID3D12GraphicsCommandList> cmdList) {

    auto optionalSkybox = pt.get_child_optional("Skybox");
    if (optionalSkybox.has_value()) {
        TRY_PRINT_ERROR_AND_MESSAGE(
            {
                std::string skyboxPathString = optionalSkybox->get_value<std::string>();
                mSkybox = std::make_shared<Skymap>(skyboxPathString, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
                                                    cmdList, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                Oblivion::DebugPrintLine("Successfully loaded skybox from: ", path.c_str());
            }, "Unable to load skybox from input file: %s", path.c_str());
    }

}

void SceneLoader::LoadSpheres(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList) {
    
    // Maybe improve this?
    auto spheresOptional = pt.get_child_optional("Spheres");
    if (spheresOptional.has_value()) {
        auto& spheres = *spheresOptional;
        mSpheres.reserve(spheres.size());
        for (auto it = spheres.begin(); it != spheres.end(); ++it) {
            auto& currentSphere = it->second;

            auto position = currentSphere.get_child("Position").get_value<DirectX::XMFLOAT3>();
            auto radius = currentSphere.get_child("Radius").get_value<float>();
            auto color = currentSphere.get_child("Color").get_value<DirectX::XMFLOAT4>();
            auto lightPropertiesOptional = currentSphere.get_child_optional("Light Properties");
            DirectX::XMFLOAT2 lightProperties;
            if (lightPropertiesOptional.has_value()) {
                lightProperties = lightPropertiesOptional.get().get_value<DirectX::XMFLOAT2>();
            } else {
                lightProperties = { 0.0f,0.0f };
            }

            mSpheres.emplace_back(position, radius, color, lightProperties);
        }
    }

}

void SceneLoader::LoadLines(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList) {
    auto linesOptional = pt.get_child_optional("Lines");
    if (linesOptional.has_value()) {
        auto& lines = *linesOptional;
        mLines.reserve(lines.size());
        for (auto it = lines.begin(); it != lines.end(); ++it) {
            auto& currentLine = it->second;

            auto start = currentLine.get_child("Start").get_value<DirectX::XMFLOAT3>();
            auto end = currentLine.get_child("End").get_value<DirectX::XMFLOAT3>();
            auto color = currentLine.get_child("Color").get_value<DirectX::XMFLOAT4>();

            mLines.emplace_back(start, end, color);
        }
    }
}

void SceneLoader::LoadLights(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList) {
    auto lightsOptional = pt.get_child_optional("Lights");
    if (lightsOptional.has_value()) {
        auto& lights = lightsOptional.get();
        for (auto it = lights.begin(); it != lights.end(); ++it) {
            auto& currentLight = it->second;

            auto position = currentLight.get_child("Position").get_value<DirectX::XMFLOAT3>();
            auto radius = currentLight.get_child("Radius").get_value<float>();
            auto emission = currentLight.get_child("Emissive").get_value<DirectX::XMFLOAT4>();

            mLights.emplace_back(position, emission, radius);
        }
    }
}

void SceneLoader::LoadMaterials(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList) {
    auto materials = pt.get_child("Material");

    mMaterials.reserve(materials.size());
    for (auto it = materials.begin(); it != materials.end(); ++it) {
        auto& currentMaterial = it->second;
        std::string materialName = it->first;

        if (mMaterialNameToMaterialIndex.find(materialName) != mMaterialNameToMaterialIndex.end()) {
            Oblivion::DebugPrintLine("Material ", materialName, " already loaded... Skipping");
            break;
        }

        Oblivion::DebugPrintLine("Loading material ", materialName);

        Material m;

        auto diffuseOptional = currentMaterial.get_child_optional("Diffuse");
        if (diffuseOptional.has_value()) {
            auto color = diffuseOptional->get_value<DirectX::XMFLOAT3>();
            m.diffuseColor = DirectX::XMFLOAT4(color.x, color.y, color.z, 1.0f);
        }

        auto emissiveOptional = currentMaterial.get_child_optional("Emissive");
        if (emissiveOptional.has_value()) {
            auto color = emissiveOptional->get_value<DirectX::XMFLOAT3>();
            m.emissiveColor = DirectX::XMFLOAT4(color.x, color.y, color.z, 1.0f);
        }

        auto metallicOptional = currentMaterial.get_child_optional("Metallic");
        if (metallicOptional.has_value()) {
            m.metallic = metallicOptional->get_value<float>();
        }

        auto roughnessOptional = currentMaterial.get_child_optional("Roughness");
        if (roughnessOptional.has_value()) {
            m.roughness = roughnessOptional->get_value<float>();
        }

        auto iorOptional = currentMaterial.get_child_optional("IoR");
        if (iorOptional.has_value()) {
            m.ior = iorOptional->get_value<float>();
        }

        auto textureNameOptional = currentMaterial.get_child_optional("Diffuse Texture");
        if (textureNameOptional.has_value()) {
            m.textureIndex = mTexturesToLoad.size();
            std::string path = std::filesystem::absolute(std::filesystem::path(textureNameOptional->get_value<std::string>())).string();
            mTexturesToLoad.push_back(path);
        }

        auto materialTypeOptional = currentMaterial.get_child_optional("Material Type");
        if (materialTypeOptional.has_value()) {
            MaterialType type;
            auto materialType = materialTypeOptional->get_value<std::string>();
            if (boost::iequals(materialType, "diffuse")) {
                type = MaterialType::Diffuse;
            } else if (boost::iequals(materialType, "specular")) {
                type = MaterialType::Specular;
            } else {
                EVALUATE(false, "Material type: ", materialType, " does not exist. Try: \"diffuse\" or \"specular\"");
            }
            m.materialType = type;
        }

        mMaterialNameToMaterialIndex.insert({ materialName, (unsigned int)mMaterials.size() });
        mMaterials.push_back(m);
    }
    
}

void SceneLoader::LoadModels(const std::string& path, const boost::property_tree::ptree& pt, ComPtr<ID3D12GraphicsCommandList> cmdList) {

    // Todo: split this in LoadModels & BuildModels & AggregateModels

    auto acceleratedStructuresOptional = pt.get_child_optional("AcceleratedModel");
    if (!acceleratedStructuresOptional.has_value()) {
        return;
    }

    auto& acceleratedStructures = *acceleratedStructuresOptional;
    if (acceleratedStructures.size() == 0) {
        return;
    }
    Oblivion::DebugPrintLine("Preparing to load ", acceleratedStructures.size(), " models");
    mModelsInfo.reserve(acceleratedStructures.size() + mModelsInfo.size());
    for (auto it = acceleratedStructures.begin(); it != acceleratedStructures.end(); ++it) {
        auto& currentModel = it->second;

        auto path = currentModel.get_child("Path").get_value<std::string>();
        auto maxPrimitivesInNode = currentModel.get_child("MaxPrimitivesInNode").get_value<unsigned int>();
        
        auto splitMethod = BvhTree::GetSplitMethodByName(currentModel.get_child("SplitMethod").get_value<std::string>());
        EVALUATE(splitMethod.has_value(), "Invalid split method for mesh at index ", mModelsInfo.size() + 1);

        bool wireframeRender = false;
        auto wireframeRenderOptional = currentModel.get_child_optional("WireframeRender");
        if (wireframeRenderOptional.has_value()) {
            wireframeRender = wireframeRenderOptional.get().get_value<bool>();
        }

        bool bvhRender = false;
        auto bvhRenderOptional = currentModel.get_child_optional("BvhRender");
        if (bvhRenderOptional.has_value()) {
            bvhRender = bvhRenderOptional.get().get_value<bool>();
        }

        auto materialName = currentModel.get_child("Material").get_value<std::string>();

        path = std::filesystem::absolute(std::filesystem::path(path)).string();
        mModelsInfo.emplace_back(path, *splitMethod, maxPrimitivesInNode, wireframeRender, bvhRender, materialName);
    }

    try {
        auto sceneSplitMethod = BvhTree::GetSplitMethodByName(pt.get_child("SceneAcceleration").get_value<std::string>());
        EVALUATE(sceneSplitMethod.has_value());
        mSceneSplit = *sceneSplitMethod;
    } catch (...) {
        Oblivion::DebugPrintLine("Cannot covnert to scene split method. Using default = SAH");
        mSceneSplit = BvhTree::SplitMethod::SAH;
    }
}

void SceneLoader::CentralizeModels() {
    Oblivion::DebugPrintLine("Start loading ", mModelsInfo.size(), " models");
    std::vector<std::unique_ptr<Model>> models;
    models.resize(mModelsInfo.size());
    std::mutex linesMutex;
    std::atomic<unsigned int> totalVertices = 0;
    Threading::Get()->ParralelForImmediate(
        [&](int64_t index) {
            const auto& constructionInfo = mModelsInfo.at(index);
            auto model = std::make_unique<Model>(constructionInfo.path);
            totalVertices += model->GetVertexCount();
            if (constructionInfo.wireframeRender) {
                const auto& renderLines = model->GetRenderLines();
                std::unique_lock<std::mutex> lock(linesMutex);
                std::move(renderLines.begin(), renderLines.end(), std::back_inserter(mLines));
            }
            models[index] = std::move(model);
        }, mModelsInfo.size(), 1);

    Oblivion::DebugPrintLine("Finished loading ", mModelsInfo.size(), " models. Start optimizing . . .");
    std::vector<std::shared_ptr<BvhTree>> bvhTrees;
    bvhTrees.resize(mModelsInfo.size());
    Threading::Get()->ParralelForImmediate(
        [&](int64_t index) {
            const auto& currentModel = models.at(index);
            const auto& constructionInfo = mModelsInfo.at(index);
            auto bvhTree = BvhTree::Create(currentModel.get(), constructionInfo.splitMethod, constructionInfo.maxPrimitivesInNode, 65536);
            if (constructionInfo.bvhRender) {
                const auto& renderLines = bvhTree->GetRenderLines();
                std::unique_lock<std::mutex> lock(linesMutex);
                std::move(renderLines.begin(), renderLines.end(), std::back_inserter(mLines));
            }
            bvhTrees[index] = std::move(bvhTree);
        }, models.size(), 1);

    {
        unsigned int totalPrimitives = 0;
        Oblivion::DebugPrintLine("Centralizing Vertex & Index Buffers & Primitives & Materials");
        mVertexBuffer.reserve(totalVertices);
        for (unsigned int i = 0; i < models.size(); ++i) {
            auto& model = models[i];
            auto& constructionInfo = mModelsInfo.at(i);

            auto& currentVertexBuffer = model->GetVertices();

            auto& bvhTree = bvhTrees[i]->GetNodes();
            for (auto& node : bvhTree) {
                if (node.numberOfPrimitives != 0) {
                    // It's a leaf
                    node.primitiveOffset += (unsigned int)mModelPrimitives.size();
                }
            }

            for (auto& vertex : currentVertexBuffer) {
                vertex.materialIndex = mMaterialNameToMaterialIndex[constructionInfo.usedMaterialName];
            }

            auto modelPrimitives = model->GetPrimitives();
            mModelPrimitives.reserve(modelPrimitives.size() + mModelPrimitives.size());
            for (auto primitive : modelPrimitives) {
                primitive->indices[0] += (unsigned int)mVertexBuffer.size();
                primitive->indices[1] += (unsigned int)mVertexBuffer.size();
                primitive->indices[2] += (unsigned int)mVertexBuffer.size();
                mModelPrimitives.push_back(*primitive);
            }

            std::move(currentVertexBuffer.begin(), currentVertexBuffer.end(), std::back_inserter(mVertexBuffer));
        }
    }

    {
        Oblivion::DebugPrintLine("Building scene BVH");
        auto scene = std::make_unique<Scene>(bvhTrees);
        auto sceneBvh = BvhTree::Create(scene.get(), mSceneSplit, 5, 65536);
        mSceneTree = std::move(sceneBvh->GetNodes());
        auto scenePrimitives = scene->GetPrimitives();
        mScenePrimitives.reserve(scenePrimitives.size());
        for (const auto it : scenePrimitives) {
            mScenePrimitives.push_back(*it.get());
        }

        Oblivion::DebugPrintLine("Centralizing Scene BVH and Models BVH");
        mSceneTree; // Already centralized from before

        Oblivion::DebugPrintLine("Scene tree: ", mSceneTree);
        mModelTrees = std::move(scene->GetModelTree());
    }
}

void SceneLoader::BuildBuffers(ComPtr<ID3D12GraphicsCommandList> cmdList) {
    mSpheresCB = std::make_shared<UploadBuffer<SpheresCB>>(1, true);
    SpheresCB sphereBufferInfo = {};
    memcpy(sphereBufferInfo.spheres, mSpheres.data(), sizeof(Sphere) * mSpheres.size());
    sphereBufferInfo.numSpheres = (unsigned int)mSpheres.size();
    mSpheresCB->CopyData(&sphereBufferInfo, 1);

    mLinesCB = std::make_shared<UploadBuffer<LinesCB>>(1, true);
    unsigned int totalLines = std::min((unsigned int)mLines.size(), (unsigned int)MAX_LINES);
    LinesCB linesBufferInfo = {};
    memcpy(linesBufferInfo.lines, mLines.data(), sizeof(Line) * totalLines);
    linesBufferInfo.numLines = totalLines;
    mLinesCB->CopyData(&linesBufferInfo, 1);

    mLightsCB = std::make_shared<UploadBuffer<LightsCB>>(1, true);
    unsigned int totalLights = std::min((unsigned int)mLights.size(), (unsigned int)MAX_LIGHTS);
    LightsCB lightsBufferInfo = {};
    memcpy(lightsBufferInfo.lights, mLights.data(), sizeof(Light) * totalLights);
    lightsBufferInfo.numLights = totalLights;
    mLightsCB->CopyData(&lightsBufferInfo, 1);
}

void SceneLoader::BuildTextures(ComPtr<ID3D12GraphicsCommandList> cmdList) {
    // We should have enough space to copy everything from arrays to texture
    // The texture has only float4 elements => we need to allocate numberOfElements * (sizeof(element) / sizeof(float4))
    // sizeof(element) is guaranteed divisible by sizeof(float4) <- 16 aligned

    auto CreateTexture =
        [&](auto dataArray, const char* description) -> std::unique_ptr<Texture> {
            static_assert(sizeof(decltype(dataArray)::value_type) % 16 == 0, "Arguments for this function should be 16 aligned");
            if (dataArray.size() == 0) {
                return nullptr;
            }
            unsigned int dataSize = (unsigned int)dataArray.size() * (sizeof(dataArray[0]) / sizeof(float[4]));
            unsigned int numRows = dataSize / MAX_TEXTURE_COLUMNS + 1;
            unsigned int numCols;
            if (numRows > 1) {
                numCols = MAX_TEXTURE_COLUMNS;
                unsigned int newSize;

                // dataSize = numRows * numCols
                // numRows * numCols = dataArray.size() * (sizeof(dataArray[0]) / sizeof(float[4]))
                // (numRows * numCols) / (sizeof(dataArray[0]) / sizeof(float[4])) = dataArray.size()
                // dataArray.size() = (numRows * numCols * sizeof(float[4])) / sizeof(dataArray[0]
                newSize = (numRows * numCols * sizeof(float[4])) / sizeof(dataArray[0]);

                dataArray.resize(newSize);

            } else {
                numCols = dataSize;
            }
            Oblivion::DebugPrintLine("Creating texture with ", numCols, " * ", numRows, " pixels", " for ", description,
                                     " with ", sizeof(dataArray[0]) / sizeof(float[4]), " texels per structure");
            return std::make_unique<Texture>(numCols, numRows, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                             D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                             cmdList, (unsigned char*)dataArray.data());
        };

     mSceneTreeTexture = CreateTexture(mSceneTree, "scene tree");
     mScenePrimitivesTexture = CreateTexture(mScenePrimitives, "scene primitives");
     mModelTreesTexture = CreateTexture(mModelTrees, "scene models tree");
     mModelPrimitivesTexture = CreateTexture(mModelPrimitives, "models' primitives");
     mVertexBufferTexture = CreateTexture(mVertexBuffer, "vertex buffer");
     mMaterialsTexture = CreateTexture(mMaterials, "materials");

     Oblivion::DebugPrintLine("Materials present in scene: ", mMaterials);
     Oblivion::DebugPrintLine("Vertices: ", mVertexBuffer);
     Oblivion::DebugPrintLine("Model tree: ", mModelTrees);


     if (mTexturesToLoad.size() > 0) {
         Oblivion::DebugPrintLine("Preparing to load ", mTexturesToLoad.size(), " textures & centralizing them");
         TRY_PRINT_ERROR({ mTextures = std::make_unique<Texture>(mTexturesToLoad, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
                                               cmdList, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        });
     }
}

void SceneLoader::LoadJSON(const std::string& path, ComPtr<ID3D12GraphicsCommandList> cmdList) {
    using namespace boost::property_tree;

    boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json(path, pt);

    auto oldCwd = std::filesystem::current_path();

    std::filesystem::path cwd = path;
    std::filesystem::current_path(cwd.parent_path());

    mVersion = pt.get_child("Version").get_value<unsigned int>();
    EVALUATE(mVersion == 1, "Invalid version for this build (", mVersion, "). Maximum supported = 1");

    LoadSkybox(path, pt, cmdList);
    LoadSpheres(path, pt, cmdList);
    LoadLines(path, pt, cmdList);
    LoadLights(path, pt, cmdList);
    LoadMaterials(path, pt, cmdList);
    LoadModels(path, pt, cmdList);

    std::filesystem::current_path(oldCwd);

    Oblivion::DebugPrintLine("Finished loading file: ", path);
}

