#include "pch.h"
#include "Editor/UI/gui_layer.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"


void GuiLayer::Begin() {

    setupGuiFrame();
    DrawFPSCounter();

    ImGui::Render();
}

void GuiLayer::setupGuiFrame() {

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(
        (float)editorContext.winWidth,
        (float)editorContext.winHeight
    );

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiLayer::DrawFPSCounter() {

    const float fps = editorContext.fps;
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.3f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("FPSOverlay", nullptr, flags)) {
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame: %.3f ms", 1000.0f / fps);
    }
    ImGui::End();
}