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

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

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
    Model environment("model/Pueblo/Pueblo.obj");


    // b) Los Objetos a recoger
    Model item1("model/farola/farola.obj");
    Model item2("model/farola/farola.obj");
    // Agrega más modelos aquí...

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

    // Game Loop

    while (!glfwWindowShouldClose(window))
    {
        // Delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Limpiar pantalla (Fondo negro)
        glClearColor(fogColorVector.x, fogColorVector.y, fogColorVector.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ============================================
        // FASE 1: DIBUJAR ESCENA (CON LINTERNA)
        // ============================================
        sceneShader.use();

        sceneShader.setVec3("fogColor", fogColorVector);

        // Configuración de Luces (LINTERNA / SPOTLIGHT)
        sceneShader.setVec3("viewPos", camera.Position);

        // Propiedades de la linterna (Spotlight)
        sceneShader.setVec3("spotLight.position", camera.Position);
        sceneShader.setVec3("spotLight.direction", camera.Front);
        if (flashlightOn)
        {
            sceneShader.setVec3("spotLight.ambient", 0.05f, 0.05f, 0.05f); // Ambiente bajo (Terror)
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
    Mix_FreeChunk(flashlightSound);
    Mix_CloseAudio();
    SDL_Quit();

    return 0;
}

// Inputs y Callbacks (Igual que tenías)
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glm::vec3 position = camera.Position;

    // Dirección hacia adelante SIN componente Y
    glm::vec3 forward = camera.Front;
    forward.y = 0.0f;
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, camera.Up));

    float velocity = camera.MovementSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= forward * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += right * velocity;

    // Altura fija tipo persona
    position.y = GROUND_HEIGHT + EYE_HEIGHT;

    camera.Position = position;
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

        std::cout << "Flashlight: " << (flashlightOn ? "ON\n" : "OFF\n");
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