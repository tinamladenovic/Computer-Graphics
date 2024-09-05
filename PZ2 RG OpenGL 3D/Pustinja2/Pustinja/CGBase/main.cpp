#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include "shader.hpp"
#include "shader2.hpp"
#include "model.hpp" 
#include "renderable.hpp" 
#include "camera.hpp"
#include "stb_image.h"


void processInput(GLFWwindow* Window, Camera& camera);
void useShader(Shader2 shader2, glm::mat4 projection, glm::mat4 view);
int WindowWidth = 1900;
int WindowHeight = 600;
const std::string WindowTitle = "Pustinja";
const float TargetFPS = 60.0f;
const float TargetFrameTime = 1.0f / TargetFPS;

// carpet movement
float carpetX = 3.5;
float carpetZ = -4.0;

float orthoLeft = -10.0f;
float orthoRight = 10.0f;
float orthoBottom = -10.0f;
float orthoTop = 10.0f;
float orthoNear = 0.1f;
float orthoFar = 100.0f;


float zoomFactor = 1.0f; // Početno podešavanje na perspektivnu projekciju


struct Input {
    bool MoveLeft;
    bool MoveRight;
    bool MoveUp;
    bool MoveDown;
    bool LookLeft;
    bool LookRight;
    bool LookUp;
    bool LookDown;

    bool ZoomIn;
    bool ZoomOut;

    bool moveB;
};


struct EngineState {
    Input* mInput;
    Camera* mCamera;
    float mDT;
};
 
const float intensityMap[5][2] = {{0.7, 1.8} , {0.35, 0.44}, {0.22, 0.20}, {0.14, 0.07}, {0.09, 0.032}};

#pragma region Projection and view properties
glm::mat4 currentProjection;
glm::mat4 projectionPerspective;
glm::mat4 projectionOrtho;
glm::mat4 view;
#pragma endregion

#pragma region Camera properties
glm::vec3 cameraPos = glm::vec3(5.0f, 5.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(-5.0f, -5.0f, -5.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = 215.0f;//-215.0f;
float pitch = -40.0f;
#pragma endregion

static unsigned loadImageToTexture(const char* filePath);


static void
ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

static void
FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    WindowWidth = width;
    WindowHeight = height;
    glViewport(0, 0, width, height);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    EngineState* State = (EngineState*)glfwGetWindowUserPointer(window);
    Input* UserInput = State->mInput;
    bool IsDown = action == GLFW_PRESS || action == GLFW_REPEAT;
    switch (key) {
    case GLFW_KEY_A: UserInput->MoveLeft = IsDown; break;
    case GLFW_KEY_D: UserInput->MoveRight = IsDown; break;
    case GLFW_KEY_W: UserInput->MoveUp = IsDown; break;
    case GLFW_KEY_S: UserInput->MoveDown = IsDown; break;

    case GLFW_KEY_RIGHT: UserInput->LookLeft = IsDown; break;
    case GLFW_KEY_LEFT: UserInput->LookRight = IsDown; break;
    case GLFW_KEY_UP: UserInput->LookUp = IsDown; break;
    case GLFW_KEY_DOWN: UserInput->LookDown = IsDown; break;

        
    //case GLFW_KEY_J: UserInput->ZoomIn = IsDown; break;
    ///case GLFW_KEY_K: UserInput->ZoomOut = IsDown; break;
        
       


    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    }
}


static void
HandleInput(EngineState* state) {
    Input* UserInput = state->mInput;
    Camera* FPSCamera = state->mCamera;

    // Dodajte logiku za zoomiranje
    if (UserInput->ZoomIn) FPSCamera->ZoomIn();    // Implementirajte funkciju za zumiranje unapred u klasi Camera
    if (UserInput->ZoomOut) FPSCamera->ZoomOut();  // Implementirajte funkciju za zumiranje unazad u klasi Camera

    // Ostatak vaše postojeće logike za pomeranje i rotiranje kamere...
    if (UserInput->MoveLeft) FPSCamera->Move(-1.0f, 0.0f, state->mDT);
    if (UserInput->MoveRight) FPSCamera->Move(1.0f, 0.0f, state->mDT);
    if (UserInput->MoveDown) FPSCamera->Move(0.0f, -1.0f, state->mDT);
    if (UserInput->MoveUp) FPSCamera->Move(0.0f, 1.0f, state->mDT);

    if (UserInput->LookLeft) FPSCamera->Rotate(1.0f, 0.0f, state->mDT);
    if (UserInput->LookRight) FPSCamera->Rotate(-1.0f, 0.0f, state->mDT);
    if (UserInput->LookDown) FPSCamera->Rotate(0.0f, -1.0f, state->mDT);
    if (UserInput->LookUp) FPSCamera->Rotate(0.0f, 1.0f, state->mDT);
}





static void
DrawFloor(unsigned vao, const Shader& shader, unsigned diffuse, unsigned specular) {
    glUseProgram(shader.GetId());
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuse);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specular);
    float Size = 4.0f;
    for (int i = -2; i < 4; ++i) {
        for (int j = -2; j < 4; ++j) {
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




int main() {
    if (!glfwInit())
    {
        std::cout << "GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* Window;
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    unsigned int wWidth = mode->width;
    unsigned int wHeight = mode->height;


    const char wTitle[] = "Pustinja";
    Window = glfwCreateWindow(wWidth, wHeight, wTitle, glfwGetPrimaryMonitor(), NULL);
    if (Window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }
    glfwMakeContextCurrent(Window);
    glViewport(0, 0, wWidth, wHeight);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }

    
    

    
    
    EngineState State = { 0 };
    Camera FPSCamera;
    Input UserInput = { 0 };
    State.mCamera = &FPSCamera;
    State.mInput = &UserInput;
    glfwSetWindowUserPointer(Window, &State);

   

    glfwSetKeyCallback(Window, KeyCallback);
    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    

    GLenum GlewError = glewInit();
    if (GlewError != GLEW_OK) {
        std::cerr << "Failed to init glew: " << glewGetErrorString(GlewError) << std::endl;
        glfwTerminate();
        return -1;
    }


    //projectionPerspective = glm::perspective(glm::radians(45.0f), (float)wWidth / (float)wHeight, 0.1f, 100.0f);
    //projectionOrtho = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.05f, 500.0f);
    //currentProjection = projectionOrtho;


    Shader2 shader2("shaders/shader.vert", "shaders/shader.frag");
    Shader BasicShader("shaders/basic_old.vert", "shaders/basic.frag");
    Shader ColorShader("shaders/color.vert", "shaders/color.frag");
    Shader PhongShaderMaterialTexture("shaders/basic.vert", "shaders/phong_material_texture.frag");
    Shader2 signatureShader("shaders/signature.vert", "shaders/signature.frag");
    
    

    //Setup Phong shader
    glUseProgram(PhongShaderMaterialTexture.GetId());
    
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Direction", glm::vec3(20.0, 25.5, 10.0));
    //glm::vec3(0.75294f, 0.75294f, 0.75294f)
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ka", glm::vec3(0.75294f, 0.75294f, 0.75294f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Kd", glm::vec3(0.5f, 0.5f, 0.5f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ks", glm::vec3(1.0f));

    PhongShaderMaterialTexture.SetUniform3f("uPointLights[0].Ka", glm::vec3(1.0f, 0.843f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLights[0].Kd", glm::vec3(0.0f, 0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLights[0].Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[0].Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[0].Kl", intensityMap[0][0]);
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[0].Kq", intensityMap[0][1]);

    PhongShaderMaterialTexture.SetUniform3f("uPointLights[1].Ka", glm::vec3(1.0f, 0.843f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLights[1].Kd", glm::vec3(0.0f, 0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLights[1].Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[1].Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[1].Kl", intensityMap[4][0]);
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[1].Kq", intensityMap[4][1]);

    PhongShaderMaterialTexture.SetUniform3f("uPointLights[2].Ka", glm::vec3(1.0f, 0.843f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLights[2].Kd", glm::vec3(0.0f, 0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLights[2].Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[2].Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[2].Kl", intensityMap[4][0]);
    PhongShaderMaterialTexture.SetUniform1f("uPointLights[2].Kq", intensityMap[4][1]);

    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Position", glm::vec3(20.0, 25.5, 10.0));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ka", glm::vec3(1.0f, 1.0f, 1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Kd", glm::vec3(0.5f, 0.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kl", 0.022f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kq", 0.0019f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.InnerCutOff", glm::cos(glm::radians(1.0f)));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.OuterCutOff", glm::cos(glm::radians(1.5f)));
    
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Kd", 0);
    
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Ks", 1);
    PhongShaderMaterialTexture.SetUniform1f("uMaterial.Shininess", 128.0f);
    glUseProgram(0);


  

    float cubeVertices[] = 
    {
        -0.2, -0.2, -0.2,       0.0, 0.0, 0.0,
        +0.2, -0.2, -0.2,       0.0, 0.0, 0.0,
        -0.2, -0.2, +0.2,       0.0, 0.0, 0.0,
        +0.2, -0.2, +0.2,       0.0, 0.0, 0.0,

        -0.2, +0.2, -0.2,       0.0, 0.0, 0.0,
        +0.2, +0.2, -0.2,       0.0, 0.0, 0.0,
        -0.2, +0.2, +0.2,       0.0, 0.0, 0.0,
        +0.2, +0.2, +0.2,       0.0, 0.0, 0.0,
    };

    unsigned int cubeIndices[] = {
        3, 6, 2,
        3, 7, 6,

        1, 7, 3,
        1, 5, 7,

        4, 6, 7,
        4, 7, 5,

        0, 4, 1,
        1, 4, 5,

        0, 2, 6,
        0, 6, 4,

        0, 1, 3,
        0, 3, 2
        
    };

    Renderable cube(cubeVertices, sizeof(cubeVertices), cubeIndices, sizeof(cubeIndices));

    unsigned CubeDiffuseTexture = Texture::LoadImageToTexture("res/sand.jpeg");
    unsigned CubeSpecularTexture = Texture::LoadImageToTexture("res/sand_spec.jpg");
    unsigned PyramidDiffuseTexture = Texture::LoadImageToTexture("res/brick.jpg");
    unsigned MoonTexture = Texture::LoadImageToTexture("res/moon.jpg");
    unsigned WaterTexture = Texture::LoadImageToTexture("res/water.jpg");
    unsigned GrassTexture = Texture::LoadImageToTexture("res/grass.jpg");
    unsigned SunTexture = Texture::LoadImageToTexture("res/sun.jpg");
  

  

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

    std::vector<float> PyramidVertices = {
        // X     Y     Z     NX    NY    NZ    U     V    FRONT SIDE
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
         0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
         0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // R U
         0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
                                                        // LEFT SIDE
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.0f,  0.5f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.0f,  0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.0f,  0.5f, -0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // RIGHT SIDE
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
         0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
         0.0f,  0.5f, -0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
         0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // BOTTOM SIDE
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // TOP SIDE
        -0.0f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // L D
         0.0f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.0f,  0.5f, -0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
         0.0f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
         0.0f,  0.5f, -0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.0f,  0.5f, -0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // BACK SIDE
         0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // L D
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
         0.0f,  0.5f, -0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
        -0.0f,  0.5f, -0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // R U
         0.0f,  0.5f, -0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
    };

    unsigned PyramidVAO;
    glGenVertexArrays(1, &PyramidVAO);
    glBindVertexArray(PyramidVAO);
    unsigned PyramidVBO;
    glGenBuffers(1, &PyramidVBO);
    glBindBuffer(GL_ARRAY_BUFFER, PyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, PyramidVertices.size() * sizeof(float), PyramidVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

   


    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.7, 0.7, 1.0, 1.0);

    float signatureVertices[] = {
     0.7, -0.7,      0.0, 1.0, // top-left
     0.7, -1.0,      0.0, 0.0, // bottom-left
     1.0,  -0.7,     1.0, 1.0,  // top-right
     1.0, -1.0,      1.0, 0.0, // bottom-right
    };

    unsigned int signatureIndecies[] = {
        0, 1, 3,
        3, 2, 0
    };
    unsigned int signatureVAO, signatureVBO, signatureEBO;
    glGenVertexArrays(1, &signatureVAO);
    glGenBuffers(1, &signatureVBO);
    glGenBuffers(1, &signatureEBO);

    glBindVertexArray(signatureVAO);
    glBindBuffer(GL_ARRAY_BUFFER, signatureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(signatureVertices), signatureVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, signatureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(signatureIndecies), signatureIndecies, GL_STATIC_DRAW);

    glBindVertexArray(0);

    unsigned signatureTexture = loadImageToTexture("res/signature.png");
    glBindTexture(GL_TEXTURE_2D, signatureTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    signatureShader.setInt("uTex", 0);


    glm::mat4 m(1.0f);
    FPSCamera.SetBirdPerspective(45.0f, (float)WindowWidth / WindowHeight, 0.1f, 100.0f);
    glm::mat4 View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
    glm::mat4 p = glm::perspective(glm::radians(90.0f), (float)WindowWidth / WindowHeight, 0.1f, 100.0f);
   
    

    float angle = 0;
    float movement = 0;

    float carpetY = 0.55;
    float capetYOffset = 0.002;
    float offsetMultiplier = 1;
    
    glClearColor(0.074, 0.094, 0.384, 1.0);


    float cameraOffset = 0.0;

    float FrameStartTime = glfwGetTime();
    float FrameEndTime = glfwGetTime();
    float dt = FrameEndTime - FrameStartTime;

    int pointLightIntensity1 = 0;
    int pointLightIntensity2 = 4;
    int pointLightIntensity3 = 4;
    int point1Direction = 1;
    int point2Direction = -1;
    int point3Direction = -1;
    int counter = 0;



    float left = -1.0f;
    float right = 1.0f;
    float bottom = -1.0f;
    float top = 1.0f;
    float near = 0.1f;
    float far = 100.0f;
    float fov = 45.0f; 



    while (!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
        HandleInput(&State);
        processInput(Window, FPSCamera);

        useShader(shader2, currentProjection, view);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        FrameStartTime = glfwGetTime();

       // glm::mat4 View = glm::lookAt(birdEyePosition, birdTargetPosition, birdUpVector);
       // glm::mat4 p = glm::perspective(glm::radians(birdFov), birdAspectRatio, birdNearClip, birdFarClip);
        FPSCamera.SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));  // Postavljanje kamere iznad piramide
        glm::vec3 pyramidTop = glm::vec3(-3.0f, 15.0f, -3.5f);  // Pozicija vrha piramide
        FPSCamera.SetTarget(pyramidTop);  // Usmjeravanje kamere prema vrhu piramide

        FPSCamera.SetBirdPerspective(45.0f, (float)WindowWidth / WindowHeight, 0.1f, 100.0f);
        p = glm::perspective(glm::radians(90.0f), (float)WindowWidth / WindowHeight, 0.1f, 100.0f);
        View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
        

        


        glUseProgram(PhongShaderMaterialTexture.GetId());
        PhongShaderMaterialTexture.SetProjection(p);
        PhongShaderMaterialTexture.SetView(View);
        PhongShaderMaterialTexture.SetUniform3f("uViewPos", FPSCamera.GetPosition());

        glm::vec3 PointLightPosition(-3.0f, 12.0f, -3.5f);
        glm::vec3 PointLight2Position(7.5f, 5.0f, 0.0f);
        glm::vec3 PointLight3Position(3.5f, 8.0f, 6.0f); 
        
        PhongShaderMaterialTexture.SetUniform3f("uPointLights[0].Position", PointLightPosition);
        PhongShaderMaterialTexture.SetUniform3f("uPointLights[1].Position", PointLight2Position);
        PhongShaderMaterialTexture.SetUniform3f("uPointLights[2].Position", PointLight3Position);

        PhongShaderMaterialTexture.SetUniform1f("uPointLights[0].Kl", intensityMap[pointLightIntensity1][0]);
        PhongShaderMaterialTexture.SetUniform1f("uPointLights[0].Kq", intensityMap[pointLightIntensity1][1]);
    
        
        PhongShaderMaterialTexture.SetUniform1f("uPointLights[1].Kl", intensityMap[pointLightIntensity2][0]);
        PhongShaderMaterialTexture.SetUniform1f("uPointLights[1].Kq", intensityMap[pointLightIntensity2][1]);

        PhongShaderMaterialTexture.SetUniform1f("uPointLights[2].Kl", intensityMap[pointLightIntensity3][0]);
        PhongShaderMaterialTexture.SetUniform1f("uPointLights[2].Kq", intensityMap[pointLightIntensity3][1]);
        

        if (counter == 5) {
            
            if (pointLightIntensity1 == 0)
                point1Direction = 1;
            else if (pointLightIntensity1 == 4)
                point1Direction = -1;

            if (pointLightIntensity2 == 0)
                point2Direction = 1;
            else if (pointLightIntensity2 == 4)
                point2Direction = -1;

            if (pointLightIntensity3 == 0)
                point3Direction = 1;
            else if (pointLightIntensity3 == 4)
                point3Direction = -1;

            pointLightIntensity1 += point1Direction;
            pointLightIntensity2 += point2Direction;
            pointLightIntensity3 += point3Direction;
            counter = 0;
        }
        
        counter++;
        
        glm::mat4 ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(carpetX, carpetY - 1.0f, carpetZ)); 
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5, 0.01, 3.0));
        PhongShaderMaterialTexture.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        //sunce
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(20.0, 15.5, 10.0));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(4.0f));
        PhongShaderMaterialTexture.SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, SunTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);
        ModelMatrix = glm::rotate(ModelMatrix, glm::radians(45.0f), glm::vec3(1.0, 1.0, 1.0));
        PhongShaderMaterialTexture.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        //mesec
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-20.0, 15.5, 10.0)); 
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(4.0f));
        PhongShaderMaterialTexture.SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, MoonTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);


        //velika piramida
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-3.0f, 5.0f, -3.5f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(10.0f));
        PhongShaderMaterialTexture.SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, PyramidDiffuseTexture);
        glBindVertexArray(PyramidVAO);
        glDrawArrays(GL_TRIANGLES, 0, PyramidVertices.size() / 8);
        
        //manja piramida
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(7.5f, 1.5f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(4.0f));
        PhongShaderMaterialTexture.SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, PyramidDiffuseTexture);
        glBindVertexArray(PyramidVAO);
        glDrawArrays(GL_TRIANGLES, 0, PyramidVertices.size() / 8);

        //manja piramida 2
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-1.5f, 1.5f, 6.0f)); 
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(4.0f));
        PhongShaderMaterialTexture.SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, PyramidDiffuseTexture);
        glBindVertexArray(PyramidVAO);
        glDrawArrays(GL_TRIANGLES, 0, PyramidVertices.size() / 8);

        // voda
        glm::mat4 CubeModelMatrix = glm::mat4(1.0f);
        CubeModelMatrix = glm::translate(CubeModelMatrix, glm::vec3(7.0f, -0.7f, 8.0f));
        CubeModelMatrix = glm::scale(CubeModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(CubeModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, WaterTexture);
        glBindVertexArray(CubeVAO); 
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8); 


        


      
        //trava levo
        glm::mat4 LeftCubeModelMatrix = glm::mat4(1.0f);
        LeftCubeModelMatrix = glm::translate(LeftCubeModelMatrix, glm::vec3(5.0f, -0.9f, 8.0f)); 
        LeftCubeModelMatrix = glm::scale(LeftCubeModelMatrix, glm::vec3(2.0f)); 
        PhongShaderMaterialTexture.SetModel(LeftCubeModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,GrassTexture); 
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        //trava desno
        glm::mat4 RightCubeModelMatrix = glm::mat4(1.0f);
        RightCubeModelMatrix = glm::translate(RightCubeModelMatrix, glm::vec3(9.0f, -0.9f, 8.0f));
        RightCubeModelMatrix = glm::scale(RightCubeModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(RightCubeModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        // Trava napred
        glm::mat4 FrontCubeModelMatrix = glm::mat4(1.0f);
        FrontCubeModelMatrix = glm::translate(FrontCubeModelMatrix, glm::vec3(9.0f, -0.9f, 7.0f));
        FrontCubeModelMatrix = glm::scale(FrontCubeModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(FrontCubeModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        // Trava napred 2
        glm::mat4 FrontCube2ModelMatrix = glm::mat4(1.0f);
        FrontCube2ModelMatrix = glm::translate(FrontCube2ModelMatrix, glm::vec3(7.0f, -0.9f, 7.0f));
        FrontCube2ModelMatrix = glm::scale(FrontCube2ModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(FrontCube2ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        // Trava napred 3
        glm::mat4 FrontCube3ModelMatrix = glm::mat4(1.0f);
        FrontCube3ModelMatrix = glm::translate(FrontCube3ModelMatrix, glm::vec3(5.0f, -0.9f, 7.0f));
        FrontCube3ModelMatrix = glm::scale(FrontCube3ModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(FrontCube3ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        // Trava nazad
        glm::mat4 BackCubeModelMatrix = glm::mat4(1.0f);
        BackCubeModelMatrix = glm::translate(BackCubeModelMatrix, glm::vec3(5.0f, -0.9f, 9.0f));
        BackCubeModelMatrix = glm::scale(BackCubeModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(BackCubeModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        // Trava nazad 2
        glm::mat4 BackCube2ModelMatrix = glm::mat4(1.0f);
        BackCube2ModelMatrix = glm::translate(BackCube2ModelMatrix, glm::vec3(7.0f, -0.9f, 9.0f));
        BackCube2ModelMatrix = glm::scale(BackCube2ModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(BackCube2ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        // Trava nazad 3
        glm::mat4 BackCube3ModelMatrix = glm::mat4(1.0f);
        BackCube3ModelMatrix = glm::translate(BackCube3ModelMatrix, glm::vec3(9.0f, -0.9f, 9.0f));
        BackCube3ModelMatrix = glm::scale(BackCube3ModelMatrix, glm::vec3(2.0f));
        PhongShaderMaterialTexture.SetModel(BackCube3ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);
        

        //ime
        signatureShader.use();
        glBindVertexArray(signatureVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, signatureTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);


        DrawFloor(CubeVAO, PhongShaderMaterialTexture, CubeDiffuseTexture, CubeSpecularTexture);
        
        glUseProgram(ColorShader.GetId());
        ColorShader.SetProjection(p);
        ColorShader.SetView(View);
        
        //svetla iznad piramida
        m = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 12.0f, -3.5f));
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0f, 0.843f, 0.0f));
        ColorShader.SetModel(m);
        cube.Render();

        m = glm::translate(glm::mat4(1.0f), glm::vec3(7.5f, 5.0f, 0.0f));
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0f, 0.843f, 0.0f));
        ColorShader.SetModel(m);
        cube.Render();

        m = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 5.0f, 6.0f));
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0f, 0.843f, 0.0f));
        ColorShader.SetModel(m);
        cube.Render();


        glUseProgram(0);
        glfwSwapBuffers(Window);

        FrameEndTime = glfwGetTime();
        dt = FrameEndTime - FrameStartTime;
        if (dt < TargetFPS) {
            int DeltaMS = (int)((TargetFrameTime - dt) * 1e3f);
            std::this_thread::sleep_for(std::chrono::milliseconds(DeltaMS));
            FrameEndTime = glfwGetTime();
        }
        dt = FrameEndTime - FrameStartTime;
        State.mDT = FrameEndTime - FrameStartTime;

        
       
    }

    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* Window, Camera& camera)
{
    if (glfwGetKey(Window, GLFW_KEY_P) == GLFW_PRESS)
    {
        // Postavite parametre kamere za perspektivnu projekciju
        camera.SetPosition(glm::vec3(5.0f, 5.0f, 5.0f));
        camera.SetYaw(210.0f);
        camera.SetPitch(-45.0f);

        // Postavite projekciju kamere na perspektivnu
        camera.SetPerspectiveProjection(45.0f, (float)WindowWidth / WindowHeight, 0.1f, 100.0f);
    }
    if (glfwGetKey(Window, GLFW_KEY_O) == GLFW_PRESS)
    {
        // Postavite parametre kamere za ortografsku projekciju
        camera.SetPosition(glm::vec3(5.0f, 5.0f, 5.0f));
        camera.SetYaw(215.0f);
        camera.SetPitch(-40.0f);

        // Postavite projekciju kamere na ortografsku
        camera.SetOrthographicProjection(orthoLeft, orthoRight, orthoBottom, orthoTop, orthoNear, orthoFar);// Postavite vrednosti prema potrebama
    }
}


void useShader(Shader2 shader2, glm::mat4 projection, glm::mat4 view)
{
    shader2.use();

    // vertex shader
    shader2.setMat4("projection", projection);
    shader2.setMat4("view", view);

    // material properties
    shader2.setInt("material.diffuse", 0);
    shader2.setInt("material.specular", 1);
    shader2.setFloat("material.shininess", 32.0f);

    // camera position
    shader2.setVec3("viewPos", cameraPos);

}



static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
      //  stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}