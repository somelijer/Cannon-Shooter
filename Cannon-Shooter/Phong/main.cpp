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
#include "stb_image.h"
#include "physics.hpp"
#include <list>
#include <random>
using namespace std;




int WindowWidth = 1600;
int WindowHeight = 900;
const float TargetFPS = 60.0f;
const std::string WindowTitle = "Cannon Shooter";
double lastX = 90;
double lastY = 90;
int PlayerScore = 0;
list<Sphere*> SphereList;
list<Plane*> PlaneList;
list<Cylinder*> CylinderList;
list<glm::vec3> PalmPositionsList;
float LastShootTime = glfwGetTime();
float CannonUpperShootLimit = 60.0f;
float CannonLowerShootLimit = 5.0f;

bool MovementDebug = false;
bool MovementDebugFreeze = true;
float MovementStep = 1.5F / TargetFPS;

float CannonError = 0.01f;

bool CatAnimationActive = false;
int CatAnimationCounter = 0;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> CannonErrorDistribution(-CannonError, CannonError);
std::uniform_real_distribution<float> BalloonPositionDistribution(10.0f, 30.0f);

glm::vec3 balloonPos;

float CatRotationAngle = glm::radians(-3.1419f);

struct Input {
    bool MoveLeft;
    bool MoveRight;
    bool MoveForward;
    bool MoveBackward;
    bool MoveUp;
    bool MoveDown;
    bool CannonLeft;
    bool CannonRight;
    bool CannonUp;
    bool CannonDown;
    bool ShootCannon;
    bool CannonUpStrenght;
    bool CannonDownStrenght;
};

struct CannonState {
    float mPitch;
    float mYaw;
    glm::vec3 mBarrelEnd;
    glm::vec3 mForwardVector;
    float mStrenght;
};


struct EngineState {
    Input* mInput;
    Camera* mCamera;
    CannonState* mCannonState;
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

    case GLFW_KEY_RIGHT: UserInput->CannonLeft = IsDown; break;
    case GLFW_KEY_LEFT: UserInput->CannonRight = IsDown; break;
    case GLFW_KEY_UP: UserInput->CannonUp = IsDown; break;
    case GLFW_KEY_DOWN: UserInput->CannonDown = IsDown; break;
    case GLFW_KEY_ENTER: UserInput->ShootCannon = IsDown; break;
    case GLFW_KEY_KP_ADD: UserInput->CannonUpStrenght = IsDown; break;
    case GLFW_KEY_KP_SUBTRACT: UserInput->CannonDownStrenght = IsDown; break;
    case GLFW_KEY_F: MovementDebugFreeze = IsDown; break;
  
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    }

}

bool IsFreezed() {
    if (!MovementDebugFreeze) {
        MovementDebugFreeze = true;
        return false;
    }
    return true;
}

static void
FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    WindowWidth = width;
    WindowHeight = height;
    glViewport(0, 0, width, height);
}

static void
mouseCallback(GLFWwindow* window, double xPos, double yPos) {
    float deltaX = xPos - lastX;
    float deltaY = lastY - yPos;  

    EngineState* State = (EngineState*)glfwGetWindowUserPointer(window);
    Camera* mCamera = State->mCamera;

    float sensitivity = 0.2f;
    deltaX *= sensitivity;
    deltaY *= sensitivity;

    mCamera->UpdateOrientation(deltaX, deltaY);

    lastX = xPos;
    lastY = yPos;
}

void UpdateCannon(float deltaX, float deltaY,CannonState* state) {

    float CannonPitchLimit = 90.0f;
    state->mPitch += deltaY;
    state->mYaw += deltaX;

    if (state->mPitch > CannonPitchLimit) state->mPitch = CannonPitchLimit;
    if (state->mPitch < -CannonPitchLimit) state->mPitch = -CannonPitchLimit;
}

void ShootBall(EngineState* state) {

    glm::vec3 shootvector = state->mCannonState->mForwardVector;
    shootvector.x += CannonErrorDistribution(gen);
    shootvector.y += CannonErrorDistribution(gen);
    shootvector.z += CannonErrorDistribution(gen);


    if (glfwGetTime() - LastShootTime > 0.5) {
        float speed = 5.0f;
        Sphere* sphere = new Sphere{ 10.0f, 0.4f, state->mCannonState->mBarrelEnd, shootvector * state->mCannonState->mStrenght };
        SphereList.push_back(sphere);
        LastShootTime = glfwGetTime();
    }
}

void UpdateCannonStrenght(EngineState* state,float delta) {
    if (state->mCannonState->mStrenght + delta > CannonUpperShootLimit || state->mCannonState->mStrenght + delta < CannonLowerShootLimit) return;
    state->mCannonState->mStrenght += delta;
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

    if (UserInput->CannonLeft) UpdateCannon(1.0f, 0.0f,state->mCannonState);
    if (UserInput->CannonRight) UpdateCannon(-1.0f, 0.0f, state->mCannonState);
    if (UserInput->CannonDown) UpdateCannon(0.0f, -1.0f, state->mCannonState);
    if (UserInput->CannonUp) UpdateCannon(0.0f, 1.0f, state->mCannonState);
    if (UserInput->ShootCannon) ShootBall(state);
    if (UserInput->CannonUpStrenght) UpdateCannonStrenght(state,1.0f);
    if (UserInput->CannonDownStrenght) UpdateCannonStrenght(state,-1.0f);
}



static void
DrawFloor(unsigned vao, const Shader& shader, unsigned texture) {
    glUseProgram(shader.GetId());
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture);
    float Size = 15.0f;
    for (int i = -16; i < 32; ++i) {
        for (int j = -16; j < 32; ++j) {
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

void AddPalmLocations()
{
    float x = 40.0f;
    PalmPositionsList.push_back(glm::vec3(x, 0.0f, x));
    PalmPositionsList.push_back(glm::vec3(x, 0.0f, -x));
    PalmPositionsList.push_back(glm::vec3(-x, 0.0f, x));
    PalmPositionsList.push_back(glm::vec3(-x, 0.0f, -x));

    PalmPositionsList.push_back(glm::vec3(x, 0.0f, x / 2));
    PalmPositionsList.push_back(glm::vec3(x, 0.0f, -x / 2));
    PalmPositionsList.push_back(glm::vec3(-x, 0.0f, x / 2));
    PalmPositionsList.push_back(glm::vec3(-x, 0.0f, -x / 2));

    PalmPositionsList.push_back(glm::vec3(x / 2, 0.0f, x));
    PalmPositionsList.push_back(glm::vec3(x / 2, 0.0f, -x));
    PalmPositionsList.push_back(glm::vec3(-x / 2, 0.0f, x));
    PalmPositionsList.push_back(glm::vec3(-x / 2, 0.0f, -x));


    float spacing = 60.0f;

    for (float angle = 0.0f; angle < 360.0f; angle += 23.5f) {
        float radians = glm::radians(angle);
        float x = glm::cos(radians) * spacing;
        float z = glm::sin(radians) * spacing;
        PalmPositionsList.push_back(glm::vec3(x, 0.0f, z));
    }

    spacing = 90.0f;

    for (float angle = 12.0f; angle < 382.0f; angle += 16.8f) {
        float radians = glm::radians(angle);
        float x = glm::cos(radians) * spacing;
        float z = glm::sin(radians) * spacing;
        PalmPositionsList.push_back(glm::vec3(x, 0.0f, z));
    }

    for (glm::vec3 pos : PalmPositionsList) {
        glm::vec3 CylPos1 = glm::vec3(pos.x - 0.0f, 20.0f, pos.z - 0.8f);
        glm::vec3 CylPos2 = glm::vec3(pos.x, 0.0f, pos.z);
        CylinderList.push_back(new Cylinder{ 0.5f, CylPos1 , CylPos2 });
    }
}

void AddPalms(glm::mat4& ModelMatrix, Shader* CurrentShader, Model& Palm)
{
    for (glm::vec3 pos : PalmPositionsList) {
        RenderPalmAt(ModelMatrix, CurrentShader, Palm, pos);
    }
}

void SetupPhongLight(Shader PhongShaderMaterialTexture)
{
    // Adjust directional light (sun)
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Direction", glm::vec3(1.0f, -1.0f, 0.5f));
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
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Position", glm::vec3(100.0f, 100.0f, 50.5f));
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
}


unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void checkBalloonHit(Sphere* sphere,float balloonRadius, glm::vec3 ballonPosWithAmpl)
{
    glm::vec3 balloonCenterPos = ballonPosWithAmpl + glm::vec3(0.0f, 1.3f, 0.0f);

    float distance = glm::distance(sphere->Position, balloonCenterPos);


    if (distance < sphere->Radius + balloonRadius) {
        balloonPos = glm::vec3(BalloonPositionDistribution(gen), BalloonPositionDistribution(gen)  - 8.f, BalloonPositionDistribution(gen));
        PlayerScore += 1;
        std::cout << "Balloon popped! Player Score: " << PlayerScore << std::endl << std::endl;
        if (PlayerScore % 1 == 0) {
            CatRotationAngle = glm::radians(-3.1419f);
            CatAnimationActive = true;
        }
    }

}

void DoCatCelebration(float& CatRotationAngle, EngineState& State, float CatVerticalMotionAmplitude, glm::mat4& ModelMatrix, Shader* CurrentShader, Model& Cat,glm::vec3 CannonPos)
{

    CatRotationAngle += State.mDT * 8;
    float verticalOffset = CatVerticalMotionAmplitude * sin(CatRotationAngle / 2);

    float distanceBetweenObjects = 5.0f;


    glm::vec3 objectPositionLeft = CannonPos + distanceBetweenObjects * glm::normalize( glm::cross(State.mCannonState->mForwardVector, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 objectPositionRight = CannonPos -  distanceBetweenObjects * glm::normalize( glm::cross(State.mCannonState->mForwardVector, glm::vec3(0.0f, 1.0f, 0.0f)));

    ModelMatrix = glm::mat4(1.0f);
    ModelMatrix = glm::translate(ModelMatrix, glm::vec3(objectPositionLeft.x, 0.0f + verticalOffset, objectPositionLeft.z));
    ModelMatrix = glm::rotate(ModelMatrix, CatRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
    CurrentShader->SetModel(ModelMatrix);
    Cat.Render();

    ModelMatrix = glm::mat4(1.0f);
    ModelMatrix = glm::translate(ModelMatrix, glm::vec3(objectPositionRight.x, 0.0f + verticalOffset, objectPositionRight.z));
    ModelMatrix = glm::rotate(ModelMatrix, CatRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
    CurrentShader->SetModel(ModelMatrix);
    Cat.Render();

    CatAnimationCounter += 1;
    if (CatAnimationCounter >= 45) {
        CatAnimationCounter = 0;
        CatAnimationActive = false;
    }
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
    CannonState cannonInfo;
    cannonInfo.mPitch = 0.0f;
    cannonInfo.mYaw = 0.0f;
    cannonInfo.mStrenght = 10.0f;
    State.mCannonState = &cannonInfo;
    glfwSetWindowUserPointer(Window, &State);

    glfwSetErrorCallback(ErrorCallback);
    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    glfwSetKeyCallback(Window, KeyCallback);
    glfwSetCursorPosCallback(Window, mouseCallback);

    glViewport(0.0f, 0.0f, WindowWidth, WindowHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    #pragma region texture_loading

    vector<std::string> faces
    {
        "res/skybox/face_1.png",
        "res/skybox/face_2.png",
        "res/skybox/face_3.png",
        "res/skybox/face_4.png",
        "res/skybox/face_5.png",
        "res/skybox/face_6.png"
    };
    unsigned int cubemapTexture = Texture::LoadCubemap(faces);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL Error: " << error << std::endl;
    }


    unsigned CubeDiffuseTexture = Texture::LoadImageToTexture("res/don.jpeg");
    unsigned CubeSpecularTexture = Texture::LoadImageToTexture("res/container_specular.png");
    unsigned FloorTexture1 = Texture::LoadImageToTexture("res/sand.jpg");
    unsigned FloorTexture2 = Texture::LoadImageToTexture("res/beach.jpg");
    unsigned SkyboxTexture = Texture::LoadImageToTexture("res/skybox.png");
    unsigned MetalTexture = Texture::LoadImageToTexture("res/metal.jpg");
    unsigned RustyMetalTexture = Texture::LoadImageToTexture("res/rusty_metal.jpg");
    unsigned SignatureTexture = Texture::LoadImageToTexture("res/potpis.png");

    
    #pragma endregion 

    #pragma region vao_vbo_setup

    float signatureVertices[] =
    {
        //  X     Y      H    V
          -1.0, -1.0,   0.0, 0.0,
           1.0, -1.0,   1.0, 0.0,
           1.0,  1.0,   1.0, 1.0,

          -1.0, -1.0,   0.0, 0.0,
           1.0,  1.0,   1.0, 1.0,
          -1.0,  1.0,   0.0, 1.0
    };


    unsigned int VAO_signature, VBO_signature;
    glGenVertexArrays(1, &VAO_signature);
    glGenBuffers(1, &VBO_signature);

    // index and name attributes
    glBindVertexArray(VAO_signature);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_signature);

    glBufferData(GL_ARRAY_BUFFER, sizeof(signatureVertices), signatureVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


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

    #pragma region skybox_setup

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);



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

    Model Cannon("res/cannon/scene.obj");
    if (!Cannon.Load()) {
        std::cerr << "Failed to load cannon\n";
        glfwTerminate();
        return -1;
    }
    
    Model Balloon("res/balloon/scene.obj");
    if (!Balloon.Load()) {
        std::cerr << "Failed to load cannon\n";
        glfwTerminate();
        return -1;
    }


    #pragma endregion

    #pragma region light_and_shader_setup

    Shader SkyboxShader("shaders/skybox.vert", "shaders/skybox.frag");


    glUseProgram(SkyboxShader.GetId());
    SkyboxShader.SetUniform1i("skybox", 0);
    glUseProgram(0);

    Shader PhongShaderMaterialTexture("shaders/basic.vert", "shaders/phong_material_texture.frag");

    glUseProgram(PhongShaderMaterialTexture.GetId());
    SetupPhongLight(PhongShaderMaterialTexture);
    glUseProgram(0);
    

    Shader Color2dShader("shaders/2dcolor.vert", "shaders/2dcolor.frag");
    glUseProgram(0);

    #pragma endregion

    
    glm::mat4 Projection = glm::perspective(45.0f, WindowWidth / (float)WindowHeight, 0.1f, 200.0f);
    glm::mat4 View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
    glm::mat4 ModelMatrix(1.0f);

    float TargetFrameTime = 1.0f / TargetFPS;
    float StartTime = glfwGetTime();
    float EndTime = glfwGetTime();

    glClearColor(0.604f, 0.792f, 0.906f, 0.0f);


    float CatVerticalMotionAmplitude = 4.0f; 
    float CatVerticalMotionFrequency = 1.0f;

    float BalloonRotationAngle = glm::radians(45.0f);
    float BalloonVerticalMotionAmplitude = 1.0f;
    float BalloonVerticalMotionFrequency = 0.5f;

    glm::vec3 CannonPos = glm::vec3(5.0f, 3.2f, 0.0f);

    PlaneList.push_back(new Plane{ glm::vec3(0.0f, 1.0f, 0.0f), 0.1f });

    balloonPos =  glm::vec3(10.0f, 1.8f, -10.0f);
    
    AddPalmLocations();
    

    
    Shader* CurrentShader = &PhongShaderMaterialTexture;
    while (!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
        HandleInput(&State);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
        StartTime = glfwGetTime();
        



        glUseProgram(CurrentShader->GetId());
        CurrentShader->SetProjection(Projection);
        CurrentShader->SetView(View);
        CurrentShader->SetUniform3f("uViewPos", FPSCamera.GetPosition());

        
        #pragma region dynamic_elements_draw

        BalloonRotationAngle += State.mDT * 4;
        float balloonVerticalOffset = BalloonVerticalMotionAmplitude * sin(BalloonRotationAngle / 2);

        if (CatAnimationActive) {
            DoCatCelebration(CatRotationAngle, State, CatVerticalMotionAmplitude, ModelMatrix, CurrentShader, Cat,CannonPos);
        }



        glm::vec3 balloonPosWithAmplitude = glm::vec3(0.0f,balloonVerticalOffset , 0.0f) + balloonPos;

        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, balloonPosWithAmplitude);

        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.05f, 0.04f, 0.05f));
        CurrentShader->SetModel(ModelMatrix);
        Balloon.Render();



        float PitchRadians = glm::radians(State.mCannonState->mPitch);
        float YawRadians = glm::radians(State.mCannonState->mYaw);
        float CannonLenght = 2.5f;
        float CannonScale = 3.5f;
        glm::vec3 CannonCenterDelta = glm::vec3(0.0f, -0.4429f * CannonScale, 0.0f);

        glm::vec3 CannonTurnVector;
        CannonTurnVector.x = glm::sin(YawRadians);
        CannonTurnVector.y = 0.0f;
        CannonTurnVector.z = glm::cos(YawRadians);

        glm::vec3 CannonForwardVector;
        CannonForwardVector.z = -glm::sin(YawRadians) * glm::cos(PitchRadians);
        CannonForwardVector.y = glm::sin(PitchRadians);
        CannonForwardVector.x = glm::cos(YawRadians) * glm::cos(PitchRadians);
        State.mCannonState->mForwardVector = glm::normalize(CannonForwardVector);
        State.mCannonState->mBarrelEnd = CannonPos + CannonLenght * CannonForwardVector + glm::vec3(0.0f, 1.50f, 0.0f);

        glm::vec3 ballPosition = glm::vec3(State.mCannonState->mBarrelEnd);
        float scaling = 0.4f / 0.2f;

        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, ballPosition);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(scaling, scaling, scaling));
        CurrentShader->SetModel(ModelMatrix);
        Beachball.Render();

        float redScale = State.mCannonState->mStrenght / 60.0f;
        glm::vec3 redColor = glm::mix(glm::vec3(1.0f), glm::vec3(1.0f, 0.0f, 0.0f), redScale);
        glm::vec3 diffmix = glm::mix(glm::vec3(0.8f, 0.8f, 0.6f), glm::vec3(1.0f, 0.0f, 0.0f), redScale * 0.3f);

        // Use the calculated color
        PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ka", redColor * 0.3f);  // Ambient component
        PhongShaderMaterialTexture.SetUniform3f("uPointLight.Kd", redColor * 0.6f);  // Diffuse component
        PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ks", glm::vec3(1.0f, 1.0f, 1.0f));  // Specular component

        PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ka", redColor * 0.9f);  // Ambient component
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Kd", redColor * 0.4f);  // Diffuse component
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ks", glm::vec3(1.0f, 1.0f, 1.0f));  // Specular component

        // Directional light
        PhongShaderMaterialTexture.SetUniform3f("uDirLight.Kd", redColor * 0.6f);  // Diffuse component
        PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ks", glm::vec3(0.8f, 0.8f, 0.8f));  // Specular component





        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, CannonPos);
        ModelMatrix = glm::translate(ModelMatrix,- CannonCenterDelta);
        ModelMatrix = glm::rotate(ModelMatrix, PitchRadians, CannonTurnVector);
        ModelMatrix = glm::rotate(ModelMatrix, YawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, CannonCenterDelta);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(CannonScale));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, RustyMetalTexture);
        CurrentShader->SetModel(ModelMatrix);
        Cannon.Render();

        SetupPhongLight(*CurrentShader);

        #pragma endregion

        #pragma region movement

        if (MovementDebug) {

            bool freezed = IsFreezed();
            for (auto sphere : SphereList) {
                float scaling = sphere->Radius / 0.2f;
                ModelMatrix = glm::mat4(1.0f);
                ModelMatrix = glm::translate(ModelMatrix, sphere->Position);
                ModelMatrix = glm::scale(ModelMatrix, glm::vec3(scaling, scaling, scaling));
                CurrentShader->SetModel(ModelMatrix);
                Beachball.Render();

                if(!freezed)updateSphere(sphere, MovementStep);
                if (!freezed)checkBalloonHit(sphere,1.0f,balloonPosWithAmplitude );
            }

            if (!freezed)checkConstraints(SphereList,PlaneList,CylinderList);

        }
        else {

            for (auto sphere : SphereList) {
                float scaling = sphere->Radius / 0.2f;
                ModelMatrix = glm::mat4(1.0f);
                ModelMatrix = glm::translate(ModelMatrix, sphere->Position);
                ModelMatrix = glm::scale(ModelMatrix, glm::vec3(scaling, scaling, scaling));
                CurrentShader->SetModel(ModelMatrix);
                Beachball.Render();
                updateSphere(sphere, State.mDT);
                checkBalloonHit(sphere, 1.0f, balloonPosWithAmplitude);
            }

            checkConstraints(SphereList, PlaneList, CylinderList);
        }
     

        #pragma endregion

        #pragma region static_elements_draw


        AddPalms(ModelMatrix, CurrentShader, Palm);
        DrawFloor(CubeVAO, *CurrentShader, FloorTexture1);


        #pragma endregion

        glUseProgram(Color2dShader.GetId());
        glBindVertexArray(VAO_signature);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, SignatureTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);

        #pragma region skybox

        glUseProgram(0);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        glUseProgram(SkyboxShader.GetId());
        View = glm::mat4(glm::mat3(View)); // remove translation from the view matrix
        SkyboxShader.SetView(View);
        SkyboxShader.SetProjection(Projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
        glUseProgram(0);

        #pragma endregion
        


        glBindVertexArray(0);
        glUseProgram(0);
        glfwSwapBuffers(Window);

        if (MovementDebug) {
            EndTime = glfwGetTime();
            float WorkTime = EndTime - StartTime;
            if (WorkTime < TargetFrameTime) {
                int DeltaMS = (int)((TargetFrameTime - WorkTime) * 1000.0f);
                std::this_thread::sleep_for(std::chrono::milliseconds(DeltaMS));
                EndTime = glfwGetTime();
            }
            State.mDT = TargetFrameTime;
            continue;
        }

        EndTime = glfwGetTime();
        float WorkTime = EndTime - StartTime;
        if (WorkTime < TargetFrameTime) {
            int DeltaMS = (int)((TargetFrameTime - WorkTime) * 1000.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(DeltaMS));
            EndTime = glfwGetTime();
        }
        State.mDT = EndTime - StartTime;
    }

    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwTerminate();
    return 0;
}






