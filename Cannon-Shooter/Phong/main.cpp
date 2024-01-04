#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <thread>
#include "shader.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "texture.hpp"




int WindowWidth = 1600;
int WindowHeight = 1200;
const float TargetFPS = 60.0f;
const std::string WindowTitle = "Cannon Shooter";


struct Input {
    bool MoveLeft;
    bool MoveRight;
    bool MoveForward;
    bool MoveBackward;
    bool MoveUp;
    bool MoveDown;
    bool LookLeft;
    bool LookRight;
    bool LookUp;
    bool LookDown;
};

struct EngineState {
    Input* mInput;
    Camera* mCamera;
    float mDT;
};

static void
ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

static void
KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    EngineState* State = (EngineState*)glfwGetWindowUserPointer(window);
    Input* UserInput = State->mInput;
    bool IsDown = action == GLFW_PRESS || action == GLFW_REPEAT;
    switch (key) {
    case GLFW_KEY_A: UserInput->MoveLeft = IsDown; break;
    case GLFW_KEY_D: UserInput->MoveRight = IsDown; break;
    case GLFW_KEY_W: UserInput->MoveForward = IsDown; break;
    case GLFW_KEY_S: UserInput->MoveBackward = IsDown; break;
    case GLFW_KEY_LEFT_SHIFT: UserInput->MoveDown = IsDown; break;
    case GLFW_KEY_SPACE: UserInput->MoveUp = IsDown; break;

    case GLFW_KEY_RIGHT: UserInput->LookLeft = IsDown; break;
    case GLFW_KEY_LEFT: UserInput->LookRight = IsDown; break;
    case GLFW_KEY_UP: UserInput->LookUp = IsDown; break;
    case GLFW_KEY_DOWN: UserInput->LookDown = IsDown; break;

    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    }
}

static void
FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    WindowWidth = width;
    WindowHeight = height;
    glViewport(0, 0, width, height);
}

static void
HandleInput(EngineState* state) {
    Input* UserInput = state->mInput;
    Camera* FPSCamera = state->mCamera;
    if (UserInput->MoveLeft) FPSCamera->Move(-1.0f, 0.0f, 0.0f ,state->mDT);
    if (UserInput->MoveRight) FPSCamera->Move(1.0f, 0.0f, 0.0f ,state->mDT);
    if (UserInput->MoveBackward) FPSCamera->Move(0.0f, -1.0f, 0.0f ,state->mDT);
    if (UserInput->MoveForward) FPSCamera->Move(0.0f, 1.0f, 0.0f ,state->mDT);
    if (UserInput->MoveDown) FPSCamera->Move(0.0f, 0.0f, -1.0f ,state->mDT);
    if (UserInput->MoveUp) FPSCamera->Move(0.0f, 0.0f, 1.0f ,state->mDT);

    if (UserInput->LookLeft) FPSCamera->Rotate(1.0f, 0.0f, state->mDT);
    if (UserInput->LookRight) FPSCamera->Rotate(-1.0f, 0.0f, state->mDT);
    if (UserInput->LookDown) FPSCamera->Rotate(0.0f, -1.0f, state->mDT);
    if (UserInput->LookUp) FPSCamera->Rotate(0.0f, 1.0f, state->mDT);
}


static void
DrawFloor(unsigned vao, const Shader& shader, unsigned texture) {
    glUseProgram(shader.GetId());
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, NULL);
    float Size = 15.0f;
    for (int i = -8; i < 16; ++i) {
        for (int j = -8; j < 16; ++j) {
            glm::mat4 Model(1.0f);
            Model = glm::translate(Model, glm::vec3(i * Size, 0.0f, j * Size));
            Model = glm::scale(Model, glm::vec3(Size, 0.1f, Size));
            shader.SetModel(Model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void RenderPalmAt(glm::mat4& ModelMatrix, Shader* CurrentShader, Model& Palm, glm::vec3 position)
{
    ModelMatrix = glm::mat4(1.0f);
    ModelMatrix = glm::translate(ModelMatrix, position);
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.02f, 0.02f, 0.02f));
    CurrentShader->SetModel(ModelMatrix);
    Palm.Render();
}

void AddPalms(glm::mat4& ModelMatrix, Shader* CurrentShader, Model& Palm)
{
    float x = 40.0f;
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(x, 0.0f, x));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(x, 0.0f, -x));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(-x, 0.0f, x));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(-x, 0.0f, -x));

    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(x / 2, 0.0f, x));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(x / 2, 0.0f, -x));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(-x / 2, 0.0f, x));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(-x / 2, 0.0f, -x));

    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(x, 0.0f, x / 2));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(x, 0.0f, -x / 2));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(-x, 0.0f, x / 2));
    RenderPalmAt(ModelMatrix, CurrentShader, Palm, glm::vec3(-x, 0.0f, -x / 2));
}

int main() {
    GLFWwindow* Window = 0;
    if (!glfwInit()) {
        std::cerr << "Failed to init glfw" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle.c_str(), 0, 0);
    if (!Window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(Window);

    GLenum GlewError = glewInit();
    if (GlewError != GLEW_OK) {
        std::cerr << "Failed to init glew: " << glewGetErrorString(GlewError) << std::endl;
        glfwTerminate();
        return -1;
    }

    EngineState State = { 0 };
    Camera FPSCamera;
    Input UserInput = { 0 };
    State.mCamera = &FPSCamera;
    State.mInput = &UserInput;
    glfwSetWindowUserPointer(Window, &State);

    glfwSetErrorCallback(ErrorCallback);
    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    glfwSetKeyCallback(Window, KeyCallback);

    glViewport(0.0f, 0.0f, WindowWidth, WindowHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    #pragma region texture_loading

    unsigned CubeDiffuseTexture = Texture::LoadImageToTexture("res/don.jpeg");
    unsigned CubeSpecularTexture = Texture::LoadImageToTexture("res/container_specular.png");
    unsigned FloorTexture1 = Texture::LoadImageToTexture("res/sand.jpg");
    unsigned FloorTexture2 = Texture::LoadImageToTexture("res/beach.jpg");
    unsigned SkyboxTexture = Texture::LoadImageToTexture("res/skybox.png");
    
    #pragma endregion 

    #pragma region cube_setup
    std::vector<float> CubeVertices = {
        // X     Y     Z     NX    NY    NZ    U     V    FRONT SIDE
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // R U
        -0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
                                                        // LEFT SIDE
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // RIGHT SIDE
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // BOTTOM SIDE
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // TOP SIDE
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // BACK SIDE
         0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // L D
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // R U
         0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
    };

    unsigned CubeVAO;
    glGenVertexArrays(1, &CubeVAO);
    glBindVertexArray(CubeVAO);
    unsigned CubeVBO;
    glGenBuffers(1, &CubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, CubeVertices.size() * sizeof(float), CubeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    #pragma endregion 

    #pragma region model_load

    Model Cat("res/maxwell_the_cat_dingus/scene.obj");
    if (!Cat.Load()) {
        std::cerr << "Failed to load ooeeaaii\n";
        glfwTerminate();
        return -1;
    }

    //PRECNIK JE 0.2f
    Model Beachball("res/beachball3/scene.obj");
    if (!Beachball.Load()) {
        std::cerr << "Failed to load beachball\n";
        glfwTerminate();
        return -1;
    }

    Model Palm("res/palm/scene.obj");
    if (!Palm.Load()) {
        std::cerr << "Failed to load palm\n";
        glfwTerminate();
        return -1;
    }
    #pragma endregion

    #pragma region light_and_shader_setup

    Shader ColorShader("shaders/color.vert", "shaders/color.frag");

    Shader PhongShaderMaterialTexture("shaders/basic.vert", "shaders/phong_material_texture.frag");
    glUseProgram(PhongShaderMaterialTexture.GetId());
    // Adjust directional light (sun)
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Direction", glm::vec3(1.0f, -1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ka", glm::vec3(0.8f, 0.8f, 0.6f));  // Warm ambient color
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Kd", glm::vec3(0.9f, 0.9f, 0.7f));  // Diffuse color
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ks", glm::vec3(1.0f, 1.0f, 1.0f));  // Specular color

    // Adjust point light (simulating a distant light source)
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ka", glm::vec3(0.2f, 0.2f, 0.2f));  // Ambient component
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Kd", glm::vec3(0.8f, 0.8f, 0.6f));  // Diffuse component
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ks", glm::vec3(1.0f, 1.0f, 1.0f));  // Specular component
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kq", 0.032f);

    // Adjust spotlight (simulating the sun casting shadows)
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Position", glm::vec3(0.0f, 5.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Direction", glm::vec3(0.0f, -1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ka", glm::vec3(0.1f, 0.1f, 0.1f));  // Ambient component
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Kd", glm::vec3(0.8f, 0.8f, 0.6f));  // Diffuse component
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ks", glm::vec3(1.0f, 1.0f, 1.0f));  // Specular component
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.InnerCutOff", glm::cos(glm::radians(5.0f)));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.OuterCutOff", glm::cos(glm::radians(15.0f)));


    // Adjust material properties for a sandy appearance
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Kd", 0);
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Ks", 1);
    PhongShaderMaterialTexture.SetUniform1f("uMaterial.Shininess", 70.0f);  // Reduce shininess for a rough surface
    glUseProgram(0);

    #pragma endregion


    glm::mat4 Projection = glm::perspective(45.0f, WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
    glm::mat4 View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
    glm::mat4 ModelMatrix(1.0f);
    

    float TargetFrameTime = 1.0f / TargetFPS;
    float StartTime = glfwGetTime();
    float EndTime = glfwGetTime();
    glClearColor(0.1f, 0.1f, 0.2f, 0.0f);



    
    Shader* CurrentShader = &PhongShaderMaterialTexture;
    while (!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
        HandleInput(&State);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // NOTE(Jovan): In case of window resize, update projection. Bit bad for performance to do it every iteration.
        // If laggy, remove this line
        //Projection = glm::perspective(45.0f, WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
        View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
        StartTime = glfwGetTime();

        glUseProgram(CurrentShader->GetId());
        CurrentShader->SetProjection(Projection);
        CurrentShader->SetView(View);
        CurrentShader->SetUniform3f("uViewPos", FPSCamera.GetPosition());
        
        #pragma region dynamic_elements_draw

        /*float verticalOffset = CatVerticalMotionAmplitude * sin(CatRotationAngle / 2);
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(20.0f, 1.0f + verticalOffset, 20.0f));
        ModelMatrix = glm::rotate(ModelMatrix, CatRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
        CurrentShader->SetModel(ModelMatrix);
        Cat.Render();*/
        
        float scaling = 8.0f;
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(2.0f, 0.2f * scaling , 2.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(scaling,scaling,scaling));
        CurrentShader->SetModel(ModelMatrix);
        Beachball.Render();

        #pragma endregion

        #pragma region static_elements_draw

        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, CubeDiffuseTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, CubeSpecularTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        AddPalms(ModelMatrix, CurrentShader, Palm);
        DrawFloor(CubeVAO, *CurrentShader, FloorTexture1);

        #pragma endregion


        glBindVertexArray(0);
        glUseProgram(0);
        glfwSwapBuffers(Window);

        // NOTE(Jovan): Time management
        EndTime = glfwGetTime();
        float WorkTime = EndTime - StartTime;
        if (WorkTime < TargetFrameTime) {
            int DeltaMS = (int)((TargetFrameTime - WorkTime) * 1000.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(DeltaMS));
            EndTime = glfwGetTime();
        }
        State.mDT = EndTime - StartTime;
    }

    glfwTerminate();
    return 0;
}



