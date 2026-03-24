#include "GuiManager.h"
#include "StatusMonitor.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <cerrno>

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

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
        std::cerr << "[DEBUG] AudioState é nullptr!" << std::endl;
        return false;
    }

    std::cerr << "[DEBUG] Iniciando GLFW..." << std::endl;
    if (!glfwInit()) {
        std::cerr << "[DEBUG] ERRO: glfwInit() falhou!" << std::endl;
        return false;
    }

    const char* glslVersion = "#version 410";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    std::cerr << "[DEBUG] Criando janela GLFW..." << std::endl;
    GLFWwindow* window = glfwCreateWindow(1180, 720, "AUDIO-THREADS | DJ Interface", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "[DEBUG] ERRO: glfwCreateWindow() falhou!" << std::endl;
        glfwTerminate();
        return false;
    }

    std::cerr << "[DEBUG] Janela criada com sucesso. Inicializando contexto OpenGL..." << std::endl;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    std::cerr << "[DEBUG] Inicializando ImGui para GLFW+OpenGL..." << std::endl;
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "[DEBUG] ERRO: ImGui_ImplGlfw_InitForOpenGL() falhou!" << std::endl;
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    std::cerr << "[DEBUG] Inicializando ImGui OpenGL3 backend com GLSL versão: " << glslVersion << std::endl;
    if (!ImGui_ImplOpenGL3_Init(glslVersion)) {
        std::cerr << "[DEBUG] ERRO: ImGui_ImplOpenGL3_Init() falhou!" << std::endl;
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    std::cerr << "[DEBUG] ImGui inicializado com sucesso!" << std::endl;

    while (!glfwWindowShouldClose(window) && state->programRunning.load(std::memory_order_relaxed)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Apply custom style
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(15.0f, 15.0f);
        style.FramePadding = ImVec2(8.0f, 6.0f);
        style.ItemSpacing = ImVec2(10.0f, 10.0f);
        style.WindowBorderSize = 1.0f;
        
        // Custom colors for dark theme
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(1160.0f, 1040.0f), ImGuiCond_Once);
        
        ImGui::Begin("Audio Mixer - DJ Interface", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        // Main header
        ImGui::PushFont(io.Fonts->Fonts[0]);
        ImGui::Text("AUDIO MIXER - DJ INTERFACE");
        ImGui::PopFont();
        ImGui::Separator();

        // Get state values
        const bool isMasterPlaying = state->globalPlay.load(std::memory_order_relaxed);
        const size_t currentFrame = state->currentFrame.load(std::memory_order_relaxed);
        const float progress = (state->totalFrames == 0)
            ? 0.0f
            : static_cast<float>(currentFrame) / static_cast<float>(state->totalFrames);

        // Master controls panel
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30, 30, 35, 255));
            ImGui::BeginChild("MasterControls", ImVec2(0, 100.0f), true);
            
            ImGui::Text("Master Control");
            ImGui::Spacing();
            
            ImGuiIO& io = ImGui::GetIO();
            
            // Play/Pause button
            if (isMasterPlaying) {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 180, 100, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 210, 120, 255));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 110, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(130, 130, 140, 255));
            }
            
            if (ImGui::Button(isMasterPlaying ? "PAUSE" : "PLAY", ImVec2(120.0f, 40.0f))) {
                state->globalPlay.store(!isMasterPlaying, std::memory_order_relaxed);
            }
            ImGui::PopStyleColor(2);
            
            ImGui::SameLine();
            if (ImGui::Button("RESET", ImVec2(120.0f, 40.0f))) {
                state->currentFrame.store(0, std::memory_order_relaxed);
            }
            
            ImGui::SameLine(400.0f);
            ImGui::Text("Status: %s", isMasterPlaying ? "PLAYING" : "PAUSED");
            
            ImGui::Spacing();
            ImGui::Text("Time: %zu / %zu frames", currentFrame, state->totalFrames);
            ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 20.0f));
            
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // Tabs for Mixer and Status
        if (ImGui::BeginTabBar("##MainTabs", ImGuiTabBarFlags_None)) {
            
            // MIXER TAB
            if (ImGui::BeginTabItem("Mixer")) {
                ImGui::Spacing();
                ImGui::Text("Track Controls");
                ImGui::Separator();
                ImGui::Spacing();

                const size_t maxTracksToShow = std::min<size_t>(8, state->tracks.size());
                
                // Grid layout for tracks
                for (size_t i = 0; i < maxTracksToShow; ++i) {
                    const bool isTrackPlaying = state->tracks[i].isPlaying.load(std::memory_order_relaxed);
                    
                    if (isTrackPlaying) {
                        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 150, 80, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(70, 180, 100, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(30, 120, 60, 255));
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 60, 70, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 90, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(50, 50, 60, 255));
                    }

                    const std::string buttonLabel = state->tracks[i].trackName + (isTrackPlaying ? " [ON]" : " [OFF]");
                    if (ImGui::Button(buttonLabel.c_str(), ImVec2(270.0f, 60.0f))) {
                        state->tracks[i].isPlaying.store(!isTrackPlaying, std::memory_order_relaxed);
                    }

                    ImGui::PopStyleColor(3);

                    if ((i % 4) != 3 && i + 1 < maxTracksToShow) {
                        ImGui::SameLine();
                    } else {
                        ImGui::Spacing();
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Active tracks: %zu / %zu", countActiveTracks(state), state->tracks.size());
                
                ImGui::EndTabItem();
            }

            // STATUS PANEL TAB
            if (ImGui::BeginTabItem("Status Details")) {
                ImGui::Spacing();
                
                bool isPlaying = state->globalPlay.load(std::memory_order_relaxed);
                size_t currentFrame = state->currentFrame.load(std::memory_order_relaxed);
                size_t totalFrames = state->totalFrames;
                uint32_t sampleRate = state->sampleRate;

                // Status info header
                {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 50, 255));
                    ImGui::BeginChild("StatusInfo", ImVec2(0, 80.0f), true);
                    
                    ImGui::Columns(3, "status_info", false);
                    ImGui::SetColumnWidth(0, 300.0f);
                    ImGui::SetColumnWidth(1, 300.0f);
                    
                    ImGui::Text("Playback Status");
                    ImGui::Text("%s", isPlaying ? "PLAYING" : "PAUSED");
                    ImGui::NextColumn();
                    
                    ImGui::Text("Sample Rate");
                    ImGui::Text("%u Hz", sampleRate);
                    ImGui::NextColumn();
                    
                    ImGui::Text("Asset");
                    ImGui::Text("%s", state->assetName.c_str());
                    ImGui::NextColumn();
                    
                    ImGui::Columns(1);
                    
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Tracks detailed view - Table
                ImGui::Text("Track Details");
                ImGui::Spacing();
                
                if (ImGui::BeginTable("TracksTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Track Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < state->tracks.size(); ++i) {
                        ImGui::TableNextRow();
                        
                        bool trackActive = state->tracks[i].isPlaying.load(std::memory_order_relaxed);
                        std::string trackName = state->tracks[i].trackName;

                        // Track name
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", trackName.c_str());

                        // Status
                        ImGui::TableSetColumnIndex(1);
                        if (trackActive) {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80, 200, 100, 255));
                            ImGui::Text("[+] ACTIVE");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 180, 255));
                            ImGui::Text("[ ] MUTED");
                            ImGui::PopStyleColor();
                        }

                        // Progress bar
                        ImGui::TableSetColumnIndex(2);
                        double trackProgress = (totalFrames == 0) ? 0.0 : (100.0 * currentFrame / totalFrames);
                        ImGui::ProgressBar(static_cast<float>(trackProgress / 100.0), ImVec2(-FLT_MIN, 20.0f));

                        // Time
                        ImGui::TableSetColumnIndex(3);
                        std::string currentTime = formatTime(currentFrame, sampleRate);
                        ImGui::Text("%s", currentTime.c_str());
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Tip: Close window to exit the application");

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
