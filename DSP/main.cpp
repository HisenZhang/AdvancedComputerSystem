#include "DSP.h"

#include "imgui.h"
#include "implot/implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#include "GLFW/glfw3.h" // Will drag system OpenGL headers
#include "nfd.h"

#include <iostream>
#include <vector>
#include <string>

// TODO:
// - Create a useful interface
//   - plot preset waveforms in table
//   - load file or choose a preset signal
//   - add and layer effects to the audio
//   - sliders for signal and effect parameters (frequency, sample rate, amplitude, phase, etc.)
//   - display result waveforms
// - Preset waveforms to use (test files and also sine/square/sawtooth/triangle)
// - Implement different filter types (echo, low pass, high pass, band pass, etc.)
// - Close window button + esc to close?
// - Run audio playback on a separate thread
// - Source groups

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
    GLFWwindow* window = glfwCreateWindow(500, 500, "DSP Tool", nullptr, nullptr);
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

    /*
	std::vector<float> sine = GenerateSineWave(440.0f, 44100.0f, 44100);
	std::vector<float> impulseResponse = { -1.0f, 0.0f, 1.0f };
	std::vector<float> derivativeImpulseResponse = {
		-0.03926588615294601,
		-0.05553547436862413,
		-0.07674634732792313,
		-0.10359512127942656,
		-0.13653763745381112,
		-0.17563154711765214,
		-0.22037129189056356,
		-0.26953979617440665,
		-0.32110731416014476,
		-0.37220987669580885,
		-0.41923575145442643,
		-0.45803722114653117,
		-0.48426719093924325,
		-0.493817805141109  ,
		-0.483315193354812  ,
		-0.4506055551298345 ,
		-0.39515794257351333,
		-0.31831200150042305,
		-0.22331589501120253,
		-0.11512890376975546,
		0.0                 ,
		0.11512890376975546 ,
		0.22331589501120253 ,
		0.31831200150042305 ,
		0.39515794257351333 ,
		0.4506055551298345  ,
		0.483315193354812   ,
		0.493817805141109   ,
		0.48426719093924325 ,
		0.45803722114653117 ,
		0.41923575145442643 ,
		0.37220987669580885 ,
		0.32110731416014476 ,
		0.26953979617440665 ,
		0.22037129189056356 ,
		0.17563154711765214 ,
		0.13653763745381112 ,
		0.10359512127942656 ,
		0.07674634732792313 ,
		0.05553547436862413 ,
	};
	std::vector<float> echoImpulseResponse;
	for (int i = 0; i < 44100; i++) {
		echoImpulseResponse.push_back(0.0f);
	} echoImpulseResponse[0] = 1.0f; echoImpulseResponse.push_back(1.0f);

	FilterInput testEchoFilter(testSignal.samples[0], echoImpulseResponse);

	std::cout << "Applying filter (non-SIMD)...\n";
	std::vector<float> result;
	//applyFirFilter(testEchoFilter, result);
	//WriteSignal(result, "data/out.wav");

	std::cout << "Applying filter (SIMD)...\n";
	result.clear();
	//applyFirFilterAVX(testEchoFilter, result);
    */

    Signal inputSignal, outputSignal;
    PlotInput mainPlotInput(&inputSignal);
    std::string messageSaveSuccesful = "";

    bool bShowGenerators = false;

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

        ImPlot::ShowDemoWindow();
        ImGui::ShowDemoWindow();

        // Main Window
        {
            ImGui::Begin("DSP Tool", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration);
            ImGui::SetWindowSize(ImVec2(windowWith, windowHeight));
            ImGui::SetWindowPos(ImVec2(windowX, windowY));

            // Input Waveform
            {
                if (ImPlot::BeginPlot("Input Signal", ImVec2(-1, 400), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
                    ImPlotAxisFlags axisFlag = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoHighlight;
                    if (mainPlotInput.bInputDirty)
                    {
                        axisFlag |= ImPlotAxisFlags_AutoFit;
                        mainPlotInput.bInputDirty = false;
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
                    }, &mainPlotInput, std::min(mainPlotInput.maxSamples, (uint32_t)mainPlotInput.signal->data.size()));

                    ImPlot::EndPlot();
                }

                if (ImGui::Button("Load File"))
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
                        mainPlotInput = PlotInput(&inputSignal);
                    }
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load an audio file (.wav only).");
                ImGui::SameLine();
                if (ImGui::Button("Generate")) bShowGenerators = true;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Generate signal from preset waveform.");

                if (ImGui::Button("Save"))
                {
                    nfdchar_t* saveFilePath = nullptr;
                    nfdresult_t res = NFD_SaveDialog("wav", nullptr, &saveFilePath);
                    if (res == NFD_OKAY)
                    {
	                    WriteSignal(inputSignal, saveFilePath);
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
                if (ImGui::Button("Play"))
                {
                    PlayBufferAsAudio(inputSignal.data.data(), inputSignal.data.size(), inputSignal.sampleRate); // TODO: should be output signal
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Playback audio signal.");

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

            if (ImGui::Button("Generate"))
            {
                float(*generator)(float) = nullptr;

                if (waveFormType == (int)WaveFormType::SINE) generator = SineGenerator;
                else if (waveFormType == (int)WaveFormType::SQUARE) generator = SquareGenerator;
                else if (waveFormType == (int)WaveFormType::TRIANGLE) generator = TriangleGenerator;
                else if (waveFormType == (int)WaveFormType::SAW) generator = SawGenerator;

                inputSignal.sampleRate = GenerateSignal(generator, generatorFrequency, generatorSampleRate, generatorDuration, inputSignal.data);
                mainPlotInput = PlotInput(&inputSignal);
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