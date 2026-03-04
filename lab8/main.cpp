#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>

// Fereastra int
int glWindowWidth = 1500;
int glWindowHeight = 800;
int retina_width, retina_height;
int renderMode = 0; // 0: Solid, 1: Wireframe, 2: Polygonal 

bool sunLightOn = true; 
float sunLightIntensity = 1.0f;

bool fogEnabled = false;
float fogDensity = 0.0005f; 
glm::vec3 fogColor = glm::vec3(0.5f, 0.7f, 1.0f); 

GLFWwindow* glWindow = NULL;

// Matrice
glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;

// Variabile pentru Transformari Model 
float modelRotation = 0.0f;
float modelScale = 15.0f;
float modelTranslationY = 0.0f;

// Variabile pentru Animatie de Prezentare 
bool presentationMode = true;
float presentationAngle = 0.0f;

// Pozitia si orientarea initiala
const glm::vec3 initialPos = glm::vec3(0.0f, 500.0f, 2000.0f);
const float initialYaw = -90.0f;
const float initialPitch = 0.0f;

// Variabile pentru lumina locala de la portal
bool portalLightOn = false;
GLuint portalLightOnLoc;

glm::vec3 portalLightPosWorld = glm::vec3(350.109f, 299.551f, 25.2743f); // Am pus lumina chiar in mijlocul portalului
float portalLightIntensity = 4.0f; 

float lightAngle = 0.0f;

// Lumina
glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

// Umbrele
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
gps::Shader depthShader;
GLuint lightSpaceTrMatrixLoc;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

// Camera si Control
gps::Camera myCamera(
    glm::vec3(0.0f, 20.0f, 100.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

float cameraSpeed = 30.0f;
bool pressedKeys[1024];
float lastX = 750, lastY = 400;
float yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;

// Modele si Shader
gps::Model3D myModel;
gps::Model3D portalPlasma;
gps::Model3D dragonModel;
gps::Shader myCustomShader;

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        presentationMode = !presentationMode;
        if (!presentationMode) {
            yaw = initialYaw;
            pitch = initialPitch;
            // Resetare totala camera la starea initiala
            myCamera = gps::Camera(initialPos, initialPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            myCamera.rotate(pitch, yaw);
            firstMouse = true;
        }
        else {
            presentationAngle = 0.0f;
        }
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        fogEnabled = !fogEnabled;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        renderMode = (renderMode + 1) % 3;
    }

    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        portalLightOn = !portalLightOn;
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        sunLightOn = !sunLightOn;
    }

    if (key == GLFW_KEY_B && action == GLFW_PRESS) {

        glm::vec3 cameraPos = myCamera.getCameraPosition();
        std::cout << "Pozitie (X, Y, Z): "
            << cameraPos.x << ", "
            << cameraPos.y << ", "
            << cameraPos.z << std::endl;
    }

    // Pentru miscare continua
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) pressedKeys[key] = true;
        else if (action == GLFW_RELEASE) pressedKeys[key] = false;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (presentationMode) return;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    myCamera.rotate(pitch, yaw);
}

void processMovement() {
    if (presentationMode) {
        presentationAngle += 0.01f;
        float radius = 2000.0f;
        float camX = sin(presentationAngle) * radius;
        float camZ = cos(presentationAngle) * radius;
        myCamera.setCameraPosition(glm::vec3(camX, 800.0f, camZ));
        myCamera.setCameraTarget(glm::vec3(0.0f, 400.0f, 0.0f));
    }
    else {
        // Miscare camera
        if (pressedKeys[GLFW_KEY_W]) myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        if (pressedKeys[GLFW_KEY_S]) myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        if (pressedKeys[GLFW_KEY_A]) myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        if (pressedKeys[GLFW_KEY_D]) myCamera.move(gps::MOVE_RIGHT, cameraSpeed);

        // Control model
        if (pressedKeys[GLFW_KEY_J]) modelRotation += 1.0f; // Rotire stanga
        if (pressedKeys[GLFW_KEY_L]) modelRotation -= 1.0f; // Rotire dreapta
        if (pressedKeys[GLFW_KEY_U]) modelScale += 0.01f; // Scalare Up
        if (pressedKeys[GLFW_KEY_I]) modelScale -= 0.01f; // Scalare Down
        if (pressedKeys[GLFW_KEY_K]) modelTranslationY += 0.5f; // Translatie Sus
        if (pressedKeys[GLFW_KEY_N]) modelTranslationY -= 0.5f; // Translatie Jos

        // Control ceata 
        if (pressedKeys[GLFW_KEY_C]) {
            fogDensity += 0.0001f;
            if (fogDensity > 0.1f) fogDensity = 0.1f;
        }
        if (pressedKeys[GLFW_KEY_V]) {
            fogDensity -= 0.0001f;
            if (fogDensity < 0.0f) fogDensity = 0.0f;
        }

        // Control lumina portal
        if (pressedKeys[GLFW_KEY_9]) {
            portalLightIntensity += 0.1f;
        }
        if (pressedKeys[GLFW_KEY_8]) {
            portalLightIntensity -= 0.1f;
            if (portalLightIntensity < 0.0f) portalLightIntensity = 0.0f;
        }

        // Control lumina globala
        if (pressedKeys[GLFW_KEY_Q]) {
            lightAngle += 0.01f;
        }
        if (pressedKeys[GLFW_KEY_E]) {
            lightAngle -= 0.01f;
        }

        if (pressedKeys[GLFW_KEY_7]) {
            sunLightIntensity += 0.01f; 
        }
        if (pressedKeys[GLFW_KEY_6]) {
            sunLightIntensity -= 0.01f;
            if (sunLightIntensity < 0.0f) sunLightIntensity = 0.0f;
        }

        // Recalculez directia luminii pe un cerc (axa X si Z se schimbă, Y ramane constant)
        lightDir = glm::normalize(glm::vec3(sin(lightAngle), 1.0f, cos(lightAngle)));

        myCustomShader.useShaderProgram();
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
    }

    view = myCamera.getViewMatrix();
    myCustomShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

glm::mat4 computeLightSpaceTrMatrix() {

    glm::mat4 lightProjection = glm::ortho(-1300.0f, 1300.0f, -1300.0f, 1300.0f, 0.1f, 2000.0f);
    glm::vec3 lightPos = lightDir * 1400.0f; //Pozitia luminii globale in spatiu
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}

void initSkyBox() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/front.tga");
    mySkyBox.Load(faces); 
}

void initShadowMap() {
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initObjects() {
    myModel.LoadModel("objects/proiect.obj", "objects/");
    portalPlasma.LoadModel("objects/portal.obj", "objects/");
    dragonModel.LoadModel("objects/Untitled.obj", "objects/");
}

void initShaders() {
    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    depthShader.loadShader("shaders/depthShader.vert", "shaders/depthShader.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

void initUniforms() {
    myCustomShader.useShaderProgram();
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    lightSpaceTrMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix");

    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 10000.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
    lightColor = glm::vec3(1.1f, 1.05f, 0.95f);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

    portalLightOnLoc = glGetUniformLocation(myCustomShader.shaderProgram, "portalLightOn");
    glUniform1i(portalLightOnLoc, portalLightOn);

    GLuint intensLoc = glGetUniformLocation(myCustomShader.shaderProgram, "portalLightIntensity");
    glUniform1f(intensLoc, portalLightIntensity);

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "portalLightPos"), 1, glm::value_ptr(portalLightPosWorld));
}

void drawScene(gps::Shader shader, bool isDepthPass) {
    shader.useShaderProgram();

    // Calculez matricea de baza - se aplica tuturor obiectelor din scena
    glm::mat4 baseModel = glm::mat4(1.0f);
    baseModel = glm::translate(baseModel, glm::vec3(0.0f, modelTranslationY, 0.0f));
    baseModel = glm::rotate(baseModel, glm::radians(modelRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    baseModel = glm::scale(baseModel, glm::vec3(modelScale));

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "diffuseTexture"), 0);

    glm::mat4 modelMain = baseModel;
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMain));

    if (!isDepthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * modelMain));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    myModel.Draw(shader);

    if (!isDepthPass) {
        glm::mat4 modelPortal = baseModel;
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelPortal));
        portalPlasma.Draw(shader);
    }

    // Animatie dragon
    float time = (float)glfwGetTime();
    float x = sin(time * 0.2f) * 50.0f;
    float z = cos(time * 0.2f) * 50.0f;

    glm::mat4 modelDragon = baseModel;
    modelDragon = glm::translate(modelDragon, glm::vec3(x, 30.0f, z));
    float angle = glm::degrees(atan2(x, z));
    modelDragon = glm::rotate(modelDragon, glm::radians(angle + 270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelDragon = glm::scale(modelDragon, glm::vec3(0.03f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDragon));

    if (!isDepthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * modelDragon));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    dragonModel.Draw(shader);
}

void renderScene() {

    depthShader.useShaderProgram();

    glm::mat4 lightSpaceMatrix = computeLightSpaceTrMatrix();
    glUniformMatrix4fv(glGetUniformLocation(depthShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Dezactivez CULL_FACE pentru ca frunzele copacilor (plane) sa lase umbra pe ambele parti
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    drawScene(depthShader, true);

    // Activez CULL_FACE la loc pentru randarea scenei principale
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, retina_width, retina_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myCustomShader.useShaderProgram();

    // Trimitem matricea View
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Trimiterea matricei pentru umbre si textura de adancime
    glUniformMatrix4fv(lightSpaceTrMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

    glm::vec3 currentPortalPos = portalLightPosWorld + glm::vec3(0.0f, modelTranslationY, 0.0f);
    glm::vec3 portalLightPosEye = glm::vec3(view * glm::vec4(currentPortalPos, 1.0f));

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "portalLightPos"), 1, glm::value_ptr(portalLightPosEye));
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "portalLightOn"), portalLightOn ? 1 : 0);

    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "sunLightOn"), sunLightOn ? 1 : 0);

    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "portalLightIntensity"), portalLightIntensity);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "sunLightIntensity"), sunLightIntensity);

    // Gestionare ceata
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "showFog"), fogEnabled ? 1 : 0);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));

    // Setare Mod Randare (Solid / Wireframe / Points)
    if (renderMode == 0) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else if (renderMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else if (renderMode == 2) glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    // Desenarea propriu-zisa a scenei
    drawScene(myCustomShader, false);

    mySkyBox.Draw(skyboxShader, view, projection);
}

bool initOpenGLWindow() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "Dragon Flight Shadows", NULL, NULL);
    if (!glWindow) { glfwTerminate(); return false; }
    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(glWindow);
#if not defined (__APPLE__) 
    glewExperimental = GL_TRUE; glewInit();
#endif 
    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
    return true;
}

void initOpenGLState() {
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
    glViewport(0, 0, retina_width, retina_height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

int main(int argc, const char* argv[]) {
    if (!initOpenGLWindow()) return 1;
    initOpenGLState();
    initShadowMap();
    initShaders();
    initObjects();
    initSkyBox();
    initUniforms();
    while (!glfwWindowShouldClose(glWindow)) {
        processMovement();
        renderScene();
        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }
    glfwTerminate();
    return 0;
}