#include "bw/armaTypes.hpp"
#include "dataServer.hpp"
#include "spdlog/spdlog.h"

#include "serverObserver.hpp"

#ifdef _DEBUG
#define HAS_GUI
#endif

#ifdef HAS_GUI
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

void logPacket(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    spdlog::info("debug message");
    spdlog::info("start");
    for (auto &variable : variables) {
        spdlog::info("\t{}: {}", potato::getTypeString(variable->type), variable->toString());
    }
    spdlog::info("end");
}

int main() {
    dataServer server;
    server.subscribe(potato::packetTypes::DEBUG_MESSAGE, logPacket);

    serverObserver observer(server);

    bool running = true;

#ifdef HAS_GUI
    glfwInit();

    GLFWwindow *app = glfwCreateWindow(640, 480, "AAR Server", nullptr, nullptr);
    glfwMakeContextCurrent(app);
    glfwSwapInterval(1); // Enable vsync

    gladLoadGL();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(app, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
#endif

    while (running) {
#ifdef HAS_GUI
        glfwPollEvents();
#endif

        observer.update();

#ifdef HAS_GUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int width, height;
        glfwGetWindowSize(app, &width, &height);
        try {
            observer.drawInfo(static_cast<float>(width), static_cast<float>(height));
        } catch (std::exception &e) {
            spdlog::error("Draw error - {}", e.what());
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(app, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(app);

        running = !glfwWindowShouldClose(app);
#endif
    }

#ifdef HAS_GUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(app);
    glfwTerminate();
#endif

    return 0;
}
