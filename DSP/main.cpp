#include "DSP.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#include "GLFW/glfw3.h" // Will drag system OpenGL headers

#include <iostream>
#include <vector>
#include <string>

// TODO:
// - Create a useful interface
//   - load file or choose a preset signal
//   - add and layer effects to the audio
//   - sliders for signal and effect parameters (frequency, sample rate, amplitude, phase, etc.)
//   - display waveforms
// - Preset waveforms to use (test files and also sine/square/sawtooth/triangle)
// - Implement different filter types (echo, low pass, high pass, band pass, 
// - Close window button + esc to close?
// - Run audio playback on a separate thread

void InitializePresets() {

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
    //glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    //    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
    //});

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
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

	std::cout << "Loading signals...\n";

	AudioFile<float> testSignal;
	testSignal.load("data/input/test-audio.wav");
	//thomasSignal.load("data/input/thomas.wav");
	std::vector<float> sine = GenerateSineWave(440.0f, 44100.0f, 44100);

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

    //PlayBufferAsAudio(sine.data(), sine.size(), 44100);

    bool bShouldStayOpen = true;
    while (!glfwWindowShouldClose(window) && bShouldStayOpen)
    {
        glfwPollEvents();
        //if (ImGui::IsKeyPressed(ImGuiKey_Escape)) break; // TODO: escape to exit

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();
        ImGui::Begin("Hello, world!", &bShouldStayOpen);
        ImGui::Text("This is some useful text.");
        //if (ImGui::Button("Button")) counter++;
        //ImGui::SameLine();
        //ImGui::Text("counter = %d", counter);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

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