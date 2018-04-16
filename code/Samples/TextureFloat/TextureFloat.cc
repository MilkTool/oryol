//------------------------------------------------------------------------------
//  TextureFloat.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
#include "Core/Time/Clock.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders.h"

using namespace Oryol;

class TextureFloatApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();
    
    Id renderPass;
    PassAction renderPassAction;
    DrawState offscreenDrawState;
    DrawState copyDrawState;
    glm::mat4 view;
    glm::mat4 proj;
    OffscreenShader::fsParams offscreenFSParams;
    TimePoint lastFrameTimePoint;
};
OryolMain(TextureFloatApp);

//------------------------------------------------------------------------------
AppState::Code
TextureFloatApp::OnInit() {
    // setup rendering system
    Gfx::Setup(GfxDesc().Width(512).Height(512).Title("Oryol Float Texture Sample"));
    Dbg::Setup();

    // check required extensions
    if (!Gfx::QueryFeature(GfxFeature::TextureFloat)) {
        o_error("ERROR: float_texture extension required!\n");
    }
    
    // create an offscreen float render target, same size as display,
    // configure texture sampler with point-filtering
    const PixelFormat::Code rtColorFormat = PixelFormat::RGBA32F;
    Id rt = Gfx::CreateTexture(NewTextureDesc()
        .RenderTarget(true)
        .Width(Gfx::Desc().Width())
        .Height(Gfx::Desc().Height())
        .Format(rtColorFormat)
        .MinFilter(TextureFilterMode::Nearest)
        .MagFilter(TextureFilterMode::Nearest)
        .Done());
    this->renderPass = Gfx::CreatePass(NewPassDesc().ColorAttachment(0, rt).Done());
    this->renderPassAction.DontCareColor(0);

    // fullscreen mesh, we'll reuse this several times
    const float quadVertices[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    this->offscreenDrawState.VertexBuffers[0] = Gfx::CreateBuffer(NewBufferDesc()
        .Size(sizeof(quadVertices))
        .Content(quadVertices)
        .Done());
    this->copyDrawState.VertexBuffers[0] = this->offscreenDrawState.VertexBuffers[0];

    // setup draw state for offscreen rendering to float render target
    this->offscreenDrawState.Pipeline = Gfx::CreatePipeline(NewPipelineDesc()
        .Shader(Gfx::CreateShader(OffscreenShader::Desc()))
        .Layout(0, {{"in_pos", VertexFormat::Float2}})
        .PrimitiveType(PrimitiveType::TriangleStrip)
        .ColorFormat(rtColorFormat)
        .DepthFormat(PixelFormat::None)
        .Done());
    this->offscreenFSParams.time = 0.0f;

    // fullscreen-copy resources
    this->copyDrawState.Pipeline = Gfx::CreatePipeline(NewPipelineDesc()
        .Shader(Gfx::CreateShader(CopyShader::Desc()))
        .Layout(0, {{"in_pos", VertexFormat::Float2}})
        .PrimitiveType(PrimitiveType::TriangleStrip)
        .Done());
    this->copyDrawState.FSTexture[CopyShader::tex] = rt;

    // setup static transform matrices
    const float fbWidth = (const float) Gfx::DisplayAttrs().Width;
    const float fbHeight = (const float) Gfx::DisplayAttrs().Height;
    this->proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.01f, 5.0f);
    this->view = glm::mat4();

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
TextureFloatApp::OnRunning() {

    this->offscreenFSParams.time += 1.0f / 60.0f;
    
    // render plasma to offscreen render target, do not clear
    Gfx::BeginPass(this->renderPass, this->renderPassAction);
    Gfx::ApplyDrawState(this->offscreenDrawState);
    Gfx::ApplyUniformBlock(this->offscreenFSParams);
    Gfx::Draw(0, 4);
    Gfx::EndPass();
    
    // copy fullscreen quad
    Gfx::BeginPass();
    Gfx::ApplyDrawState(this->copyDrawState);
    Gfx::Draw(0, 4);
    Dbg::DrawTextBuffer();
    Gfx::EndPass();
    Gfx::CommitFrame();
    
    Duration frameTime = Clock::LapTime(this->lastFrameTimePoint);
    Dbg::PrintF("%.3fms", frameTime.AsMilliSeconds());
    
    // continue running or quit?
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
TextureFloatApp::OnCleanup() {
    Dbg::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}
