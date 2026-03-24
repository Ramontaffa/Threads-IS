#include "GuiManager.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cfloat>
#include <string>

namespace {

size_t countActiveTracks(AudioState* state) {
    size_t activeTracks = 0;
    for (auto& track : state->tracks) {
        if (track.isPlaying.load(std::memory_order_relaxed)) {
            ++activeTracks;
        }
    }
    return activeTracks;
}

} // namespace

bool runGui(AudioState* state) {
    if (state == nullptr) {
        return false;
    }

    if (!glfwInit()) {
        return false;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1180, 720, "AUDIO-THREADS | DJ Interface", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init(glslVersion)) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    while (!glfwWindowShouldClose(window) && state->programRunning.load(std::memory_order_relaxed)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1140.0f, 680.0f), ImGuiCond_Once);
        ImGui::Begin("Mixer");

        const bool isMasterPlaying = state->globalPlay.load(std::memory_order_relaxed);
        if (ImGui::Button(isMasterPlaying ? "Pause Master" : "Play Master", ImVec2(210.0f, 44.0f))) {
            state->globalPlay.store(!isMasterPlaying, std::memory_order_relaxed);
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset Timeline", ImVec2(210.0f, 44.0f))) {
            state->currentFrame.store(0, std::memory_order_relaxed);
        }

        const size_t currentFrame = state->currentFrame.load(std::memory_order_relaxed);
        const float progress = (state->totalFrames == 0)
            ? 0.0f
            : static_cast<float>(currentFrame) / static_cast<float>(state->totalFrames);

        ImGui::Spacing();
        ImGui::Text("Status: %s", isMasterPlaying ? "TOCANDO" : "PAUSADO");
        ImGui::Text("Frame: %zu / %zu", currentFrame, state->totalFrames);
        ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 18.0f));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Tracks");
        ImGui::Spacing();

        const size_t maxTracksToShow = std::min<size_t>(8, state->tracks.size());
        for (size_t i = 0; i < maxTracksToShow; ++i) {
            const bool isTrackPlaying = state->tracks[i].isPlaying.load(std::memory_order_relaxed);

            if (isTrackPlaying) {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 160, 80, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 190, 100, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(30, 130, 65, 255));
            }

            const std::string buttonLabel = "Track " + std::to_string(i + 1) + (isTrackPlaying ? " [ON]" : " [OFF]");
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(250.0f, 52.0f))) {
                state->tracks[i].isPlaying.store(!isTrackPlaying, std::memory_order_relaxed);
            }

            if (isTrackPlaying) {
                ImGui::PopStyleColor(3);
            }

            if ((i % 4) != 3 && i + 1 < maxTracksToShow) {
                ImGui::SameLine();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Active tracks: %zu", countActiveTracks(state));
        ImGui::Text("Atalhos: feche a janela para encerrar o programa.");

        ImGui::End();

        ImGui::Render();
        int displayWidth = 0;
        int displayHeight = 0;
        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.07f, 0.08f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    state->programRunning.store(false, std::memory_order_relaxed);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return true;
}
