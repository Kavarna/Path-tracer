#pragma once


#include <Oblivion.h>
#include "Graphics/Utils/QueueManager.h"
#include "Graphics/Utils/UploadBuffer.h"
#include "Graphics/RasterizedModel.h"
#include "Graphics/Texture.h"
#include "Graphics/SceneLoader.h"
#include "Gameplay/Camera.h"

enum class OblivionMode {
    Debug, None
};

struct OblivionInitialization {
    unsigned int numSamples;
    OblivionMode applicationMode;
    std::vector<std::string> inputFiles;
    std::string outputFile;
    std::string configFile;
    float maxSecondsPerFrame;
};


class Application : public ISingletone<Application> {
    MAKE_SINGLETONE_CAPABLE(Application);
    constexpr static const unsigned int BufferCount = 3;
    constexpr static const unsigned int MaxDescriptorCount = 14;
private:
    Application(HINSTANCE hInstance, const OblivionInitialization& initData);
    ~Application();
    
private:
    void InitFromConfigFile(const std::string& path);
    void InitFromDefaultConfigurations(const std::string& path);
    void WriteConfigurationsToFile(const std::string& path);

    void InitWindow(HINSTANCE hInstance);
    void InitD3D();
    void InitRenderingPipeline();
    void InitRaytracingPipeline();
    void InitPathTracingPipeline();
    void InitModels();
    void InitImgui();
    void InitGameplayObjects();

private:
    void OnCreate(const EventArgs& eventArgs);
    void OnResize(const ResizeEventArgs& eventArgs);
    void OnKeyPressed(const KeyEventArgs& eventArgs);
    void OnKeyReleased(const KeyEventArgs& eventArgs);
    void OnMouseMoved(const MouseMotionEventArgs& eventArgs);

private:
    void UpdateRenderTargetViews();

public:
    void Run();

private:
    void Update();
    void Render();
    
private:
    void Trace();
    void RenderResult();
    void RenderGui(ID3D12GraphicsCommandList* cmdList);

private:
    HWND mWindow;

    bool mUseVsync = false;

    std::unique_ptr<QueueManager> mDirectCommandQueue;
    std::unique_ptr<QueueManager> mComputeCommandQueue;
    ComPtr<IDXGISwapChain4> mSwapchain;

    ComPtr<ID3D12DescriptorHeap> mRTVHeap;
    std::array<ComPtr<ID3D12Resource1>, BufferCount>  mRenderTargetViews;

    bool mTearingSupport;

    UINT mCurrentBackBufferIndex;

    struct InternalMessage {
        unsigned int message_id;
        WPARAM wParam;
        LPARAM lParam;
    };

    std::queue<InternalMessage> mInternalMessageQueue;

    std::set<KeyCode::Key> mPressedKeys;

    struct {
        float xRelativeMoved;
        float yRelativeMoved;
    } mMouseState;

private: // Rendering pipeline

    struct RenderingCB {
        unsigned int applyGamma = 0;
        float invGamma = 1.0f;
        unsigned int numPasses;
    };

    RenderingCB mRenderingCB;

    ComPtr<ID3DBlob> mSimpleVertexShader;
    ComPtr<ID3DBlob> mSimplePixelShader;

    ComPtr<ID3D12RootSignature> mRenderingRootSignature;
    ComPtr<ID3D12PipelineState> mRenderingPipelineState;

    ComPtr<ID3D12DescriptorHeap> mRenderingDescriptorHeap;

    D3D12_VIEWPORT mRenderingViewport;
    D3D12_RECT mRenderingScissors;

    std::unique_ptr<RasterizedModel> mQuad;
    Texture* mActiveTexture = nullptr;

private: // RayTracing pipeline

    struct RayTraceLowResCB {
        DirectX::XMFLOAT2 textureResolution;
        unsigned int hasSkybox;
        
    };

    RayTraceLowResCB mRayTraceLowResCB;

    ComPtr<ID3DBlob> mRayTraceLowResComputeShader;

    ComPtr<ID3D12RootSignature> mRayTraceLowResRootSignature;
    ComPtr<ID3D12PipelineState> mRayTraceLowResPipelineState;
    
    ComPtr<ID3D12DescriptorHeap> mRayTraceLowResDescriptorHeap;

    std::unique_ptr<UploadBuffer<Camera::CameraCB>> mCameraBuffer;
    std::unique_ptr<Texture> mLowResTexture;

private: // PathTracing pipeline
    struct PathTraceCB {
        DirectX::XMFLOAT2 textureResolution;
        unsigned int hasSkybox;
        unsigned int isCameraMoving;

        DirectX::XMFLOAT3 randomVector;
        float invGamma = 2.2f;

        unsigned int numPasses;
        unsigned int depth = 4;
    };

    PathTraceCB mPathTraceCB;
    ComPtr<ID3DBlob> mPathTraceComputeShader;

    ComPtr<ID3D12RootSignature> mPathTraceSignature;
    ComPtr<ID3D12PipelineState> mPathTracePipelineState;

    ComPtr<ID3D12DescriptorHeap> mPathTraceDescriptorHeap;
    std::unique_ptr<Texture> mPathtracedTexture;

    enum class RendererState {
        RayTrace = 0, PathTrace = 1
    };
    RendererState mRendererState = RendererState::RayTrace;

    unsigned int mCurrentSample = 0;

    std::mt19937 mRandomGenerator;
    std::uniform_real_distribution<float> mUniformRandom;

private: // Configurations
    OblivionInitialization mInitData;

    unsigned int mClientWidth;
    unsigned int mClientHeight;

    int mLowResScale; // divide window size by this value to perform fast low res raytracing

    float mMouseSensivity;
    bool mMouseInverted;

    float mCameraSpeed = 1.0f;

    std::unique_ptr<SceneLoader> mSceneLoader;

private: // "Gameplay"
    std::unique_ptr<Camera> mCamera;

    std::chrono::system_clock::time_point mStartPathTracing;
    std::chrono::system_clock::time_point mLastPathTrace;

private: // Imgui rendering
    ComPtr<ID3D12DescriptorHeap> mImguiDescriptorHeap;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

};

