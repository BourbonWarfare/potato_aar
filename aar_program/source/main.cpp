#include "bw/armaTypes.hpp"
#include "dataServer.hpp"
#include "spdlog/spdlog.h"

#include "eventProcessor.hpp"
#include "projectileTracker.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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

    eventProcessor eventHandler(server);
    projectileTracker projectileHandler(server);

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

    while (!glfwWindowShouldClose(app)) {
        projectileHandler.update();

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("Server Overview", false, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            int width, height;
            glfwGetWindowSize(app, &width, &height);

            ImGui::SetWindowSize({ static_cast<float>(width), static_cast<float>(height) });
            ImGui::SetWindowPos({ 0, 0 });

            if (ImGui::BeginTabBar("##ViewTabs")) {
                eventHandler.drawInfo();
                projectileHandler.drawInfo();
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(app, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(app);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(app);
    glfwTerminate();

    return 0;
}
