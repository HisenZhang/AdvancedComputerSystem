#include "DSP.h"

#include <iostream>
#include <string>

// TODO:
// - Create a useful interface
//   - display result waveforms
//   - drag to reorder, button/right click to delete
//   - loading icon while running
// - Implement different filter types (reverb, low pass, high pass, band pass, etc.)
// - Run dsp on a separate thread from the UI
// - Source groups
// - Build on Unix
// - Flag for useAVX
// - Benchmarking

struct PlotInput
{
    PlotInput(Signal *signal)
    {
        this->signal = signal;
        UpdateSampleStride();
    }

    void UpdateSampleStride()
    {
        if (signal->data.size() > maxSamples)
        {
            sampleStride = signal->data.size() / maxSamples;
        }
        else { sampleStride = 1; }

        bInputDirty = true;
    }

    Signal *signal = nullptr;
    uint32_t maxSamples = 10000;
    uint32_t sampleStride = 0;
    bool bInputDirty = false;
};

int main(int, char**)
{
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << "\n";
    });

    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1200, 1080, "DSP Tool", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
    });

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    Signal inputSignal, outputSignal;
    PlotInput inPlotInput(&inputSignal), outPlotInput(&outputSignal);
    std::vector<std::unique_ptr<AudioEffect>> effects;

    bool bShowGenerators = false;
    ImVec2 buttonSize(200, 70);
    std::string messageSaveSuccesful = "";

	while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int windowWith, windowHeight, windowX, windowY;
        glfwGetFramebufferSize(window, &windowWith, &windowHeight);
        glfwGetWindowPos(window, &windowX, &windowY);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImPlot::ShowDemoWindow();
        //ImGui::ShowDemoWindow();

        // Main Window
        {
            ImGui::Begin("DSP Tool", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoScrollbar);
            ImGui::SetWindowSize(ImVec2(windowWith, windowHeight));
            ImGui::SetWindowPos(ImVec2(windowX, windowY));

            // Input Waveform
            {
                if (ImPlot::BeginPlot("Input Signal", ImVec2(-1, 400), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
                    ImPlotAxisFlags axisFlag = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoHighlight;
                    if (inPlotInput.bInputDirty)
                    {
                        axisFlag |= ImPlotAxisFlags_AutoFit;
                        inPlotInput.bInputDirty = false;
                    }

                    ImPlot::SetupAxes("Time (s)", NULL, axisFlag | ImPlotAxisFlags_PanStretch | ImPlotAxisFlags_LockMin, axisFlag | ImPlotAxisFlags_Lock);
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0f, 10.0f);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1f, 1.1f);
                    ImPlot::SetNextLineStyle(ImColor(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLineG("data", [](int i, void* data) -> ImPlotPoint {
                        PlotInput* pi = (PlotInput*)data;
                        float value = pi->signal->data[i * pi->sampleStride];
                        float time = (float)i * pi->sampleStride / pi->signal->sampleRate;

                        return ImPlotPoint(time, value);
                    }, &inPlotInput, std::min(inPlotInput.maxSamples, (uint32_t)inPlotInput.signal->data.size()));

                    ImPlot::EndPlot();
                }

                if (ImGui::Button("Load File", buttonSize))
                {
                    nfdchar_t* loadFilePath = nullptr;
                    nfdresult_t res = NFD_OpenDialog("wav", nullptr, &loadFilePath);
                    if (res == NFD_OKAY)
                    {
	                    AudioFile<float> af;
	                    af.load(loadFilePath);
                        free(loadFilePath);

                        inputSignal.data = af.samples[0];
                        inputSignal.sampleRate = af.getSampleRate();
                        inPlotInput = PlotInput(&inputSignal);
                    }
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load an audio file (.wav only).");
                ImGui::SameLine();
                if (ImGui::Button("Generators", buttonSize)) bShowGenerators = true;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Generate signal from preset waveform.");
            }

            // Effect List
            {
                ImGui::Text("Audio Effects");
                ImGui::SameLine();
                static bool bUseAVX = true;
                ImGui::Checkbox("Use AVX", &bUseAVX);

                ImGui::Separator();

                for (auto& effect : effects)
                {
                    effect->DrawGUI();
                    ImGui::Separator();
                }

                const char* effectNames[] = { "Echo", "Derivative" };
                static int effectIndex = 0;
                ImGui::Combo(" ", &effectIndex, effectNames, IM_ARRAYSIZE(effectNames));
                ImGui::SameLine();
                if (ImGui::Button("Add Effect"))
                {
                    if (effectIndex == 0) effects.emplace_back(new EchoEffect);
                    else if (effectIndex == 1) effects.emplace_back(new DerivativeEffect);
                }

                if (ImGui::Button("Apply", buttonSize))
                {
                    if (inputSignal.data.size() > 0)
                    {
                        if (effects.size() == 0)
                        {
                            outputSignal = inputSignal;
                        }
                        else
                        {
                            Signal temp = inputSignal;
                            for (auto& effect : effects)
                            {
                                effect->Apply(temp, outputSignal, bUseAVX);
                                temp = outputSignal;
                            }
                        }

                        outPlotInput = PlotInput(&outputSignal);
                    }
                }

                //static const char* item_names[] = { "Item One", "Item Two", "Item Three", "Item Four", "Item Five" };
                //for (int n = 0; n < IM_ARRAYSIZE(item_names); n++)
                //{
                //    const char* item = item_names[n];
                //    ImGui::Selectable(item);

                //    ImGui::DragInt()

                //    if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
                //    {
                //        int n_next = n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
                //        if (n_next >= 0 && n_next < IM_ARRAYSIZE(item_names))
                //        {
                //            item_names[n] = item_names[n_next];
                //            item_names[n_next] = item;
                //            ImGui::ResetMouseDragDelta();
                //        }
                //    }
                //}

                //auto pos = ImGui::GetCursorPos();
                //static int selected = false;
                //for (int n = 0; n < 10; n++)
                //{
                //    ImGui::PushID(n);
                //
                //    char buf[32];
                //    sprintf(buf, "##Object %d", n);
                //
                //    ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
                //    if (ImGui::Selectable(buf, n == selected, 0, ImVec2(100, 50))) {
                //        selected = n;
                //    }
                //    ImGui::SetItemAllowOverlap();
                //
                //    ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
                //    ImGui::Text("foo");
                //
                //    ImGui::SetCursorPos(ImVec2(pos.x + 30, pos.y+5));
                //    if(ImGui::Button("do thing", ImVec2(70, 30)))
                //    {
                //        ImGui::OpenPopup("Setup?");
                //        selected = n;
                //        printf("SETUP CLICKED %d\n", n);
                //    }
                //
                //    if (ImGui::BeginPopupModal("Setup?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                //    {
                //        ImGuiContext& g = *GImGui;
                //        ImGuiWindow* window = g.CurrentWindow;
                //        ImVec2 pos_before = window->DC.CursorPos;
                //
                //        ImGui::Text("Setup Popup");
                //        if (ImGui::Button("OK", ImVec2(120, 0))) { printf("OK PRESSED!\n"); ImGui::CloseCurrentPopup(); }
                //        ImGui::EndPopup();
                //    }
                //
                //    ImGui::SetCursorPos(ImVec2(pos.x, pos.y+20));
                //    ImGui::Text("bar");
                //
                //    pos.y += 55;
                //
                //    ImGui::PopID();
                //}
            }

            // Output Waveform
            {
                if (ImPlot::BeginPlot("Output Signal", ImVec2(-1, 400), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
                    ImPlotAxisFlags axisFlag = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoHighlight;
                    if (outPlotInput.bInputDirty)
                    {
                        axisFlag |= ImPlotAxisFlags_AutoFit;
                        outPlotInput.bInputDirty = false;
                    }

                    ImPlot::SetupAxes("Time (s)", NULL, axisFlag | ImPlotAxisFlags_PanStretch | ImPlotAxisFlags_LockMin, axisFlag | ImPlotAxisFlags_Lock);
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0f, 10.0f);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1f, 1.1f);
                    ImPlot::SetNextLineStyle(ImColor(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLineG("data", [](int i, void* data) -> ImPlotPoint {
                        PlotInput* pi = (PlotInput*)data;
                        float value = pi->signal->data[i * pi->sampleStride];
                        float time = (float)i * pi->sampleStride / pi->signal->sampleRate;

                        return ImPlotPoint(time, value);
                    }, &outPlotInput, std::min(outPlotInput.maxSamples, (uint32_t)outPlotInput.signal->data.size()));

                    ImPlot::EndPlot();
                }

                if (ImGui::Button("Save", buttonSize))
                {
                    nfdchar_t* saveFilePath = nullptr;
                    nfdresult_t res = NFD_SaveDialog("wav", nullptr, &saveFilePath);
                    if (res == NFD_OKAY)
                    {
	                    WriteSignal(outputSignal, saveFilePath);
                        messageSaveSuccesful = "Saved";
                        free(saveFilePath);
                    }
                    else if (res == NFD_ERROR)
                    {
                        messageSaveSuccesful = "Failed to save";
                    }
                    else
                    {
                        messageSaveSuccesful = "";
                    }
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save to disk (.wav only).");
                ImGui::SameLine();
                if (ImGui::Button("Play", buttonSize))
                {
                    PlayBufferAsAudio(outputSignal.data.data(), outputSignal.data.size(), outputSignal.sampleRate);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Playback audio signal.");
                ImGui::SameLine();
                if (ImGui::Button("Stop", buttonSize))
                {
                    StopAudio();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop audio playback.");
                ImGui::SameLine();
                if (ImGui::Button("Clear", buttonSize))
                {
                    outputSignal.data.clear();
                    outPlotInput = PlotInput(&outputSignal);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop audio playback.");

                ImGui::Text(messageSaveSuccesful.c_str());
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Generator Selection Menu
        if (bShowGenerators)
        {
            ImGui::Begin("Generators", &bShowGenerators);
            if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Escape)) bShowGenerators = false;

            static int generatorFrequency = 440;

            static ImPlotSubplotFlags subPlotFlags = ImPlotSubplotFlags_NoTitle | ImPlotSubplotFlags_NoLegend | ImPlotSubplotFlags_NoMenus | ImPlotSubplotFlags_NoResize;
            static int rows = 2;
            static int cols = 2;
            if (ImPlot::BeginSubplots("Presets", rows, cols, ImVec2(-1, 400), subPlotFlags, nullptr, nullptr)) {
                ImPlotFlags plotFlags = ImPlotFlags_NoInputs | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText;

                if (ImPlot::BeginPlot("Sine", ImVec2(), plotFlags)) {
                    ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxesLimits(0, 1000, -1.1f, 1.1f);

                    ImPlot::SetNextLineStyle(ImColor(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLineG("data", [](int i, void* data) -> ImPlotPoint {
                        int f = *(int*)data;
                        float value = GenerateSignalAtSampleIndex(SineGenerator, i, f / 100.0f, 1000.0f);
                        return ImPlotPoint(i, value);
                        }, &generatorFrequency, 1000);
                    ImPlot::EndPlot();
                }

                if (ImPlot::BeginPlot("Square", ImVec2(), plotFlags)) {
                    ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxesLimits(0, 1000, -1.1f, 1.1f);

                    ImPlot::SetNextLineStyle(ImColor(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLineG("data", [](int i, void* data) -> ImPlotPoint {
                        int f = *(int*)data;
                        float value = GenerateSignalAtSampleIndex(SquareGenerator, i, f / 100.0f, 1000.0f);
                        return ImPlotPoint(i, value);
                        }, &generatorFrequency, 1000);
                    ImPlot::EndPlot();
                }

                if (ImPlot::BeginPlot("Triangle", ImVec2(), plotFlags)) {
                    ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxesLimits(0, 1000, -1.1f, 1.1f);

                    ImPlot::SetNextLineStyle(ImColor(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLineG("data", [](int i, void* data) -> ImPlotPoint {
                        int f = *(int*)data;
                        float value = GenerateSignalAtSampleIndex(TriangleGenerator, i, f / 100.0f, 1000.0f);
                        return ImPlotPoint(i, value);
                        }, &generatorFrequency, 1000);
                    ImPlot::EndPlot();
                }

                if (ImPlot::BeginPlot("Saw", ImVec2(), plotFlags)) {
                    ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxesLimits(0, 1000, -1.1f, 1.1f);

                    ImPlot::SetNextLineStyle(ImColor(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
                    ImPlot::PlotLineG("data", [](int i, void* data) -> ImPlotPoint {
                        int f = *(int*)data;
                        float value = GenerateSignalAtSampleIndex(SawGenerator, i, f / 100.0f, 1000.0f);
                        return ImPlotPoint(i, value);
                        }, &generatorFrequency, 1000);
                    ImPlot::EndPlot();
                }

                ImPlot::EndSubplots();
            }

            enum class WaveFormType { SINE, SQUARE, TRIANGLE, SAW, COUNT };
            static int waveFormType = (int)WaveFormType::SINE;
            const char* waveFormNames[(int)WaveFormType::COUNT] = { "Sine", "Square", "Triangle", "Saw" };
            const char* waveFormName = (waveFormType >= 0 && waveFormType < (int)WaveFormType::COUNT) ? waveFormNames[waveFormType] : "Unknown";
            ImGui::SliderInt("Wave Form", &waveFormType, 0, (int)(WaveFormType::COUNT) - 1, waveFormName);

            static int generatorSampleRate = 100000;
            static float generatorDuration = 1.0f;
            ImGui::DragInt("Frequency", &generatorFrequency, 1.0f, 50, 1000);
            ImGui::DragFloat("Duration (s)", &generatorDuration, 1.0f, 0.1f, 60.0f);

            if (ImGui::Button("Generate", buttonSize))
            {
                float(*generator)(float) = nullptr;

                if (waveFormType == (int)WaveFormType::SINE) generator = SineGenerator;
                else if (waveFormType == (int)WaveFormType::SQUARE) generator = SquareGenerator;
                else if (waveFormType == (int)WaveFormType::TRIANGLE) generator = TriangleGenerator;
                else if (waveFormType == (int)WaveFormType::SAW) generator = SawGenerator;

                inputSignal.sampleRate = GenerateSignal(generator, generatorFrequency, generatorSampleRate, generatorDuration, inputSignal.data);
                inPlotInput = PlotInput(&inputSignal);
            }

            ImGui::End();
        }

        ImGui::Render();
        glViewport(0, 0, windowWith, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}