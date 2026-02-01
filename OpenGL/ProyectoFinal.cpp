#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <string> 
#include <windows.h>
#include <cstdlib>
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Forzar uso de GPU dedicada
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadCubemap(std::vector<std::string> faces);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Configuraciones Globales
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float GROUND_HEIGHT = 0.0f;
const float EYE_HEIGHT = -0.75f;

// Estado del Jugador
bool flashlightOn = true;
bool leftMousePressed = false;
bool isMoving = false;
int itemsCollected = 0;

// Audio
Mix_Chunk* flashlightSound = nullptr;
Mix_Chunk* footstepSound = nullptr;
Mix_Music* ambientMusic = nullptr;
Mix_Chunk* screamerSound = nullptr;
Mix_Chunk* rainSound = nullptr;
int rainSoundChannel = -1;
float stepTimer = 0.0f;
float stepInterval = 0.60f;
int ambientBaseVolume = 40;
int ambientCurrentVolume = 40;

// Screamer
bool screamerTriggered = false;
float gameTime = 0.0f;
Model* screamerModel = nullptr;
float screamerTimer = 0.0f;
const float SCREAMER_DURATION = 2.0f;
glm::vec3 screamerOffset = glm::vec3(-0.02f, -0.5f, 0.0f);
float screamerDistance = 0.5f;

// --- Evento del Ángel de la Muerte ---
Model* angelModel = nullptr;
glm::vec3 angelPos = glm::vec3(14.8837f, -0.5f, 4.18598f);
glm::vec3 angelTriggerPos = glm::vec3(11.6398f, -0.75f, 3.91526f);
bool angelEventActive = false;
bool angelGone = false;
float angelTimer = 0.0f;

// --- SISTEMA DE LÁMPARAS ---
struct Lamp {
    glm::vec3 pos;
    float rotY;
};

std::vector<Lamp> lamps = {
    {{-3.65768f, -0.75f,  5.56654f},  90.0f},
    {{ 3.72922f, -0.75f,  1.19345f}, 180.0f},
    {{ 3.72544f, -0.75f, -8.0755f}, 180.0f},
    {{ 9.46805f, -0.75f,  5.56654f}, 90.0f},
    {{15.65700f, -0.75f,  2.95896f}, -90.0f},
    {{23.27900f, -0.75f,  5.26618f}, 90.0f},
    {{22.22610f, -0.75f, -6.51624f}, 0.0f},
    {{22.11870f, -0.75f,-12.51980f}, 0.0f},
    {{12.17610f, -0.75f,-18.7778f},   0.0f},
    {{ 5.15026f, -0.75f,-15.49330f},   -90.0f},
    {{ 0.86502f, -0.75f,-19.59110f},   -90.0f},
    {{-9.39945f, -0.75f,-34.2173f}, -90.0f},
    {{-13.2559f, -0.75f,-25.0242f}, 0.0f},
    {{-15.6580f, -0.75f,-21.8439f}, -90.0f},
    {{-6.11057f, -0.75f,-21.8438f},   -90.0f}
};

// Sistema de Lluvia
struct RainDrop {
    glm::vec3 position;
    float speed;
    float lifetime;
};
std::vector<RainDrop> rainDrops;
const int MAX_RAIN_DROPS = 2000;
bool rainEnabled = true;
Shader* rainShader = nullptr;
unsigned int rainVAO, rainVBO;

// Cámara
Camera camera(glm::vec3(-8.0f, GROUND_HEIGHT + EYE_HEIGHT, -0.21f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Tiempos
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- VARIABLES DE GAMEPLAy ---
glm::vec3 item1Pos = glm::vec3(2.05819f, -2.0f, -6.94706f);
glm::vec3 item2Pos = glm::vec3(22.569f, -2.0f, -17.1857f);
glm::vec3 item3Pos = glm::vec3(-6.8024f, -2.0f, -34.2286f); 
glm::vec3 item4Pos = glm::vec3(-42.11f, -2.0f, -17.943f); 

bool haveItem1 = false;
bool haveItem2 = false;
bool haveItem3 = false;
bool haveItem4 = false;

// ---------------------------------------------------------
// ZONAS CAMINABLES
// ---------------------------------------------------------
struct WalkableZone {
    float minX, maxX;
    float minZ, maxZ;
};

std::vector<WalkableZone> walkableAreas = {
    { -9.35477f, -6.70353f,   -0.61720f,  5.57943f },
    { -7.54984f,  9.46954f,    3.15600f,  5.56662f },
    {  0.24099f,  3.73608f,   -8.47747f,  7.68283f },
    { -3.82140f,  6.36511f,   -4.18086f, -1.86407f },
    {  9.55354f, 10.63090f,    3.81593f,  4.65058f },
    {  9.74751f, 11.29860f,   3.72231f,  4.65789f },
    { 10.73870f, 24.44420f,   2.95155f,  5.26790f },
    { 22.21220f, 24.41170f, -10.20810f,  4.93245f },
    { 23.21270f, 24.08760f, -12.09040f, -10.29110f },
    { 22.08550f, 24.42590f, -14.78630f, -11.51420f },
    { 17.99060f, 24.11030f, -14.73550f, -13.63640f },
    { 12.14940f, 12.87390f, -19.56570f, -13.60410f },
    { 19.85310f, 20.54520f, -17.40200f, -14.30370f },
    { 19.45220f, 22.57080f, -17.73980f, -16.75950f },
    { 22.19620f, 24.06940f, -18.09680f, -17.38580f },
    {  0.38228f,  1.37348f, -19.62230f, -13.26360f },
    {  0.38228f,  0.51773f, -15.13060f, -13.26360f },
    { 8.74876f, 13.0992f,  3.73347f, 4.61959f },
    { 23.1403f, 24.1653f,  -13.5508f, -9.37504f },
    { 19.7804f, 20.6700f,  -17.4783f, -13.9402f },
    { 17.9698f, 18.7442f,  -19.9036f, -13.5784f },
    { 12.2209f, 18.7448f, -20.0113f, -17.4991f },
    { 0.39260f, 13.0042f,  -15.5047f, -13.6736f },
    { 0.47255f, 1.43194f,  -19.5189f, -13.4574f },
    { -2.32320f, 1.13305f, -19.0887f, -18.3950f },
    { -2.50748f, -0.36141f, -33.93380f, -14.44820f },
    { -13.14400f, -0.39184f, -34.21760f, -32.60140f },
    { -7.51194f, -5.51990f, -34.23120f, -32.78120f },
    { -13.25680f, -11.77440f, -32.60460f, -20.10840f },
    { -17.86610f, -0.88548f, -21.66190f, -20.28800f },
    { -4.65820f, -0.69792f, -24.12450f, -20.01970f },
    { -17.20890f, -16.27030f, -21.57570f, -15.72030f },
    { -17.8923f, -15.4457f, -18.3792f, -11.1435f },
    { -17.7483f, -12.6737f, -13.3903f, -11.1105f },
    { -22.9581f, -15.7195f, -12.8089f, -11.0721f },
    { -28.2802f, -22.0809f, -13.4403f, -11.1745f },
    { -32.4487f, -27.5281f, -13.4308f, -11.5554f },
    { -35.3055f, -30.9076f, -13.3474f, -10.9517f },
    { -36.0052f, -33.9271f, -19.0435f, -13.0609f },
    { -42.6378f, -33.8999f, -19.2734f, -16.8495f }
};

bool isWalkable(glm::vec3 nextPos) {
    for (const auto& zone : walkableAreas) {
        if (nextPos.x >= zone.minX && nextPos.x <= zone.maxX &&
            nextPos.z >= zone.minZ && nextPos.z <= zone.maxZ) {
            return true;
        }
    }
    return false;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ProyectoFinal", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader sceneShader("shaders/scene.vs", "shaders/scene.fs");
    Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.fs");
    rainShader = new Shader("shaders/rain.vs", "shaders/rain.fs");

    stbi_set_flip_vertically_on_load(false);

    // Cargar Modelos
    Model environment("model/Pasillo/Pasillos.gltf");
    angelModel = new Model("model/angelMuerte/angelMuerte.obj");

    // Modelos para items y lámparas
    Model itemModel("model/farola/farola.obj");
    Model lampModel("model/lampara1/lampara1.obj");

    screamerModel = new Model("model/bebeTerror/bebeTerror.obj");

    // Inicializar Lluvia
    srand(time(NULL));
    for (int i = 0; i < MAX_RAIN_DROPS; i++) {
        RainDrop drop;
        drop.position = glm::vec3((rand() % 40) - 20.0f, (rand() % 30) + 5.0f, (rand() % 40) - 20.0f);
        drop.speed = 8.0f + (rand() % 100) / 100.0f * 4.0f;
        drop.lifetime = 1.0f;
        rainDrops.push_back(drop);
    }
    glGenVertexArrays(1, &rainVAO);
    glGenBuffers(1, &rainVBO);

    // Skybox Setup
    float skyboxVertices[] = {
        -10.0f,  10.0f, -10.0f, -10.0f, -10.0f, -10.0f,  10.0f, -10.0f, -10.0f,
         10.0f, -10.0f, -10.0f,  10.0f,  10.0f, -10.0f, -10.0f,  10.0f, -10.0f,
        -10.0f, -10.0f,  10.0f, -10.0f, -10.0f, -10.0f, -10.0f,  10.0f, -10.0f,
        -10.0f,  10.0f, -10.0f, -10.0f,  10.0f,  10.0f, -10.0f, -10.0f,  10.0f,
         10.0f, -10.0f, -10.0f,  10.0f, -10.0f,  10.0f,  10.0f,  10.0f,  10.0f,
         10.0f,  10.0f,  10.0f,  10.0f,  10.0f, -10.0f,  10.0f, -10.0f, -10.0f,
        -10.0f, -10.0f,  10.0f, -10.0f,  10.0f,  10.0f,  10.0f,  10.0f,  10.0f,
         10.0f,  10.0f,  10.0f,  10.0f, -10.0f,  10.0f, -10.0f, -10.0f,  10.0f,
        -10.0f,  10.0f, -10.0f,  10.0f,  10.0f, -10.0f,  10.0f,  10.0f,  10.0f,
         10.0f,  10.0f,  10.0f, -10.0f,  10.0f,  10.0f, -10.0f,  10.0f, -10.0f,
        -10.0f, -10.0f, -10.0f, -10.0f, -10.0f,  10.0f,  10.0f, -10.0f, -10.0f,
         10.0f, -10.0f, -10.0f, -10.0f, -10.0f,  10.0f,  10.0f, -10.0f,  10.0f
    };
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    std::vector<std::string> faces{
        "textures/skybox/right.png", "textures/skybox/left.png",
        "textures/skybox/top.png",   "textures/skybox/bottom.png",
        "textures/skybox/front.png", "textures/skybox/back.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    glm::vec3 fogColorVector = glm::vec3(0.05f, 0.05f, 0.05f);

    SDL_Init(SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    flashlightSound = Mix_LoadWAV("audio/flashlight_click.wav");
    footstepSound = Mix_LoadWAV("audio/footstep.wav");
    screamerSound = Mix_LoadWAV("audio/scream.wav");
    rainSound = Mix_LoadWAV("audio/rain.wav");
    ambientMusic = Mix_LoadMUS("audio/ambient.wav");

    if (ambientMusic) { Mix_VolumeMusic(ambientBaseVolume); Mix_PlayMusic(ambientMusic, -1); }
    if (rainSound) { rainSoundChannel = Mix_PlayChannel(-1, rainSound, -1); Mix_Volume(rainSoundChannel, 30); }

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        gameTime += deltaTime;

        // Calcular Flicker (Parpadeo) de las lámparas
        float flicker = 0.6f + 0.4f * sin(glfwGetTime() * 5.0f) + 0.2f * sin(glfwGetTime() * 14.0f);

        // --- LÓGICA DEL ÁNGEL ---
        if (!angelGone && !angelEventActive) {
            float dist = glm::distance(camera.Position, angelTriggerPos);
            if (dist < 5.0f) {
                angelEventActive = true;
                angelTimer = 0.0f;
            }
        }
        if (angelEventActive) {
            angelTimer += deltaTime;
            if (angelTimer < 1.2f) {
                int flicker = (int)(angelTimer * 20.0f);
                flashlightOn = (flicker % 2 == 0);
            }
            else if (angelTimer < 2.2f) {
                flashlightOn = false;
            }
            else {
                flashlightOn = true;
                angelGone = true;
                angelEventActive = false;
            }
        }

        // --- LÓGICA DE RECOLECCIÓN (4 ITEMS) ---
        if (!haveItem1 && glm::distance(camera.Position, item1Pos) < 1.5f) {
            haveItem1 = true; itemsCollected++;
            if (flashlightSound) Mix_PlayChannel(-1, flashlightSound, 0);
        }
        if (!haveItem2 && glm::distance(camera.Position, item2Pos) < 1.5f) {
            haveItem2 = true; itemsCollected++;
            if (flashlightSound) Mix_PlayChannel(-1, flashlightSound, 0);
        }
        if (!haveItem3 && glm::distance(camera.Position, item3Pos) < 1.5f) {
            haveItem3 = true; itemsCollected++;
            if (flashlightSound) Mix_PlayChannel(-1, flashlightSound, 0);
        }
        if (!haveItem4 && glm::distance(camera.Position, item4Pos) < 1.5f) {
            haveItem4 = true; itemsCollected++;
            if (flashlightSound) Mix_PlayChannel(-1, flashlightSound, 0);
            if (!screamerTriggered) {
                screamerTriggered = true; screamerTimer = 0.0f;
                if (screamerSound) Mix_PlayChannel(-1, screamerSound, 0);
                Mix_VolumeMusic(10);
            }
        }

        if (screamerTriggered && screamerTimer < SCREAMER_DURATION) {
            screamerTimer += deltaTime;
            if (screamerTimer >= SCREAMER_DURATION) Mix_VolumeMusic(ambientBaseVolume);
        }

        if (isMoving && footstepSound) {
            stepTimer += deltaTime;
            if (stepTimer >= stepInterval) { Mix_PlayChannel(-1, footstepSound, 0); stepTimer = 0.0f; }
        }
        else { stepTimer = stepInterval; }

        int targetVolume = ambientBaseVolume;
        if (isMoving) targetVolume -= 10;
        if (!flashlightOn) targetVolume += 10;
        ambientCurrentVolume += (targetVolume - ambientCurrentVolume) * 0.05f;
        Mix_VolumeMusic(ambientCurrentVolume);

        // --- RENDER ---
        glClearColor(fogColorVector.x, fogColorVector.y, fogColorVector.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.use();

        // --- ENVIAR LUCES DE LÁMPARAS AL SHADER (NUEVO) ---
        sceneShader.setInt("numPointLights", lamps.size());
        for (int i = 0; i < lamps.size(); i++)
        {
            std::string idx = "pointLights[" + std::to_string(i) + "]";
            sceneShader.setVec3(idx + ".position", lamps[i].pos + glm::vec3(0.0f, 0.4f, 0.0f));

            // Luz tenue con flicker
            sceneShader.setVec3(idx + ".ambient", glm::vec3(0.04f * flicker));
            sceneShader.setVec3(idx + ".diffuse", glm::vec3(0.55f * flicker, 0.45f * flicker, 0.32f * flicker));
            sceneShader.setVec3(idx + ".specular", glm::vec3(0.28f * flicker));

            sceneShader.setFloat(idx + ".constant", 1.0f);
            sceneShader.setFloat(idx + ".linear", 0.09f);
            sceneShader.setFloat(idx + ".quadratic", 0.032f);
        }

        // Niebla
        if (itemsCollected == 0) fogColorVector = glm::vec3(0.05f, 0.05f, 0.05f);
        else if (itemsCollected == 1) fogColorVector = glm::vec3(0.03f, 0.03f, 0.04f);
        else if (itemsCollected == 2) fogColorVector = glm::vec3(0.01f, 0.01f, 0.02f);
        else if (itemsCollected >= 3) fogColorVector = glm::vec3(0.0f, 0.0f, 0.0f);
        sceneShader.setVec3("fogColor", fogColorVector);

        sceneShader.setVec3("viewPos", camera.Position);
        sceneShader.setVec3("spotLight.position", camera.Position);
        sceneShader.setVec3("spotLight.direction", camera.Front);

        if (flashlightOn) {
            sceneShader.setVec3("spotLight.ambient", 0.9f, 0.9f, 0.9f);
            sceneShader.setVec3("spotLight.diffuse", 0.4f, 0.4f, 0.4f);
            sceneShader.setVec3("spotLight.specular", 0.9f, 0.9f, 0.9f);
        }
        else {
            sceneShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            sceneShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
            sceneShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
        }

        sceneShader.setFloat("spotLight.constant", 1.0f);
        sceneShader.setFloat("spotLight.linear", (itemsCollected > 0) ? 0.14f : 0.022f);
        sceneShader.setFloat("spotLight.quadratic", (itemsCollected > 0) ? 0.07f : 0.01f);
        sceneShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.0f)));
        sceneShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.0f)));

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        sceneShader.setMat4("projection", projection);
        sceneShader.setMat4("view", view);

        // Dibujar Entorno
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, GROUND_HEIGHT, 0.0f));
        sceneShader.setMat4("model", model);
        environment.Draw(sceneShader);

        // --- DIBUJAR LÁMPARAS ---
        for (const Lamp& lamp : lamps)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, lamp.pos + glm::vec3(0.0f, 0.75f, 0.0f));
            model = glm::rotate(model, glm::radians(lamp.rotY), glm::vec3(0, 1, 0));
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.25f)); // Offset
            model = glm::scale(model, glm::vec3(0.4f));
            sceneShader.setMat4("model", model);
            lampModel.Draw(sceneShader);
        }

        // --- DIBUJAR ITEMS ---
        if (!haveItem1) {
            model = glm::mat4(1.0f); model = glm::translate(model, item1Pos); model = glm::scale(model, glm::vec3(1.0f));
            sceneShader.setMat4("model", model); itemModel.Draw(sceneShader);
        }
        if (!haveItem2) {
            model = glm::mat4(1.0f); model = glm::translate(model, item2Pos); model = glm::scale(model, glm::vec3(1.0f));
            sceneShader.setMat4("model", model); itemModel.Draw(sceneShader);
        }
        if (!haveItem3) {
            model = glm::mat4(1.0f); model = glm::translate(model, item3Pos); model = glm::scale(model, glm::vec3(1.0f));
            sceneShader.setMat4("model", model); itemModel.Draw(sceneShader);
        }
        if (!haveItem4) {
            model = glm::mat4(1.0f); model = glm::translate(model, item4Pos); model = glm::scale(model, glm::vec3(1.0f));
            sceneShader.setMat4("model", model); itemModel.Draw(sceneShader);
        }

        // Ángel
        if (!angelGone) {
            if (flashlightOn || (angelEventActive && angelTimer < 1.2f)) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, angelPos);
                model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(3.0f));
                sceneShader.setMat4("model", model);
                if (angelModel) angelModel->Draw(sceneShader);
            }
        }

        // Screamer
        if (screamerTriggered && screamerTimer < SCREAMER_DURATION && screamerModel) {
            model = glm::mat4(1.0f);
            glm::vec3 screamerPos = camera.Position + (camera.Front * screamerDistance);
            glm::vec3 cameraRight = glm::normalize(glm::cross(camera.Front, camera.Up));
            glm::vec3 cameraUp = camera.Up;
            screamerPos += cameraRight * screamerOffset.x;
            screamerPos += cameraUp * screamerOffset.y;
            screamerPos += camera.Front * screamerOffset.z;
            model = glm::translate(model, screamerPos);
            glm::vec3 direction = glm::normalize(camera.Position - screamerPos);
            float angle = atan2(direction.x, direction.z);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.5f));
            sceneShader.setMat4("model", model);
            screamerModel->Draw(sceneShader);
        }

        // Lluvia y Skybox 
        if (rainEnabled && rainShader) {
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            rainShader->use();
            rainShader->setMat4("projection", projection); rainShader->setMat4("view", view);
            rainShader->setVec3("spotLightPos", camera.Position); rainShader->setVec3("spotLightDir", camera.Front);
            rainShader->setBool("flashlightOn", flashlightOn);
            std::vector<float> rainVertices;
            for (auto& drop : rainDrops) {
                drop.position.y -= drop.speed * deltaTime;
                if (drop.position.y < GROUND_HEIGHT) { drop.position.x = camera.Position.x + (rand() % 40) - 20.0f; drop.position.y = camera.Position.y + 15.0f + (rand() % 15); drop.position.z = camera.Position.z + (rand() % 40) - 20.0f; }
                glm::vec3 toCam = camera.Position - drop.position; toCam.y = 0;
                if (glm::length(toCam) > 25.0f) { drop.position = camera.Position + glm::vec3((rand() % 40) - 20, 15, (rand() % 40) - 20); }
                rainVertices.push_back(drop.position.x); rainVertices.push_back(drop.position.y); rainVertices.push_back(drop.position.z);
                rainVertices.push_back(drop.position.x); rainVertices.push_back(drop.position.y - 0.3f); rainVertices.push_back(drop.position.z);
            }
            glBindVertexArray(rainVAO); glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
            glBufferData(GL_ARRAY_BUFFER, rainVertices.size() * sizeof(float), rainVertices.data(), GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glLineWidth(1.5f); glDrawArrays(GL_LINES, 0, rainDrops.size() * 2); glBindVertexArray(0); glDisable(GL_BLEND);
        }

        glDepthFunc(GL_LEQUAL); skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view); skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36); glBindVertexArray(0); glDepthFunc(GL_LESS);

        glfwSwapBuffers(window); glfwPollEvents();
    }

    glfwTerminate();
    if (angelModel) delete angelModel;
    if (screamerModel) delete screamerModel;
    if (rainShader) delete rainShader;
    Mix_CloseAudio(); SDL_Quit();
    return 0;
}

// Callbacks y Input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    glm::vec3 currentPos = camera.Position; glm::vec3 nextPos = currentPos;
    glm::vec3 forward = camera.Front; forward.y = 0.0f; forward = glm::normalize(forward);
    glm::vec3 right = glm::normalize(glm::cross(forward, camera.Up));
    float velocity = camera.MovementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPos += forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPos -= forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPos -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPos += right * velocity;
    nextPos.y = GROUND_HEIGHT + EYE_HEIGHT;
    if (isWalkable(nextPos)) { camera.Position = nextPos; isMoving = glm::distance(nextPos, currentPos) > 0.0001f; }
    else { isMoving = false; }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) std::cout << "Pos: " << nextPos.x << ", " << nextPos.y << ", " << nextPos.z << std::endl;
    static bool rPress = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPress) { rainEnabled = !rainEnabled; rPress = true; if (rainEnabled) { if (rainSoundChannel == -1) rainSoundChannel = Mix_PlayChannel(-1, rainSound, -1); } else { Mix_HaltChannel(rainSoundChannel); rainSoundChannel = -1; } }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) rPress = false;
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }
void mouse_callback(GLFWwindow* window, double xpos, double ypos) { if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; } float xoffset = xpos - lastX; float yoffset = lastY - ypos; lastX = xpos; lastY = ypos; camera.ProcessMouseMovement(xoffset, yoffset); }
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { camera.ProcessMouseScroll(yoffset); }
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) { if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { if (!angelEventActive) { flashlightOn = !flashlightOn; if (flashlightSound) Mix_PlayChannel(-1, flashlightSound, 0); } } }
unsigned int loadCubemap(std::vector<std::string> faces) { unsigned int textureID; glGenTextures(1, &textureID); glBindTexture(GL_TEXTURE_CUBE_MAP, textureID); int width, height, nrChannels; for (unsigned int i = 0; i < faces.size(); i++) { unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0); if (data) { glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); stbi_image_free(data); } else { std::cout << "Cubemap failed: " << faces[i] << std::endl; stbi_image_free(data); } } glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); return textureID; }