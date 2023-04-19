#include "DSP.h"

#include <iostream>
#include <string>

// TODO:
// - Implement different filter types
// - Benchmarking
// - Center convolution
// - Fix issue with thomas file
// - Undefined behavior in AVX code ?

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
    io.Fonts->AddFontFromFileTTF("data/fonts/Ruda/Ruda-Bold.ttf", 16);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    {
        style.WindowPadding = ImVec2(15, 15);
        style.WindowRounding = 5.0f;
        style.FramePadding = ImVec2(5, 5);
        style.FrameRounding = 4.0f;
        style.ItemSpacing = ImVec2(12, 8);
        style.ItemInnerSpacing = ImVec2(8, 6);
        style.IndentSpacing = 25.0f;
        style.GrabMinSize = 5.0f;
        style.GrabRounding = 3.0f;
        style.Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.09f, 0.22f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
    }

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
    std::future<Signal> outSignalFuture;

    bool bShowGenerators = false;
    ImVec2 buttonSize(190, 50);
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
                if (ImPlot::BeginPlot("Input Signal", ImVec2(-1, 400), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus)) {
                    ImPlotAxisFlags axisFlag = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoHighlight;
                    if (inPlotInput.bInputDirty)
                    {
                        axisFlag |= ImPlotAxisFlags_AutoFit;
                        inPlotInput.bInputDirty = false;
                    }

                    ImPlot::SetupAxes("Time (s)", NULL, axisFlag /*| ImPlotAxisFlags_PanStretch | ImPlotAxisFlags_LockMin*/, axisFlag | ImPlotAxisFlags_Lock);
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
                if (ImGui::Button("Generators", buttonSize)) bShowGenerators = !bShowGenerators;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Generate signal from preset waveform.");
                ImGui::SameLine();
                if (ImGui::Button("Play##Input", buttonSize))
                {
                    PlaySignal(inputSignal);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Playback audio signal.");
                ImGui::SameLine();
                if (ImGui::Button("Stop##Input", buttonSize))
                {
                    StopAudio();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop audio playback.");
                ImGui::SameLine();
                if (ImGui::Button("Clear##Input", buttonSize))
                {
                    inputSignal.data.clear();
                    inPlotInput = PlotInput(&inputSignal);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear output signal.");
            }

            ImGui::Separator();

            // Effect List
            {
                ImGui::Text("Audio Effects");
                int toRemove = -1;
                for (uint32_t i = 0; i < effects.size(); i++)
                {
                    effects[i]->DrawGUI();
                    std::string name = "Remove##" + std::to_string(i);
                    if (ImGui::Button(name.c_str())) toRemove = i;
                    ImGui::Separator();
                }

                if (toRemove > -1) effects.erase(effects.begin() + toRemove);

                const float width = ImGui::GetWindowWidth();
                const float combo_width = width * 0.12f;
                ImGui::SetNextItemWidth(combo_width);

                const char* effectNames[] = { "Delay", "Derivative" };
                static int effectIndex = 0;
                ImGui::Combo("##effectNames", &effectIndex, effectNames, IM_ARRAYSIZE(effectNames));
                ImGui::SameLine();
                if (ImGui::Button("Add Effect"))
                {
                    if (effectIndex == 0) effects.emplace_back(new DelayEffect);
                    else if (effectIndex == 1) effects.emplace_back(new DerivativeEffect);
                }

                if (bFilterThreadRunning)
                {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                static bool bUseAVX = true;
                static bool bFilterThreadStartedThisTime = false;
                static bool bFilterThreadWasRunning = false;
                if (ImGui::Button("Apply", ImVec2(128, 45)))
                {
                    if (inputSignal.data.size() > 0)
                    {
                        if (effects.size() == 0)
                        {
                            outputSignal = inputSignal;
                            outPlotInput = PlotInput(&outputSignal);
                        }
                        else
                        {
                            bFilterThreadWasRunning = true;
                            bFilterThreadRunning.store(true);
                            bFilterThreadStartedThisTime = true;

                            outputSignal.data.clear();

                            std::promise<Signal> outSignalPromise;
                            outSignalFuture = outSignalPromise.get_future();
                            ApplyEffectsAsync(inputSignal, outSignalPromise, effects, bUseAVX);
                        }
                    }
                }

                ImGui::SameLine();
                ImGui::Checkbox("Use AVX", &bUseAVX);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Use AVX instructions to accelerate filter processing.");
                ImGui::Separator();

                if (bFilterThreadWasRunning && !bFilterThreadRunning)
                {
                    outputSignal = outSignalFuture.get();
                    outPlotInput = PlotInput(&outputSignal);
                    bFilterThreadWasRunning = false;
                }

                if (bFilterThreadRunning && !bFilterThreadStartedThisTime)
                {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar();
                    //ImGui::SameLine();
                    ImSpinner::SpinnerAng("Spinner", 16.0f, 6.0f, ImSpinner::white, ImColor(255, 255, 255, 128), 4.0f, TAU / 4.0f);
                }

                bFilterThreadStartedThisTime = false;
            }

            // Output Waveform
            {
                if (ImPlot::BeginPlot("Output Signal", ImVec2(-1, 400), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus)) {
                    ImPlotAxisFlags axisFlag = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoHighlight;
                    if (outPlotInput.bInputDirty)
                    {
                        axisFlag |= ImPlotAxisFlags_AutoFit;
                        outPlotInput.bInputDirty = false;
                    }

                    ImPlot::SetupAxes("Time (s)", NULL, axisFlag /*| ImPlotAxisFlags_PanStretch | ImPlotAxisFlags_LockMin*/, axisFlag | ImPlotAxisFlags_Lock);
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
                if (ImGui::Button("Play##Output", buttonSize))
                {
                    PlaySignal(outputSignal);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Playback audio signal.");
                ImGui::SameLine();
                if (ImGui::Button("Stop##Output", buttonSize))
                {
                    StopAudio();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop audio playback.");
                ImGui::SameLine();
                if (ImGui::Button("Clear##Output", buttonSize))
                {
                    outputSignal.data.clear();
                    outPlotInput = PlotInput(&outputSignal);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear output signal.");

                ImGui::Text(messageSaveSuccesful.c_str());
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Generator Selection Menu
        if (bShowGenerators)
        {
            ImGui::Begin("Generators", &bShowGenerators);
            ImGui::SetWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
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
            ImGui::DragFloat("Duration (s)", &generatorDuration, 1.0f, 0.1f, 10.0f);

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