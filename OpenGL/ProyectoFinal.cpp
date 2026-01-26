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
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Forzar uso de GPU dedicada en laptops con GPU 
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

void framebuffer_size_callback(GLFWwindow * window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadCubemap(std::vector<std::string> faces);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Configuraciones
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float GROUND_HEIGHT = 0.0f;
const float EYE_HEIGHT = 0.6f;
bool flashlightOn = true;
bool leftMousePressed = false;
Mix_Chunk* flashlightSound = nullptr;
Mix_Chunk* footstepSound = nullptr;
float stepTimer = 0.0f;
float stepInterval = 0.60f; // tiempo entre pasos (ajustable)
bool isMoving = false;
Mix_Music* ambientMusic = nullptr;
int ambientBaseVolume = 40;   // volumen base (0–128)
int ambientCurrentVolume = 40;

// Screamer
bool screamerTriggered = false;
float gameTime = 0.0f;
Model* screamerModel = nullptr;
Mix_Chunk* screamerSound = nullptr;
float screamerTimer = 0.0f;
const float SCREAMER_DURATION = 2.0f; // Duración del screamer en pantalla
// Configuración de posición del screamer
glm::vec3 screamerOffset = glm::vec3(-0.02f, -0.5f, 0.0f); // Sin offset = centrado
float screamerDistance = 0.5f; // Distancia desde la cámara

// Cámara
Camera camera(glm::vec3(0.0f, GROUND_HEIGHT + EYE_HEIGHT, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Tiempos
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Estado del juego (Progreso)
int itemsCollected = 0;

// Estructura para definir una zona prohibida
struct Wall {
    float minX, maxX;
    float minZ, maxZ;
};

// LISTA DE MUROS
std::vector<Wall> mapWalls = {
    { -2.06932f,  3.94866f,   10.7121f,  13.9746f },
    { -2.65907f, -1.76947f,   -1.40436f, 13.7197f },
    { -16.5053f, -2.57247f,    7.77555f, 13.7702f },
    { -14.0376f, -8.53279f,    4.45856f,  8.40696f },
    { -16.7831f, -13.6299f,  -14.9171f,   8.72878f },
    { -16.3006f, -9.60847f,  -13.5624f,  -7.31482f },
    { -9.95989f,  3.4585f,   -12.8188f,  -9.98209f },
    {  3.1686f,   4.86991f,  -13.4659f,  -3.04957f },
    {  3.1794f,   7.27756f,   -4.1687f,  -3.03074f },
    {  3.06694f, 10.5866f,   -13.3407f, -11.769f  },
    { 10.4088f,  21.3489f,   -14.1892f,  -3.27584f },
    { -15.9309f, -0.476604f,  -4.60152f, -3.75159f },
    { 18.8284f,  21.4914f,    -4.70449f,  0.698352f},
    { 20.7644f,  21.7245f,    -0.993197f, 13.684f  },
    {  3.49084f,  5.58993f,   -1.31225f, 13.79f    },
    {  3.72947f, 22.3513f,     6.2273f,  13.7062f  },
    {  7.48924f, 11.0058f,     3.27616f,  5.12232f },
    { 15.6376f,  18.9299f,     3.46294f,  4.89013f },
    { -13.6106f, -11.0124f, -0.253142f, 4.45014f },
};

// Función para verificar si una posición choca con algún muro
bool checkCollision(glm::vec3 nextPos) {
    for (const auto& wall : mapWalls) {
        if (nextPos.x >= wall.minX && nextPos.x <= wall.maxX &&
            nextPos.z >= wall.minZ && nextPos.z <= wall.maxZ) {
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


    stbi_set_flip_vertically_on_load(false); // Flip para modelos .obj

    // a) El Entorno (Suelo/Paredes)
    //Model environment("C:/Texturas/model/Town/Untitled.obj");
    Model environment("model/Pueblo/Pueblo10.obj");


    // b) Los Objetos a recoger
    Model item1("model/farola/farola.obj");
    Model item2("model/farola/farola.obj");
    // Agrega más modelos aquí...

    // Cargar modelo del screamer
    screamerModel = new Model("model/bebeTerror/bebeTerror.obj");

    stbi_set_flip_vertically_on_load(false); // Desactivar flip para el skybox

    // 4. Configuración del Skybox (Cubo)
    // ----------------------------------
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
        "textures/skybox/right.png",
        "textures/skybox/left.png",
        "textures/skybox/top.png",
        "textures/skybox/bottom.png",
        "textures/skybox/front.png",
        "textures/skybox/back.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    glm::vec3 fogColorVector = glm::vec3(0.05f, 0.05f, 0.05f);

    //Prueba audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        std::cout << "Error SDL_AUDIO\n";
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        std::cout << "Error SDL_mixer\n";
    }

    // Cargar sonido de linterna
    flashlightSound = Mix_LoadWAV("audio/flashlight_click.wav");
    if (!flashlightSound)
    {
        std::cout << "Error cargando sonido de linterna\n";
    }

    // Cargar sonido de pasos
    footstepSound = Mix_LoadWAV("audio/footstep.wav");
    if (!footstepSound)
    {
        std::cout << "Error cargando sonido de pasos\n";
    }

    // Cargar música de ambiente
    ambientMusic = Mix_LoadMUS("audio/ambient.wav");
    if (!ambientMusic)
    {
        std::cout << "Error cargando sonido ambiente\n";
    }
    else
    {
        Mix_VolumeMusic(ambientBaseVolume);
        Mix_PlayMusic(ambientMusic, -1); // loop infinito
    }

    // Cargar sonido del screamer
    screamerSound = Mix_LoadWAV("audio/scream.wav");
    if (!screamerSound)
    {
        std::cout << "Error cargando sonido de screamer\n";
    }

    // Game Loop

    while (!glfwWindowShouldClose(window))
    {
        // Delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Actualizar tiempo de juego
        gameTime += deltaTime;

        // Trigger del screamer a los 7 segundos
        if (!screamerTriggered && gameTime >= 90.0f)
        {
            screamerTriggered = true;
            screamerTimer = 0.0f;

            // Reproducir sonido de grito
            if (screamerSound)
            {
                Mix_PlayChannel(-1, screamerSound, 0);
            }

            // Opcional: bajar música ambiente durante el screamer
            Mix_VolumeMusic(10);
        }

        // Actualizar timer del screamer activo
        if (screamerTriggered && screamerTimer < SCREAMER_DURATION)
        {
            screamerTimer += deltaTime;

            // Restaurar volumen cuando termine
            if (screamerTimer >= SCREAMER_DURATION)
            {
                Mix_VolumeMusic(ambientBaseVolume);
            }
        }

        // SONIDO DE PASOS
        if (isMoving && footstepSound)
        {
            stepTimer += deltaTime;

            if (stepTimer >= stepInterval)
            {
                Mix_PlayChannel(-1, footstepSound, 0);
                stepTimer = 0.0f;
            }
        }
        else
        {
            stepTimer = stepInterval; // evita retraso al volver a moverse
        }

        // SONIDO AMBIENTE DINÁMICO
        int targetVolume = ambientBaseVolume;

        // Si se mueve → baja el ambiente (los pasos dominan)
        if (isMoving)
        {
            targetVolume -= 10;
        }

        // Si la linterna está apagada → más tensión
        if (!flashlightOn)
        {
            targetVolume += 10;
        }

        // Clamp seguro
        if (targetVolume < 10) targetVolume = 10;
        if (targetVolume > 70) targetVolume = 70;

        // Suavizado (muy importante)
        ambientCurrentVolume += (targetVolume - ambientCurrentVolume) * 0.05f;
        Mix_VolumeMusic(ambientCurrentVolume);

        // LIMPIAR PANTALLA (Fondo negro)
        glClearColor(fogColorVector.x, fogColorVector.y, fogColorVector.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // FASE 1: DIBUJAR ESCENA (CON LINTERNA)
        sceneShader.use();

        sceneShader.setVec3("fogColor", fogColorVector);

        // Configuración de Luces (LINTERNA / SPOTLIGHT)
        sceneShader.setVec3("viewPos", camera.Position);

        // Propiedades de la linterna (Spotlight)
        sceneShader.setVec3("spotLight.position", camera.Position);
        sceneShader.setVec3("spotLight.direction", camera.Front);
        if (flashlightOn)
        {
            sceneShader.setVec3("spotLight.ambient", 0.4f, 0.4f, 0.4f); // Ambiente bajo (Terror)
            sceneShader.setVec3("spotLight.diffuse", 0.9f, 0.9f, 0.8f); // Luz blanca
            sceneShader.setVec3("spotLight.specular", 0.2f, 0.2f, 0.2f);
        }
        else
        {
            // Linterna apagada = luz 0
            sceneShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            sceneShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
            sceneShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
        }

        sceneShader.setFloat("spotLight.constant", 1.0f);
        sceneShader.setFloat("spotLight.linear", 0.09f);
        sceneShader.setFloat("spotLight.quadratic", 0.032f);
        sceneShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(10.0f)));
        sceneShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        // Matrices de transformación
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        sceneShader.setMat4("projection", projection);
        sceneShader.setMat4("view", view);

        // 1. Dibujar Entorno (Suelo)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // Bajar un poco el suelo
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        sceneShader.setMat4("model", model);
        environment.Draw(sceneShader);

        // 2. Dibujar Item 1 (Siempre visible)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(5.0f, 0.0f, -5.0f)); // Posición Item 1
        model = glm::scale(model, glm::vec3(0.005f));
        sceneShader.setMat4("model", model);
        item1.Draw(sceneShader);

        // 3. Dibujar Item 2 (Solo si recogiste el anterior... lógica pendiente)
        // Por ahora dibujamos todos para probar
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-5.0f, 0.0f, -8.0f)); // Posición Item 2
        model = glm::scale(model, glm::vec3(0.1f));
        sceneShader.setMat4("model", model);
        item2.Draw(sceneShader);

        // Dibujar screamer si está activo
        if (screamerTriggered && screamerTimer < SCREAMER_DURATION && screamerModel)
        {
            model = glm::mat4(1.0f);

            // 1. Calcular posición base frente a la cámara
            glm::vec3 screamerPos = camera.Position + (camera.Front * screamerDistance);

            // 2. Aplicar offset en el espacio de la cámara
            glm::vec3 cameraRight = glm::normalize(glm::cross(camera.Front, camera.Up));
            glm::vec3 cameraUp = camera.Up;

            screamerPos += cameraRight * screamerOffset.x;  // Izquierda/Derecha
            screamerPos += cameraUp * screamerOffset.y;     // Arriba/Abajo
            screamerPos += camera.Front * screamerOffset.z; // Adelante/Atrás (adicional)

            model = glm::translate(model, screamerPos);

            // Hacer que mire hacia la cámara
            glm::vec3 direction = glm::normalize(camera.Position - screamerPos);
            float angle = atan2(direction.x, direction.z);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

            // Escala (ajusta según tu modelo)
            model = glm::scale(model, glm::vec3(0.5f));

            sceneShader.setMat4("model", model);
            screamerModel->Draw(sceneShader);
        }

        // FASE 2: DIBUJAR SKYBOX 

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // Quitar traslación
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    //Limpieza SDL_mixer
    if (screamerModel) delete screamerModel;
    Mix_FreeChunk(screamerSound);
    Mix_FreeChunk(flashlightSound);
    Mix_FreeChunk(footstepSound);
    Mix_FreeMusic(ambientMusic);
    Mix_CloseAudio();
    SDL_Quit();

    return 0;
}

// Inputs y Callbacks
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 1. Guardamos la posición actual por si acaso
    glm::vec3 currentPos = camera.Position;

    // 2. Calculamos la "Posición Futura" (nextPos)
    glm::vec3 nextPos = currentPos;

    // Dirección hacia adelante SIN componente Y (para no volar)
    glm::vec3 forward = camera.Front;
    forward.y = 0.0f;
    forward = glm::normalize(forward);
    glm::vec3 right = glm::normalize(glm::cross(forward, camera.Up));

    float velocity = camera.MovementSpeed * deltaTime;

    // Calculamos el movimiento deseado
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        nextPos += forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        nextPos -= forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        nextPos -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        nextPos += right * velocity;

    // Mantenemos la altura fija
    nextPos.y = GROUND_HEIGHT + EYE_HEIGHT;

    // Si la "siguiente posición" NO choca con nada, actualizamos la cámara.
    if (!checkCollision(nextPos)) {
        camera.Position = nextPos;
        isMoving = glm::distance(nextPos, currentPos) > 0.0001f;
    }
    else {
        // Si choca, intentamos deslizar (Opcional básico: simplemente no se mueve)
        isMoving = false;
    }

    // Si presionas la tecla P, imprime tu posición en la consola
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        std::cout << "Pos: " << nextPos.x << ", " << nextPos.z << std::endl;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        flashlightOn = !flashlightOn;

        // Sonido de linterna
        if (flashlightSound)
        {
            Mix_PlayChannel(-1, flashlightSound, 0);
        }
    }
}

// Función auxiliar para cargar Cubemap
unsigned int loadCubemap(std::vector<std::string> faces)
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