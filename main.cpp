// **********************************************************************************
// Author: Vilja Seppänen https://github.com/vilja22
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Windows.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX
#include <math.h>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>          // vsnprintf, sscanf, printf
#include <stdlib.h>         // NULL, malloc, free, atoi
#include <iostream>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>

#define GL_SILENCE_DEPRECATION



// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}
// Show windows
bool bShowSettings = false;
ImVec2 miniscreen = {128, 128};
bool bScreen = false;
int activeKey = -1;
uint8_t activeRotatoryBindsp1 = 0;
uint8_t activeFnRotatoryBindsp1 = 1;
uint8_t activeRotatoryBindsp2 = 0;
uint8_t activeFnRotatoryBindsp2 = 1;
uint8_t activeKeyboardProfile = 0;
int brightnessp1 = 16;
int brightnessp2 = 16;
int timeToSleep = 2;
int timeToDim = 1;
static ImVec4 rgbColor1 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 rgbColor2 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
int rgbProfile1 = 0;
int rgbProfile2 = 3;

bool bResetting = false;

float longestUpdatef = 0.0f;
bool bGetPerformance = false;
bool updateRgb = false;

const char* buttonNames[] = {
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",
    "Profile\nSwitch", "Delete", "Insert", "Pg Up", "Pg Down", "End", "Home" ,
    "Up\nArrow" , "Down\nArrow" , "Left\nArrow" , "Right\nArrow", "Function"
};
const char* currentKeyNames1[] = {
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",
    "Profile\nSwitch", "Delete", "Insert", "Pg Up", "Pg Down", "End", "Home" ,
    "Up\nArrow" , "Down\nArrow" , "Left\nArrow" , "Right\nArrow", "Function"
};

const char* currentKeyNames2[] = {
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",
    "Profile\nSwitch", "Delete", "Insert", "Pg Up", "Pg Down", "End", "Home" ,
    "Up\nArrow" , "Down\nArrow" , "Left\nArrow" , "Right\nArrow", "Function"
};

const char* preLoadedRgbProfiles[] = {
     "Rainbow", "Pride", "Pink & Purple", "Custom Color", "Disabled"
};


uint8_t currentKeysp1[] = {24, 12, 13, 14, 15, 8 , 7, 10, 11, 25, 27, 29, 31, 30,
                     35,  0,  1,  2,  3,  4,  5,  6,  7, 26, 28, 33, 32, 34
};

uint8_t currentKeysp2[] = { 24, 12, 13, 14, 15, 8 , 7, 10, 11, 25, 27, 29, 31, 30,
                     35,  0,  1,  2,  3,  4,  5,  6,  7, 26, 28, 33, 32, 34
};

bool connection;
HANDLE hComm;
uint16_t longestUpdate = 0;
uint16_t avargeUpdate = 0;
static char lastCharPressed = 0;

void CharCallback(GLFWwindow* window, unsigned int codepoint) {
    lastCharPressed = static_cast<char>(codepoint);  // Store the character
}

// Creates connection to device
void createConnection()
{
    connection = false;
    DCB dcb = { 0 };
    COMMTIMEOUTS timeouts = { 0 };

    while (!connection){
        // Attempt to close any existing handle
        if (hComm != INVALID_HANDLE_VALUE) {
            CloseHandle(hComm);
        }

        hComm = CreateFileW(L"\\\\.\\COM13", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hComm == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open the serial port: COM13" << std::endl;
            break;
        }

        // Get the current configuration of the serial port
        dcb.DCBlength = sizeof(DCB);
        if (!GetCommState(hComm, &dcb)) {
            std::cerr << "Failed to get serial port state" << std::endl;
            CloseHandle(hComm);
            continue;
        }

        // Configure the serial port settings
        dcb.BaudRate = CBR_115200;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;

        if (!SetCommState(hComm, &dcb)) {
            std::cerr << "Failed to set serial port state" << std::endl;
            CloseHandle(hComm);
            continue;
        }

        // Set communication timeouts
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(hComm, &timeouts)) {
            std::cerr << "Failed to set serial port timeouts" << std::endl;
            CloseHandle(hComm);
            continue;
        }

        connection = true;
        std::cout << "Connected to COM13" << std::endl;
    }

    if (!connection) {
        std::cerr << "Could not establish a connection." << std::endl;
    }
}

int hasData(HANDLE hComm) {
    DWORD bytesAvailable = 0;
    DWORD dummy = 0;
    if (PeekNamedPipe(hComm, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
        return bytesAvailable > 0;
    }
    return 0;
}

uint8_t sendDataCommand(uint8_t command) {

    uint8_t sendData[1] = { command };

    DWORD bytesWritten;
    if (!WriteFile(hComm, sendData, sizeof(sendData), &bytesWritten, nullptr)) {
        std::cerr << "Failed to write to the serial port" << std::endl;
    }
    else {
        std::cout << "Sent " << bytesWritten << " bytes" << std::endl;
    }
    Sleep(100);
    // Read the device's response after sending all data
    uint8_t responseBuffer[1] = { 0 };
    DWORD bytesRead;
    if (ReadFile(hComm, responseBuffer, sizeof(responseBuffer), &bytesRead, nullptr)) {
        std::cout << "Received " << bytesRead << " bytes (" << responseBuffer << ")" << std::endl;
        // Process the response as needed
    }
    else {
        DWORD error = GetLastError();
        std::cerr << "Failed to read from the serial port. Error code: " << error << std::endl;
        createConnection(); // Resets connection
    }
    if (responseBuffer[0] == sendData[0]) {
        std::cout << "Response OK" << std::endl;
    }
    return responseBuffer[0];
}

void readAndDecodeData() {


    if (sendDataCommand(10) == 10) {
        std::cout << "Starting data transfer" << std::endl;

        uint8_t recivedData[74] = { 0 };
        // Read usb data
        for (int n = 0; n < 5; n++) {
            uint8_t buffer[16] = { 0 };
            DWORD bytesRead1;
            Sleep(100);
            if (ReadFile(hComm, buffer, sizeof(buffer), &bytesRead1, nullptr)) {
                if (bytesRead1 > 0) {
                    std::cout << "Received data: ";

                    // Print each byte in hexadecimal format
                    for (DWORD i = 0; i < bytesRead1; i++) {
                        recivedData[i + n * 16] = buffer[i];
                        std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
                    }
                    std::cout << std::endl;
                }
            }
            else {
                DWORD error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    std::cerr << "Failed to read from the serial port. Error code: " << error << std::endl;
                    createConnection(); // Resets connection
                }
            }
        }
        if (recivedData[73] == 10) {
            for (int i = 1; i < 28; i++) {
                currentKeysp1[i] = recivedData[i];
            }
            for (int i = 1; i < 28; i++) {
                currentKeysp2[i] = recivedData[i + 28];
            }
            activeRotatoryBindsp1 = recivedData[56];
            activeFnRotatoryBindsp1 = recivedData[57];
            activeRotatoryBindsp2 = recivedData[58];
            activeFnRotatoryBindsp2 = recivedData[59];
            rgbProfile1 = recivedData[60];
            rgbProfile2 = recivedData[61];

            rgbColor1.x = recivedData[62] / 255.0f;
            rgbColor1.y = recivedData[63] / 255.0f;
            rgbColor1.z = recivedData[64] / 255.0f;
            rgbColor2.x = recivedData[65] / 255.0f;
            rgbColor2.y = recivedData[66] / 255.0f;
            rgbColor2.z = recivedData[67] / 255.0f;


            brightnessp1 = recivedData[68];
            brightnessp2 = recivedData[69];
            timeToSleep = recivedData[70];
            timeToDim = recivedData[71];
            activeKeyboardProfile = recivedData[72];
            std::cout << "Data transfer complete" << std::endl;
        }
        else {
            std::cout << "Data transfer failed" << std::endl;
        }
    }
    else {
        std::cout << "Failed to start data transfer" << std::endl;
    }

}

void readData() {
    if (hasData) {
        uint8_t buffer[4] = { 0 };
        DWORD bytesRead;
        if (ReadFile(hComm, buffer, sizeof(buffer), &bytesRead, nullptr)) {
            if (bytesRead > 0) {
                std::cout << "Received data: ";

                // Print each byte in hexadecimal format
                for (DWORD i = 0; i < bytesRead; i++) {
                    std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
                }
                for (DWORD i = 0; i < bytesRead; i++) {
                    std::cout << (buffer[i]);
                }
               
                    longestUpdate = buffer[0] << 8;
                    longestUpdate |= buffer[1];
                    avargeUpdate = buffer[2] << 8;
                    avargeUpdate |= buffer[3];
                    longestUpdatef = longestUpdate / 1000.0f;
           
                std::cout << std::endl;
            }
        }
        else { // If fails to read usb data   
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
            {
                std::cerr << "Failed to read from the serial port. Error code: " << error << std::endl;
                createConnection(); // Resets connection
            }
        }
    }
}

void sendDataB1() {
    // Example initialization of encodedData and other variables
    uint8_t encodedData1[29] = { 0 };
    encodedData1[0] = 1;
    // Fill encodedData with values
    for (int i = 0; i < 28; i++) {
        encodedData1[i+1] = currentKeysp1[i];
    }

    DWORD bytesWritten;
    if (!WriteFile(hComm, encodedData1, sizeof(encodedData1), &bytesWritten, nullptr)) {
        std::cerr << "Failed to write to the serial port" << std::endl;
    }
    else {
        std::cout << "Sent " << bytesWritten << " bytes" << std::endl;
    }
    Sleep(100);
    // Read the device's response after sending all data
    uint8_t responseBuffer[1] = { 0 };
    DWORD bytesRead;
    if (ReadFile(hComm, responseBuffer, sizeof(responseBuffer), &bytesRead, nullptr)) {
        std::cout << "Received " << bytesRead << " bytes (" << responseBuffer << ")" << std::endl;
        // Process the response as needed
    }
    else {
        DWORD error = GetLastError();
        std::cerr << "Failed to read from the serial port. Error code: " << error << std::endl;
        createConnection(); // Resets connection
    }
}

void sendDataB2() {
    uint8_t encodedData2[29] = { 0 };
    encodedData2[0] = 2;
    for (int i = 0; i < 28; i++) {
        encodedData2[i+1] = currentKeysp2[i];
    }

    DWORD bytesWritten2;
    if (!WriteFile(hComm, encodedData2, sizeof(encodedData2), &bytesWritten2, nullptr)) {
        std::cerr << "Failed to write to the serial port" << std::endl;
    }
    else {
        std::cout << "Sent " << bytesWritten2 << " bytes" << std::endl;
    }
    Sleep(100);
    // Read the device's response after sending all data
    uint8_t responseBuffer2[1] = { 0 };
    DWORD bytesRead2;
    if (ReadFile(hComm, responseBuffer2, sizeof(responseBuffer2), &bytesRead2, nullptr)) {
        std::cout << "Received " << bytesRead2 << " bytes (" << responseBuffer2 << ")" << std::endl;
        // Process the response as needed
    }
    else {
        DWORD error = GetLastError();
        std::cerr << "Failed to read from the serial port. Error code: " << error << std::endl;
        createConnection(); // Resets connection
    }
}

void sendDataRgb() {
    uint8_t encodedData3[18] = { 0 };
    encodedData3[0] = 3;
    encodedData3[1] = activeRotatoryBindsp1;
    encodedData3[2] = activeFnRotatoryBindsp1;
    encodedData3[3] = activeRotatoryBindsp2;
    encodedData3[4] = activeFnRotatoryBindsp2;
    encodedData3[5] = rgbProfile1;
    encodedData3[6] = rgbProfile2;

    uint8_t rgbR1 = static_cast<uint8_t>(rgbColor1.x * 255);
    uint8_t rgbG1 = static_cast<uint8_t>(rgbColor1.y * 255);
    uint8_t rgbB1 = static_cast<uint8_t>(rgbColor1.z * 255);
    uint8_t rgbR2 = static_cast<uint8_t>(rgbColor2.x * 255);
    uint8_t rgbG2 = static_cast<uint8_t>(rgbColor2.y * 255);
    uint8_t rgbB2 = static_cast<uint8_t>(rgbColor2.z * 255);

    encodedData3[7] = rgbR1;
    encodedData3[8] = rgbG1;
    encodedData3[9] = rgbB1;
    encodedData3[10] = rgbR2;
    encodedData3[11] = rgbG2;
    encodedData3[12] = rgbB2;
    if (rgbProfile1 == 3) {
        brightnessp1 = 255;
    }
    if (rgbProfile2 == 3) {
        brightnessp2 = 255;
    }

    encodedData3[13] = brightnessp1;
    encodedData3[14] = brightnessp2;
    encodedData3[15] = timeToSleep;
    encodedData3[16] = timeToDim;
    encodedData3[17] = activeKeyboardProfile;
    DWORD bytesWritten3;
    if (!WriteFile(hComm, encodedData3, sizeof(encodedData3), &bytesWritten3, nullptr)) {
        std::cerr << "Failed to write to the serial port" << std::endl;
    }
    else {
        std::cout << "Sent " << bytesWritten3 << " bytes" << std::endl;
    }

    // Read the device's response after sending all data
    uint8_t responseBuffer3[1] = { 0 };
    while (responseBuffer3[0] == 0) {
        DWORD bytesRead3;
        if (ReadFile(hComm, responseBuffer3, sizeof(responseBuffer3), &bytesRead3, nullptr)) {
            std::cout << "Received " << bytesRead3 << " bytes (" << responseBuffer3 << ")" << std::endl;
            // Process the response as needed
        }
        else {
            DWORD error = GetLastError();
            std::cerr << "Failed to read from the serial port. Error code: " << error << std::endl;
            createConnection(); // Resets connection
        }
    }


}



int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1050, 530, "Keyboard Extension", NULL, NULL);
    if (window == NULL)
        return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glfwSetCharCallback(window, CharCallback);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    bool stoppingCode = false;
   
    float textSize = 1;

    bool fullscreen = false;
    ImVec2 butttonSize = {60, 60};


    createConnection();
    readAndDecodeData();

    using namespace std::chrono;
    auto lastReadTime = high_resolution_clock::now();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
      

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

        auto now = high_resolution_clock::now();
        auto elapsed = duration_cast<seconds>(now - lastReadTime);

        if (elapsed.count() >= 5) { // Check if 1 second has passed
            readData();
            lastReadTime = now; // Update the time of the last read
        }

        if (updateRgb) {
            sendDataRgb();
            updateRgb = false;
        }

        if (bResetting) {
            Sleep(500);
            createConnection();
            readAndDecodeData();
            bResetting = false;
        }

        if (activeKey != -1 && lastCharPressed > 35) {
            if (activeKeyboardProfile == 0) {
                currentKeysp1[activeKey] = lastCharPressed;
            }
            else {
                currentKeysp2[activeKey] = lastCharPressed;
            }
            activeKey = -1;
            lastCharPressed = 0;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 1) {
            activeKey = -1;
            lastCharPressed = 0;
            if (bShowSettings) {
                bShowSettings = false;
            }
        }

        // Buttions
        ImGui::SetNextWindowPos({ 0, 20 });
        ImGui::SetNextWindowSize({ 1100, 700 });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::Begin("Current Setup", nullptr, window_flags);
        for (int n = 0; n <= (IM_ARRAYSIZE(buttonNames) + std::size(currentKeysp1)); n++)
        {       
            if (n < std::size(currentKeysp1)) {
                // Our buttons are both drag sources and drag targets here!

                ImGui::PushID(n);
                if (n == 1) {
                    ImGui::SameLine(0, 20);
                }
                else if (n == 0) {
                    ImGui::SameLine(0, 0);
                }
                else if (n == 5 or n == 9 or n == 15 or n == 19) {
                    ImGui::SameLine(0, 30);
                }
                else if (n == 11 or n == 23 or n == 25) {
                    ImGui::SameLine(0, 20);
                }
                else if (n != 14) {
                    ImGui::SameLine();
                }
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.8f, 1.0f });
                if (n == activeKey) {
                    ImGui::PopStyleColor(1);
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.5f, 0.5f, 0.8f, 1.0f });
                }
                
                if (n == 0) {
                    ImGui::PopStyleVar();
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,30.0f);
                }
                if (n == 1) {
                    ImGui::PopStyleVar();
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                }

                if (activeKeyboardProfile == 0) {
                    if (currentKeysp1[n] > 35) {
                        char lable[2] = {static_cast<char>(currentKeysp1[n]), '\0'};
                        if (ImGui::Button(lable, (butttonSize)))
                        {
                            activeKey = n;
                        }
                    }
                    else {
                        if (ImGui::Button(currentKeyNames1[currentKeysp1[n]], butttonSize))
                        {
                            activeKey = n;
                        }
                    }
                }
                else {
                    if (currentKeysp2[n] > 35) {
                        char lable[2] = { static_cast<char>(currentKeysp2[n]), '\0' };
                        if (ImGui::Button(lable, (butttonSize)))
                        {
                            activeKey = n;
                        }
                    }
                    else {
                        if (ImGui::Button(currentKeyNames2[currentKeysp2[n]], butttonSize))
                        {
                            activeKey = n;
                        }
                    }
                }

                ImGui::PopStyleColor(1);

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    // Set payload to carry the index of our item (could be anything)
                    if (activeKeyboardProfile == 0) {
                        ImGui::SetDragDropPayload("DND_DEMO_CELL", &currentKeysp1[n], sizeof(int));
                        ImGui::Text("Copy %s", buttonNames[currentKeysp1[n]]);
                    }
                    else {
                        ImGui::SetDragDropPayload("DND_DEMO_CELL", &currentKeysp1[n], sizeof(int));
                        ImGui::Text("Copy %s", buttonNames[currentKeysp2[n]]);
                    }
                    // Display preview (could be anything, e.g. when dragging an image we could decide to display
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(int));
                        int payload_n = *(const int*)payload->Data;
                        if (activeKeyboardProfile == 0) {
                            currentKeysp1[n] = payload_n;
                        }
                        else {
                            currentKeysp2[n] = payload_n;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::PopID();

            }
            else if (n == std::size(currentKeysp1)) {
                ImGui::Spacing();
                ImGui::Text("Drag to applay");
            }
            else if (n != std::size(currentKeysp1)){
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.0f, 0.8f, 1.0f });

                int i = n - std::size(currentKeysp1)-1;
                // Our buttons are both drag sources and drag targets here!
                ImGui::PushID(i);

                if (i != 0 and i != 12 and i != 24) {
                    ImGui::SameLine();
                }
                
                ImGui::Button(buttonNames[i], butttonSize);
                ImGui::PopStyleColor(1);
                
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    // Set payload to carry the index of our item (could be anything)
                    ImGui::SetDragDropPayload("DND_DEMO_CELL", &i, sizeof(int));
                    ImGui::Text("Copy %s", buttonNames[i]);
                    // Display preview (could be anything, e.g. when dragging an image we could decide to display
                    ImGui::EndDragDropSource();
                }
                ImGui::PopID();
            }         
        }

        const char* rotatoryProfiles[] = {
            "Volume Up & Down", "F21 & F22", "Right & Left Arrow", "Ctrl + Z & Ctrl + Y", "Disabled"
        };
        const char* rotatoryProfiles2[] = {
            " Volume Up & Down ", " F21 & F22 ", " Right & Left Arrow ", " Ctrl + Z & Ctrl + Y ", " Disabled "
        };
        ImGui::Spacing();
        ImGui::Text("Scroll Binds");
        ImGui::SameLine(0,103);
        ImGui::Text("Function + Scroll");
        ImGui::SameLine(0, 77);
        ImGui::Text("Profile");
        if (activeKeyboardProfile == 0) {
            for (int n = 0; n < std::size(rotatoryProfiles); n++) {

                ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.0f, 0.8f, 1.0f });
                if (n == activeRotatoryBindsp1) {
                    ImGui::PopStyleColor(1);
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.8f, 1.0f });
                }
                if (ImGui::Button(rotatoryProfiles[n], { 180, 20 })) {
                    activeRotatoryBindsp1 = n;
                }
                ImGui::PopStyleColor(1);

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.0f, 0.8f, 1.0f });
                if (n == activeFnRotatoryBindsp1) {
                    ImGui::PopStyleColor(1);
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.8f, 1.0f });
                }
                if (ImGui::Button(rotatoryProfiles2[n], { 180, 20 })) {
                    activeFnRotatoryBindsp1 = n;
                }
                ImGui::PopStyleColor(1);
            }
        }
        else {
            for (int n = 0; n < std::size(rotatoryProfiles); n++) {

                ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.0f, 0.8f, 1.0f });
                if (n == activeRotatoryBindsp2) {
                    ImGui::PopStyleColor(1);
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.8f, 1.0f });
                }
                if (ImGui::Button(rotatoryProfiles[n], { 180, 20 })) {
                    activeRotatoryBindsp2 = n;
                }
                ImGui::PopStyleColor(1);

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.0f, 0.8f, 1.0f });
                if (n == activeFnRotatoryBindsp2) {
                    ImGui::PopStyleColor(1);
                    ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.8f, 1.0f });
                }
                if (ImGui::Button(rotatoryProfiles2[n], { 180, 20 })) {
                    activeFnRotatoryBindsp2 = n;
                }
                ImGui::PopStyleColor(1);
            }
        }

        const char* keyboardProfiles[] = {"1", "2"};
        
        ImGui::SetCursorPos({ 390, 389 });
        for (int n = 0; n < std::size(keyboardProfiles); n++) {
            if (n != 0) {
                ImGui::SameLine();
            }
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.15f, 0.1f, 0.2f, 1.0f });

            if (n == activeKeyboardProfile) {
                ImGui::PopStyleColor(1);
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.4f, 0.2f, 0.8f, 1.0f });
            }
            if (ImGui::Button(keyboardProfiles[n], { 105, 116 })) {
                activeKeyboardProfile = n;
                updateRgb = true;
            }
            ImGui::PopStyleColor(1);
        }
       

        ImGui::SetCursorPos({ 625, 375});
        ImGui::PushStyleColor(ImGuiCol_Button, {0.4f, 0.0f, 0.8f, 1.0f});
        if (ImGui::Button("Load Current Settings", { 190, 35 })) {
            readAndDecodeData();
        }
        ImGui::SetCursorPos({ 625, 420 });
        if (ImGui::Button("Save to Keyboard", { 190, 35 })) {
            sendDataB1();
            sendDataB2();
            sendDataRgb();
            Sleep(50);
            if (sendDataCommand(99) == 99) {
                std::cout << "Settings saved to keyboard" << std::endl;
            }
            else {
                std::cout << "Saving settings failed" << std::endl;
            }
        }
        ImGui::SetCursorPos({ 625, 465 });
        if (ImGui::Button("Reset unsaved", { 190, 35 })) {
            if (sendDataCommand(110) == 110) {
                std::cout << "Restarting keyboard" << std::endl;
            }
            ImGui::SetCursorPos({ 300, 150 });

            ImGui::Button("Restarting device", { 300, 200 });
            bResetting = true;
            

        }
        ImGui::PopStyleVar();

        ImGui::PopStyleColor(1);
        if (bGetPerformance) {
            ImGui::SetCursorPos({ 840, 440 });
            ImGui::Text("Performance");
            ImGui::SetCursorPos({ 840, 460 });
            ImGui::Text("Logest update %f", longestUpdatef);
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::SetCursorPos({ 840, 480 });
            ImGui::Text("Avarage updates %5d", avargeUpdate);
            ImGui::SameLine();
            ImGui::Text("/s");
        }


        ImGui::End();


        window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoAlpha;

        ImGui::SetNextWindowPos({ 830, 195 });
        ImGui::SetNextWindowSize({ 200, 300 });
        ImGui::Begin("RGB", nullptr, window_flags);
        if (activeKeyboardProfile == 0) {
            ImGui::Text("RGB profiles");
            if (ImGui::Combo(" ", &rgbProfile1, preLoadedRgbProfiles, IM_ARRAYSIZE(preLoadedRgbProfiles), IM_ARRAYSIZE(preLoadedRgbProfiles))) {
                updateRgb = true;
            }
            if (rgbProfile1 == 3) {
                if (ImGui::ColorPicker4("MyColor##4", (float*)&rgbColor1, color_flags)) {
                    updateRgb = true;
                }
            }
            else {
                ImGui::Text("Brightness");
                if (ImGui::SliderInt("Profile 1", (int*)&brightnessp1, 0, 127)) {
                    updateRgb = true;
                }
            }
        }
        else {
            ImGui::Text("RGB profiles");
            if (ImGui::Combo(" ", &rgbProfile2, preLoadedRgbProfiles, IM_ARRAYSIZE(preLoadedRgbProfiles), IM_ARRAYSIZE(preLoadedRgbProfiles))) {
                updateRgb = true;
            }
            if (rgbProfile2 == 3) {
                if (ImGui::ColorPicker4("MyColor##4", (float*)&rgbColor2, color_flags)) {
                    updateRgb = true;
                }
            }
            else {
                ImGui::Text("Brightness");
                if (ImGui::SliderInt("Profile 2", (int*)&brightnessp2, 0, 127)){
                    updateRgb = true;
                }
            }
        }
        ImGui::End();

        // This is for when i make the same device again but with screen :)
        if (bScreen) { 
            if (ImGui::Begin("Screen"))
            {
                ImGui::SetWindowSize(miniscreen);
                ImGui::SetWindowFontScale(1.4f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::SetCursorPos(ImVec2(8, 22));
                ImGui::Text("Volume: 30");
                ImGui::SetCursorPos(ImVec2(4, 45));
                ImGui::ProgressBar(0.3, ImVec2(120.0f, 15.0f), " ");
                ImGui::SetCursorPos(ImVec2(8, 60));
                ImGui::Text("CPU: -- C");
                ImGui::PopStyleVar();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::End();
            }
        }

        // Main menu bar
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::MenuItem("Open Settings    ", NULL, &bShowSettings)) {}

            ImGui::Checkbox("Show Demo Window    ", &show_demo_window);
            if (ImGui::MenuItem("Quit    ")) 
            {
                stoppingCode = true;
            }
            ImGui::EndMainMenuBar();
        }
        // Settings
        if (bShowSettings)
        {
            ImGui::Begin("Settings");
            ImGui::Text("Sleepmode time");
            if (ImGui::SliderInt("minutes", (int*)&timeToSleep, 0, 30)) {
                if (timeToDim > timeToSleep) {
                    timeToDim = timeToSleep;
                }
                updateRgb = true;
            }
            ImGui::Text("Dimming time");
            if (ImGui::SliderInt("minutes ", (int*)&timeToDim, 0, 30)) {
                if (timeToDim > timeToSleep) {
                    timeToSleep = timeToDim;
                }
                updateRgb = true;
            }
            ImGui::Text("0 = Never sleep / dim");

            if (ImGui::Checkbox("Get performance", &bGetPerformance)) {
                if (!bGetPerformance) {
                    sendDataCommand(120);
                }
                else {
                    sendDataCommand(121);
                }
            }
            if (ImGui::Button("Exit Settings"))
            {
                bShowSettings = false;
            }
            ImGui::End();
        }
       
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
       
        // Stop code
        if (stoppingCode == true) break;
    }

    if (hComm != INVALID_HANDLE_VALUE) {
        CloseHandle(hComm);
        hComm = INVALID_HANDLE_VALUE;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
