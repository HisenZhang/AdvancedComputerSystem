#include "DSP.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include "AudioFile/AudioFile.h"

#include <iostream>
#include <vector>
#include <string>

// TODO:
// - Implement different filter types (echo, low pass, high pass, band pass, 
// - Preset waveforms to use (test files and also saw/square/sine waves)
// - Some kind of interface? imgui?

#define TAU 6.2831855

std::vector<float> GenerateSineWave(float amplitude, float frequency, int numSamples) {
	std::vector<float> result(numSamples);

	for (int i = 0; i < numSamples; i++)
	{
		result[i] = amplitude * std::sin(frequency * i * TAU);
	}

	return result;
}

void WriteSignal(const std::vector<float>& signal, const std::string& path) {
	std::vector<std::vector<float>> channels = { signal };

	AudioFile<float> af;
	af.setAudioBuffer(channels);
	af.save(path);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main(int, char**)
{
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << "\n";
    });

    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "DSP Demo", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glfwSetKeyCallback(window, key_callback);
   // [](GLFWwindow* window, int key, int scancode, int action, int mods) {
   //     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
   // });

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.WantCaptureKeyboard = true;
    //io.WantCaptureMouse = true;
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

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

	std::cout << "Loading signals...\n";

	AudioFile<float> testSignal;
	testSignal.load("data/input/test-audio.wav");
	//thomasSignal.load("data/input/thomas.wav");
	//std::vector<float> sine = GenerateSineWave(1.0f, 440.0f, 441000 * 5);

	std::cout << "Loading filters...\n";
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
	//WriteSignal(result, "data/outSIMD.wav");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        ImGui::ShowDemoWindow();

        //if (ImGui::IsKeyPressed(ImGuiKey_Escape)) break;

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");
            ImGui::Text("This is some useful text.");

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
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
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}