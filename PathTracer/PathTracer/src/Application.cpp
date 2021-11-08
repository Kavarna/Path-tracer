#include "Application.h"
#include "./Graphics/Direct3D.h"
#include "Utils/Threading.h"

// #define DISABLE_IMGUI
#ifndef DISABLE_IMGUI
// Imgui stuff
#include "./Graphics/imgui/imgui.h"
#include "./Graphics/imgui/imgui_impl_win32.h"
#include "./Graphics/imgui/imgui_impl_dx12.h"
#endif // DISABLE_IMGUI

// Boost stuff
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#ifndef DISABLE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // DISABLE_IMGUI


Application::Application(HINSTANCE hInstance, const OblivionInitialization& initData) : mInitData(initData) {
    if (!TRY_RETURN_VALUE(InitFromConfigFile(initData.configFile), true, false)) {
        Oblivion::DebugPrintLine("Using default settings. "\
                                 "The settings will be written to the specified file (", initData.configFile, ")");
        InitFromDefaultConfigurations(initData.configFile);
    }
    InitWindow(hInstance);
    
}

Application::~Application() {
    Threading::Reset();
    WriteConfigurationsToFile(mInitData.configFile);
#ifndef DISABLE_IMGUI
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif // DISABLE_IMGUI

    mComputeCommandQueue.reset();
    mDirectCommandQueue.reset();
    Direct3D::Reset();
}

void Application::InitFromConfigFile(const std::string& path) {
    using namespace boost::property_tree;

    ptree pt;
    json_parser::read_json(path, pt);

    mClientWidth = pt.get_child("Client.Width").get_value<unsigned int>();
    mClientHeight = pt.get_child("Client.Height").get_value<unsigned int>();

    mLowResScale = pt.get_child("Render.LowResInvMultiplier").get_value<unsigned int>();
    mRenderingCB.invGamma = pt.get_child("Render.InvGamma").get_value<float>();
    mPathTraceCB.depth = pt.get_child("Render.Depth").get_value<unsigned int>();

    mMouseSensivity = pt.get_child("Gameplay.Mouse Sensivity").get_value<float>();
    mMouseInverted = pt.get_child("Gameplay.Mouse inverted").get_value<bool>();
    mCameraSpeed = pt.get_child("Gameplay.Camera Speed").get_value<float>();
}

void Application::WriteConfigurationsToFile(const std::string& path) {
    using namespace boost::property_tree;
    ptree pt;

    pt.put("Client.Width", mClientWidth);
    pt.put("Client.Height", mClientHeight);

    pt.put("Render.LowResInvMultiplier", mLowResScale);
    pt.put("Render.InvGamma", mRenderingCB.invGamma);
    pt.put("Render.Depth", mPathTraceCB.depth);

    pt.put("Gameplay.Mouse Sensivity", mMouseSensivity);
    pt.put("Gameplay.Mouse inverted", mMouseInverted);
    pt.put("Gameplay.Camera Speed", mCameraSpeed);

    json_parser::write_json(path, pt);
}

void Application::InitFromDefaultConfigurations(const std::string& path) {    
    mClientWidth = 1280;
    mClientHeight = 720;
    mLowResScale = 4;

    mMouseSensivity = 100.f;
    mMouseInverted = false;
}

void Application::InitWindow(HINSTANCE hInstance) {

    WNDCLASSEX wndClass = { 0 };
    wndClass.cbSize = sizeof(wndClass);
    wndClass.lpszClassName = Oblivion::wENGINE_NAME;
    wndClass.lpfnWndProc = Application::WndProc;
    wndClass.hInstance = hInstance;
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;

    EVALUATE(RegisterClassEx(&wndClass) != 0, "Unable to register Window class");

    int screenSizeX = GetSystemMetrics(SM_CXSCREEN);
    int screenSizeY = GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, (LONG)mClientWidth, (LONG)mClientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int windowX = (int)std::max({ 0.0f, (screenSizeX - windowWidth) / 2.0f });
    int windowY = (int)std::max({ 0.0f, (screenSizeY - windowHeight) / 2.0f });

    mWindow = CreateWindow(Oblivion::wENGINE_NAME, Oblivion::wAPPLICATION_NAME,
                           WS_OVERLAPPEDWINDOW, windowX, windowY,
                           windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);
    EVALUATE(mWindow != nullptr, "Unable to create window");

    Oblivion::DebugPrintLine("Successfully created window");
}

void Application::InitD3D() {
    auto* d3d = Direct3D::Get();

    mDirectCommandQueue = std::make_unique<QueueManager>(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT);
    mComputeCommandQueue = std::make_unique<QueueManager>(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE);

    mSwapchain = d3d->CreateSwapchain(mWindow, BufferCount, Direct3D::Get()->HasTearingSupport(), mDirectCommandQueue->GetQueue().Get());
    mCurrentBackBufferIndex = mSwapchain->GetCurrentBackBufferIndex();
    mRTVHeap = d3d->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV, BufferCount);
    
    Oblivion::DebugPrintLine("Successfully initialized Direct3D");
}

void Application::InitRenderingPipeline() {
    auto* d3d = Direct3D::Get();

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\SimpleVertexShader.cso", &mSimpleVertexShader));
    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\SimplePixelShader.cso", &mSimplePixelShader));

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    
    // Handle parameters
    CD3DX12_ROOT_PARAMETER parameters[2];

    // texture
    CD3DX12_DESCRIPTOR_RANGE textureRange = {};
    textureRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    parameters[0].InitAsDescriptorTable(1, &textureRange, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL);
    parameters[1].InitAsConstants(sizeof(RenderingCB) / sizeof(float), 1, 0, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL);

    // Handle samplers
    CD3DX12_STATIC_SAMPLER_DESC samplerDesc[1] = {};
    samplerDesc[0].Init(0, D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                        D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                        D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                        D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    rootSignatureDesc.Init(ARRAYSIZE(parameters), parameters, ARRAYSIZE(samplerDesc), samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRenderingRootSignature = d3d->CreateRootSignature(rootSignatureDesc);

    mRenderingDescriptorHeap = d3d->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxDescriptorCount,
                                                         D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    mSwapchain->GetDesc1(&swapchainDesc);

    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE;
    dsDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pipelineDesc.DepthStencilState = dsDesc;
    pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
    auto layoutInfo = RasterizedModel::SVertex::GetInputElementDesc();
    pipelineDesc.InputLayout = { layoutInfo.data(), RasterizedModel::SVertex::LayoutElementCount };
    pipelineDesc.NodeMask = 0;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = swapchainDesc.Format;
    pipelineDesc.pRootSignature = mRenderingRootSignature.Get();
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.SampleDesc = { 1, 0 };
    pipelineDesc.SampleMask = UINT_MAX;

    pipelineDesc.VS = { mSimpleVertexShader->GetBufferPointer(), mSimpleVertexShader->GetBufferSize() };
    pipelineDesc.PS = { mSimplePixelShader->GetBufferPointer(), mSimplePixelShader->GetBufferSize() };

    mRenderingPipelineState = d3d->CreateGraphicsPipeline(pipelineDesc);

    Oblivion::DebugPrintLine("Successfully initialized Rendering Pipeline");
}

void Application::InitRaytracingPipeline() {
    auto d3d = Direct3D::Get();

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\RayTraceLowRes_CS.cso", &mRayTraceLowResComputeShader));

    CD3DX12_DESCRIPTOR_RANGE descRange[4];
    descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // uav 0
    descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 2); // srv 0
    descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 3, 1, 0, 3); // cbv 1-3
    descRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 1, 0, 6); // srv 1-7
    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsConstants(5, 0);
    rootParameters[1].InitAsDescriptorTable(ARRAYSIZE(descRange), descRange);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignature = {};
    CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    rootSignature.Init(ARRAYSIZE(rootParameters), rootParameters, 1, &sampler);

    mRayTraceLowResRootSignature = d3d->CreateRootSignature(rootSignature);

    mRayTraceLowResDescriptorHeap = d3d->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxDescriptorCount,
                                                              D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
    pipelineDesc.NodeMask = 0;
    pipelineDesc.pRootSignature = mRayTraceLowResRootSignature.Get();

    pipelineDesc.CS = { mRayTraceLowResComputeShader->GetBufferPointer(), mRayTraceLowResComputeShader->GetBufferSize() };

    mRayTraceLowResPipelineState = d3d->CreateComputePipeline(pipelineDesc);


    Oblivion::DebugPrintLine("Successfully initialized Tracing Pipeline");
}

void Application::InitPathTracingPipeline() {
    auto d3d = Direct3D::Get();

    ThrowIfFailed(D3DReadFileToBlob(L"Shaders\\PathTrace_CS.cso", &mPathTraceComputeShader));

    CD3DX12_DESCRIPTOR_RANGE descRange[5];
    descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // uav 0
    descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 2); // srv 0
    descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 3, 1, 0, 3); // cbv 1-3
    descRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 1, 0, 6); // srv 1-7
    descRange[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 4, 0, 13); // cbv 4
    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsConstants(sizeof(PathTraceCB) / sizeof(float), 0);
    rootParameters[1].InitAsDescriptorTable(ARRAYSIZE(descRange), descRange);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignature = {};
    CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    rootSignature.Init(ARRAYSIZE(rootParameters), rootParameters, 1, &sampler);

    mPathTraceSignature = d3d->CreateRootSignature(rootSignature);

    mPathTraceDescriptorHeap = d3d->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxDescriptorCount,
                                                         D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
    pipelineDesc.NodeMask = 0;
    pipelineDesc.pRootSignature = mPathTraceSignature.Get();

    pipelineDesc.CS = { mPathTraceComputeShader->GetBufferPointer(), mPathTraceComputeShader->GetBufferSize() };

    mPathTracePipelineState = d3d->CreateComputePipeline(pipelineDesc);

    mUniformRandom = std::uniform_real_distribution<float>(-1.0f, 1.0f);

    Oblivion::DebugPrintLine("Successfully initialized Path Tracing Pipeline");
}

void Application::InitModels() {

    auto commandList = mDirectCommandQueue->GetCommandList(nullptr);

    mSceneLoader = std::make_unique<SceneLoader>(mInitData.inputFiles);
    mSceneLoader->Load(commandList);

    mQuad = std::make_unique<RasterizedModel>(RasterizedModel::PrimitiveType::Quad, commandList);

    auto fenceValue = mDirectCommandQueue->ExecuteCommandList(commandList);
    mDirectCommandQueue->WaitForFenceValue(fenceValue);

    mQuad->ResetIntermediaryBuffer();
    mSceneLoader->ResetIntermediaryBuffer();

    mRayTraceLowResCB.hasSkybox = mSceneLoader->GetSkybox() == nullptr ? 0 : 1;
    mPathTraceCB.hasSkybox = mSceneLoader->GetSkybox() == nullptr ? 0 : 1;

    Oblivion::DebugPrintLine("Successfully loaded models");
}

void Application::InitImgui() {
    auto d3d = Direct3D::Get();
#ifndef DISABLE_IMGUI
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc1 = {};
    ThrowIfFailed(mSwapchain->GetDesc1(&swapchainDesc1));

    mImguiDescriptorHeap = d3d->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1,
                                                     D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(mWindow);
    ImGui_ImplDX12_Init(d3d->GetD3D12Device(), BufferCount,
                        swapchainDesc1.Format, mImguiDescriptorHeap.Get(),
                        mImguiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        mImguiDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    Oblivion::DebugPrintLine("Successfully initialized Imgui");
#endif // DISABLE_IMGUI

}

void Application::InitGameplayObjects() {
    auto d3d = Direct3D::Get();

    mCamera = std::make_unique<Camera>(DirectX::XMFLOAT3(1.0f, 3.0f, -5.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f),
                                       DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), 1.0f, 60.f);

    mCameraBuffer = std::make_unique<UploadBuffer<Camera::CameraCB>>(1, true);
    
    Oblivion::DebugPrintLine("Successfully initialized gameplay objects");
}

void Application::OnCreate(const EventArgs& eventArgs) {
    InitD3D();
    InitRenderingPipeline();
    InitRaytracingPipeline();
    InitPathTracingPipeline();
    InitImgui();
    InitModels();
    InitGameplayObjects();

    RECT windowRect = {};
    GetWindowRect(mWindow, &windowRect);
    OnResize({ windowRect.right - windowRect.left, windowRect.bottom - windowRect.top });

    Oblivion::DebugPrintLine("Successfully created application");
}

void Application::OnResize(const ResizeEventArgs& eventArgs) {

    if (eventArgs.Width == 0 && eventArgs.Height == 0) {
        return;
    }
    Oblivion::DebugPrintLine("Window resized. New size = (", eventArgs.Width, ", ", eventArgs.Height, ")");

    mClientWidth = eventArgs.Width;
    mClientHeight = eventArgs.Height;

    mDirectCommandQueue->Flush();

    for (unsigned int i = 0; i < BufferCount; ++i) {
        mRenderTargetViews[i].Reset();
    }

    DXGI_SWAP_CHAIN_DESC1 desc1;
    ThrowIfFailed(mSwapchain->GetDesc1(&desc1));

    ThrowIfFailed(
        mSwapchain->ResizeBuffers(BufferCount, eventArgs.Width, eventArgs.Height,
                                  desc1.Format, desc1.Flags)
    );

    UpdateRenderTargetViews();
#ifndef DISABLE_IMGUI
    ImGui_ImplDX12_CreateDeviceObjects();
#endif // DISABLE_IMGUI

#pragma region Prepare descriptor heaps
    auto textureWidth = mClientWidth / mLowResScale;
    auto textureHeight = mClientHeight / mLowResScale;
    std::size_t lowResHeapOffset = 0, hiResHeapOffset = 0;
    mLowResTexture = std::make_unique<Texture>(textureWidth, textureHeight, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                         D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                         D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    mActiveTexture = mLowResTexture.get();
    mLowResTexture->CreateViewInHeap(mRayTraceLowResDescriptorHeap.Get(), lowResHeapOffset);
    lowResHeapOffset += mLowResTexture->GetHeapUsedSize();


    mPathtracedTexture = std::make_unique<Texture>(mClientWidth, mClientHeight, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                   D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                                   D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    mPathtracedTexture->CreateViewInHeap(mPathTraceDescriptorHeap.Get(), hiResHeapOffset);
    hiResHeapOffset += mPathtracedTexture->GetHeapUsedSize();

    auto skybox = mSceneLoader->GetSkybox();
    if (skybox) {
        skybox->CreateViewInHeap(mRayTraceLowResDescriptorHeap.Get(), lowResHeapOffset);
        lowResHeapOffset += skybox->GetHeapUsedSize();
        skybox->CreateViewInHeap(mPathTraceDescriptorHeap.Get(), hiResHeapOffset);
        hiResHeapOffset += skybox->GetHeapUsedSize();
    } else {
        lowResHeapOffset += Direct3D::Get()->GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        hiResHeapOffset += Direct3D::Get()->GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    mCameraBuffer->CreateViewInHeap(mRayTraceLowResDescriptorHeap.Get(), lowResHeapOffset);
    lowResHeapOffset += mCameraBuffer->GetHeapUsedSize();
    mCameraBuffer->CreateViewInHeap(mPathTraceDescriptorHeap.Get(), hiResHeapOffset);
    hiResHeapOffset += mCameraBuffer->GetHeapUsedSize();
    mSceneLoader->BindScene(mRayTraceLowResDescriptorHeap.Get(), lowResHeapOffset);
    mSceneLoader->BindScene(mPathTraceDescriptorHeap.Get(), hiResHeapOffset);

    
#pragma endregion
    mPathTraceCB.textureResolution = DirectX::XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mRayTraceLowResCB.textureResolution = DirectX::XMFLOAT2((float)textureWidth, (float)textureHeight);
    mRenderingViewport = CD3DX12_VIEWPORT(0.f, 0.f, (float)eventArgs.Width, (float)eventArgs.Height);

    mRenderingScissors.top = 0;
    mRenderingScissors.left = 0;
    mRenderingScissors.right = LONG_MAX;
    mRenderingScissors.bottom = LONG_MAX;

    mRendererState = RendererState::RayTrace;
}

void Application::OnKeyPressed(const KeyEventArgs& eventArgs) {
    mPressedKeys.insert(eventArgs.Key);

    if (eventArgs.Key == KeyCode::Key::Escape) {
        PostQuitMessage(0);
    }

}

void Application::OnKeyReleased(const KeyEventArgs& eventArgs) {
    mPressedKeys.erase(eventArgs.Key);
}

void Application::OnMouseMoved(const MouseMotionEventArgs& eventArgs) {

    static int lastMouseXPosition = 0;
    static int lastMouseYPosition = 0;

    if (eventArgs.LeftButton && eventArgs.Shift) {
        int relativeMouseMovedX = eventArgs.X - lastMouseXPosition;
        int relativeMouseMovedY = eventArgs.Y - lastMouseYPosition;

        mMouseState.xRelativeMoved = (float)relativeMouseMovedX / (float)mClientWidth;
        mMouseState.yRelativeMoved = (float)relativeMouseMovedY / (float)mClientHeight;

        mRendererState = RendererState::RayTrace;
    }
    lastMouseXPosition = eventArgs.X;
    lastMouseYPosition = eventArgs.Y;
}

void Application::UpdateRenderTargetViews() {

    auto d3d = Direct3D::Get();

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());

    for (unsigned int i = 0; i < BufferCount; ++i) {

        ComPtr<ID3D12Resource1> resource;
        ThrowIfFailed(mSwapchain->GetBuffer(i, IID_PPV_ARGS(&resource)));

        d3d->GetD3D12Device()->CreateRenderTargetView(resource.Get(), nullptr, cpuHandle);
        
        mRenderTargetViews[i] = resource;

        cpuHandle.Offset(d3d->GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    }

    mCurrentBackBufferIndex = mSwapchain->GetCurrentBackBufferIndex();

}

void Application::Run() {

    OnCreate({});

    UpdateWindow(mWindow);
    ShowWindow(mWindow, SW_SHOWNORMAL);

    Oblivion::DebugPrintLine("Showing window & start rendering ! ! !");


    // unsigned long long frameID = 0;
    MSG message;
    while (true) {
        if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        } else {
            if (mInternalMessageQueue.size()) {
                auto& currentMessage = mInternalMessageQueue.front();

                SendMessage(mWindow, currentMessage.message_id, currentMessage.wParam, currentMessage.lParam);

                mInternalMessageQueue.pop();
            }
            // Oblivion::DebugPrintLine("~~~~~~~~~~~~~~~~~~~~~~~~~~~~FRAME ", frameID, " STARTED~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
            Update();
            Render();
            // Oblivion::DebugPrintLine("~~~~~~~~~~~~~~~~~~~~~~~~~~~~FRAME ", frameID++, " ENDED~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        }
    }

}

void Application::Update() {
    float dt = 1.0f / ImGui::GetIO().Framerate;

    if (mPressedKeys.find(KeyCode::Key::W) != mPressedKeys.end()) {
        mCamera->WalkForward(dt * mCameraSpeed);
        mRendererState = RendererState::RayTrace;
    }
    if (mPressedKeys.find(KeyCode::Key::A) != mPressedKeys.end()) {
        mCamera->StrafeLeft(dt * mCameraSpeed);
        mRendererState = RendererState::RayTrace;
    }
    if (mPressedKeys.find(KeyCode::Key::S) != mPressedKeys.end()) {
        mCamera->WalkBackward(dt * mCameraSpeed);
        mRendererState = RendererState::RayTrace;
    }
    if (mPressedKeys.find(KeyCode::Key::D) != mPressedKeys.end()) {
        mCamera->StrafeRight(dt * mCameraSpeed);
        mRendererState = RendererState::RayTrace;
    }
    if (mPressedKeys.find(KeyCode::Key::E) != mPressedKeys.end()) {
        mCamera->Lift(dt * mCameraSpeed);
        mRendererState = RendererState::RayTrace;
    }
    if (mPressedKeys.find(KeyCode::Key::Q) != mPressedKeys.end()) {
        mCamera->Lower(dt * mCameraSpeed);
        mRendererState = RendererState::RayTrace;
    }

    mCamera->RotateX(0.05f, mMouseState.yRelativeMoved * mMouseSensivity * (mMouseInverted ? -1 : 1));
    mCamera->RotateY(0.05f, mMouseState.xRelativeMoved * mMouseSensivity);

    mCamera->Construct();

    mCameraBuffer->CopyData(&mCamera->GetCameraCB());


    mMouseState.xRelativeMoved = 0.0f;
    mMouseState.yRelativeMoved = 0.0f;
}

void Application::Render() {
    Trace();
    RenderResult();
}

void Application::Trace() {
    auto d3d = Direct3D::Get();

    constexpr const unsigned int ThreadsInGroupX = 32;
    constexpr const unsigned int ThreadsInGroupY = 32;
    constexpr const unsigned int ThreadsInGroupZ = 1;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(mLastPathTrace - mStartPathTracing);

    if (mRendererState == RendererState::RayTrace) {
        auto cmdList = mComputeCommandQueue->GetCommandList(mRayTraceLowResPipelineState);

        cmdList->SetComputeRootSignature(mRayTraceLowResRootSignature.Get());
        cmdList->SetComputeRoot32BitConstants(0, 3, &mRayTraceLowResCB, 0);
        cmdList->SetDescriptorHeaps(1, mRayTraceLowResDescriptorHeap.GetAddressOf());
        cmdList->SetComputeRootDescriptorTable(1, mRayTraceLowResDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        cmdList->Dispatch((UINT)ceil((FLOAT)mLowResTexture->GetWidth() / ThreadsInGroupX), (UINT)ceil((FLOAT)mLowResTexture->GetHeight() / ThreadsInGroupY), 1);

        mRendererState = RendererState::PathTrace;
        mActiveTexture = mLowResTexture.get();
        mCurrentSample = 0;
        mRenderingCB.applyGamma = 0;

        auto fenceValue = mComputeCommandQueue->ExecuteCommandList(cmdList);
        mComputeCommandQueue->WaitForFenceValue(fenceValue);

        mStartPathTracing = std::chrono::system_clock::now();
        mLastPathTrace = mStartPathTracing;
    } else if ((float)elapsed.count() / 1000.f < mInitData.maxSecondsPerFrame) {

        // Pathtrace
        auto cmdList = mComputeCommandQueue->GetCommandList(mPathTracePipelineState);

        static bool justShow = false;

        if (mCurrentSample == 0 || justShow) {
            mPathTraceCB.isCameraMoving = true;
            mCurrentSample = 0;
        } else {
            mPathTraceCB.isCameraMoving = false;
        }

        mCurrentSample++;

        mRenderingCB.numPasses = mCurrentSample;
        mRenderingCB.applyGamma = 1;

        mPathTraceCB.randomVector.x = mUniformRandom(mRandomGenerator);
        mPathTraceCB.randomVector.y = mUniformRandom(mRandomGenerator);
        mPathTraceCB.randomVector.z = mUniformRandom(mRandomGenerator);

        cmdList->SetComputeRootSignature(mPathTraceSignature.Get());
        cmdList->SetComputeRoot32BitConstants(0, sizeof(mPathTraceCB) / sizeof(float), &mPathTraceCB, 0);
        cmdList->SetDescriptorHeaps(1, mPathTraceDescriptorHeap.GetAddressOf());
        cmdList->SetComputeRootDescriptorTable(1, mPathTraceDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        cmdList->Dispatch((UINT)ceil((FLOAT)mPathtracedTexture->GetWidth() / ThreadsInGroupX), (UINT)ceil((FLOAT)mPathtracedTexture->GetHeight() / ThreadsInGroupY), 1);

        mRendererState = RendererState::PathTrace;
        mActiveTexture = mPathtracedTexture.get();

        auto fenceValue = mComputeCommandQueue->ExecuteCommandList(cmdList);
        mComputeCommandQueue->WaitForFenceValue(fenceValue);
        mLastPathTrace = std::chrono::system_clock::now();
    }
}

void Application::RenderResult() {

    static Texture* lastActiveTexture = nullptr;
    auto d3d = Direct3D::Get();
    auto commandList = mDirectCommandQueue->GetCommandList(mRenderingPipelineState);

    if (mActiveTexture && mActiveTexture != lastActiveTexture) {
        mActiveTexture->CreateViewInHeap(mRenderingDescriptorHeap.Get(), 0);
        lastActiveTexture = mActiveTexture;
    }

    auto currentRenderTargetViewResource = mRenderTargetViews[mCurrentBackBufferIndex];
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart(),
                                            mCurrentBackBufferIndex,
                                            d3d->GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    { // Prepare for rendering
        d3d->TransitionResource(commandList.Get(), currentRenderTargetViewResource.Get(),
                                D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT backgroundColor[] = { 0.0f,0.0f,0.0f,1.0f };
        commandList->ClearRenderTargetView(rtvHandle, backgroundColor, 0, nullptr);
        commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, nullptr);
    }

    commandList->SetGraphicsRootSignature(mRenderingRootSignature.Get());

    commandList->SetDescriptorHeaps(1, mRenderingDescriptorHeap.GetAddressOf());
    commandList->SetGraphicsRootDescriptorTable(0, mRenderingDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRoot32BitConstants(1, sizeof(RenderingCB) / sizeof(float), &mRenderingCB, 0);
    mQuad->Bind(commandList.Get());

    commandList->RSSetViewports(1, &mRenderingViewport);
    commandList->RSSetScissorRects(1, &mRenderingScissors);

    commandList->DrawIndexedInstanced(mQuad->GetIndexCount(), 1, 0, 0, 0);

    RenderGui(commandList.Get());

    { // Execute & Present
        d3d->TransitionResource(commandList.Get(), currentRenderTargetViewResource.Get(),
                                D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);

        auto fenceValue = mDirectCommandQueue->ExecuteCommandList(commandList);

        unsigned int presentInterval = mUseVsync ? 1 : 0;
        unsigned int presentFlags = d3d->HasTearingSupport() && !mUseVsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        mDirectCommandQueue->WaitForFenceValue(fenceValue);

        mSwapchain->Present(presentInterval, presentFlags);
    }

    mCurrentBackBufferIndex = mSwapchain->GetCurrentBackBufferIndex();

}

void Application::RenderGui(ID3D12GraphicsCommandList* cmdList) {
#ifndef DISABLE_IMGUI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    ImGui::Begin("Configurations");

    if (ImGui::SliderInt("Low res scale", &mLowResScale, 1, 16)) {
        mInternalMessageQueue.push(InternalMessage{ WM_SIZE, SIZE_RESTORED, MAKELPARAM(mClientWidth, mClientHeight) });
    }

    ImGui::Separator();

    float fov = mCamera->GetCameraFov();
    if (ImGui::SliderFloat("FOV", &fov, 10.f, 90.f, "%.1f", 1.0f)) {
        mCamera->SetCameraFov(fov);
        mInternalMessageQueue.push(InternalMessage{ WM_SIZE, SIZE_RESTORED, MAKELPARAM(mClientWidth, mClientHeight) });
    }

    ImGui::SliderFloat("Camera speed", &mCameraSpeed, 0.1f, 100.f);

    ImGui::Separator();

    if (ImGui::SliderFloat("InvGamma", &mRenderingCB.invGamma, 0.1f, 3.0f)) {
        // mInternalMessageQueue.push(InternalMessage{ WM_SIZE, SIZE_RESTORED, MAKELPARAM(mClientWidth, mClientHeight) });
    }

    int depth = mPathTraceCB.depth;
    if (ImGui::SliderInt("Depth", &depth, 1, 10)) {
        mPathTraceCB.depth = static_cast<unsigned int>(depth);
        mInternalMessageQueue.push(InternalMessage{ WM_SIZE, SIZE_RESTORED, MAKELPARAM(mClientWidth, mClientHeight) });
    }

    ImGui::End();

    ImGui::Begin("Debug info");
    ImGui::Text("Frametime: %f (%.2f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Sample: %d", mCurrentSample);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(mLastPathTrace - mStartPathTracing);
    float renderedTime = (float)elapsed.count() / 1000.f;
    ImGui::Text("Time spent rendering: %.3f", renderedTime);
    ImGui::Separator();

    auto camPos = mCamera->GetPosition();
    auto camDir = mCamera->GetCameraCB().direction;
    auto camUp = mCamera->GetCameraCB().up;
    ImGui::Text(Oblivion::appendToString("Camera position: ", camPos).c_str());
    ImGui::Text(Oblivion::appendToString("Camera direction: ", camDir).c_str());
    ImGui::Text(Oblivion::appendToString("Camera up: ", camUp).c_str());

    ImGui::End();

    ImGui::Render();

    cmdList->SetDescriptorHeaps(1, mImguiDescriptorHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
#endif // DISABLE_IMGUI
}


LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Application* game = Application::Get();

#ifndef DISABLE_IMGUI
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
        return true;
#endif // DISABLE_IMGUI

    switch (message) {

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            MSG charMsg;
            // Get the Unicode character (UTF-16)
            unsigned int c = 0;
            // For printable characters, the next message will be WM_CHAR.
            // This message contains the character code we need to send the KeyPressed event.
            // Inspired by the SDL 1.2 implementation.
            if (PeekMessage(&charMsg, hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR) {
                GetMessage(&charMsg, hwnd, 0, 0);
                c = static_cast<unsigned int>(charMsg.wParam);
            }
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
            KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
            game->OnKeyPressed(keyEventArgs);
        }
        break;
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int c = 0;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

            // Determine which key was released by converting the key code and the scan code
            // to a printable character (if possible).
            // Inspired by the SDL 1.2 implementation.
            unsigned char keyboardState[256];
            if (GetKeyboardState(keyboardState)) {
                wchar_t translatedCharacters[4];
                if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0) {
                    c = translatedCharacters[0];
                }

                KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
                game->OnKeyReleased(keyEventArgs);
            }
        }
        break;
        // The default window procedure will play a system notification sound 
        // when pressing the Alt+Enter keyboard combination if this message is 
        // not handled.
        case WM_SYSCHAR:
            break;
        case WM_MOUSEMOVE:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            if (GetActiveWindow() == hwnd) {
                MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
                game->OnMouseMoved(mouseMotionEventArgs);
            }
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            /*MouseButtonEventArgs mouseButtonEventArgs( DecodeMouseButton( message ), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y );
            pWindow->OnMouseButtonPressed( mouseButtonEventArgs );*/
        }
        break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            /*MouseButtonEventArgs mouseButtonEventArgs( DecodeMouseButton( message ), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y );
            pWindow->OnMouseButtonReleased( mouseButtonEventArgs );*/
        }
        break;
        case WM_MOUSEWHEEL:
        {
            // The distance the mouse wheel is rotated.
            // A positive value indicates the wheel was rotated to the right.
            // A negative value indicates the wheel was rotated to the left.
            float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
            short keyStates = (short)LOWORD(wParam);

            bool lButton = (keyStates & MK_LBUTTON) != 0;
            bool rButton = (keyStates & MK_RBUTTON) != 0;
            bool mButton = (keyStates & MK_MBUTTON) != 0;
            bool shift = (keyStates & MK_SHIFT) != 0;
            bool control = (keyStates & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            // Convert the screen coordinates to client coordinates.
            POINT clientToScreenPoint;
            clientToScreenPoint.x = x;
            clientToScreenPoint.y = y;
            ScreenToClient(hwnd, &clientToScreenPoint);

            MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
            // pWindow->OnMouseWheel( mouseWheelEventArgs );
        }
        break;
        case WM_SIZE:
        {
            int width = ((int)(short)LOWORD(lParam));
            int height = ((int)(short)HIWORD(lParam));
            ResizeEventArgs resizeEventArgs(width, height);
            game->OnResize(resizeEventArgs);
        }
        break;
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);;
}
