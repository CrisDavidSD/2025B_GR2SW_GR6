#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>
#include <cstdlib>   // rand
#include <ctime>     // time
#include <cmath>



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ===================== ANIMACIÓN PERSONAJE =====================
enum AnimState {
    IDLE,
    WALK,
    PICKUP
};

AnimState currentAnim = IDLE;

glm::vec3 characterPos(0.0f, 0.0f, 0.0f);
float characterRot = 0.0f;

float walkTime = 0.0f;
float idleTime = 0.0f;
float pickupTime = 0.0f;
bool picking = false;


struct NPC {
    glm::vec3 basePos;
    glm::vec3 pos;
    float yawDeg;

    // comportamiento
    float patrolT;       // tiempo interno
    float appearT;       // tiempo interno para aparecer/desaparecer
    bool visible;

    // item
    bool hasPart;        // si aún tiene el resto
};

NPC npc;

// para evitar que se active muchas veces con E (debounce)
bool eWasDown = false;

// contador de partes recolectadas (tu objetivo)
int partsCollected = 0;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 16 Task 3", NULL, NULL);
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

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("shaders/shader_exercise16_mloading.vs", "shaders/shader_exercise16_mloading.fs");

    // load models
    // -----------
    //Model ourModel(FileSystem::getPath("resources/objects/backpack/backpack.obj"));
    Model ourModel("C:/Users/NETWORKS/Documents/Visual Studio 2022/Open_GL/Open_GL/model/huesos/huesos.obj");
    //Model ourModel("model/backpack/backpack.obj");

    std::srand((unsigned)time(nullptr));

    npc.basePos = glm::vec3(0.0f, 0.0f, 0/.0f);  // donde “vive”
    npc.pos = npc.basePos;
    npc.yawDeg = 0.0f;

    npc.patrolT = 0.0f;
    npc.appearT = 0.0f;
    npc.visible = true;

    npc.hasPart = true;  // al inicio sí tiene el resto




    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    camera.MovementSpeed = 10; //Optional. Modify the speed of the camera

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // ===================== NPC UPDATE =====================
        float t = (float)glfwGetTime();

        // 1) Aparece/desaparece cada cierto tiempo
        // visible durante 6s, oculto 4s (ciclo 10s)
        float cycle = fmod(t, 10.0f);
        npc.visible = (cycle < 6.0f);

        // 2) Si ya entregó la parte, desaparece para siempre
        if (!npc.hasPart) {
            npc.visible = false;
        }

        // 3) Patrulla suave (solo cuando está visible)
        if (npc.visible) {
            npc.patrolT += deltaTime;

            // movimiento en círculo pequeño alrededor de basePos
            float r = 0.8f;           // radio patrulla
            float speed = 0.8f;       // velocidad
            float a = npc.patrolT * speed;

            npc.pos = npc.basePos + glm::vec3(cos(a) * r, 0.0f, sin(a) * r);

            // mira en dirección de movimiento (yaw)
            npc.yawDeg = glm::degrees(a) + 90.0f;
        }

        // render
        // ------
        glClearColor(0.09f, 0.09f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);



        // ===================== ANIMACIONES =====================

// --- IDLE: respiración MUY sutil
        idleTime += deltaTime;
        float breathe = sin(idleTime * 1.5f) * 0.02f;  // mucho más leve

        // --- CAMINAR: bobbing vertical pesado (sin balanceo lateral)
        if (currentAnim == WALK) {
            walkTime += deltaTime * 4.0f; // más lento = más peso
        }
        float walkBob = (currentAnim == WALK)
            ? fabs(sin(walkTime)) * 0.035f   // solo sube y baja
            : 0.0f;

        // --- RECOGER OBJETO: inclinación controlada
        if (picking) {
            pickupTime += deltaTime;
            if (pickupTime > 1.0f) {
                picking = false;
                currentAnim = IDLE;
            }
        }
        float pickAngle = picking ? sin(pickupTime * 3.14f) * 20.0f : 0.0f;


        if (npc.visible) {
            glm::mat4 model = glm::mat4(1.0f);

            // posición NPC
            model = glm::translate(model, npc.pos);

            // rotación (para que "mire" su dirección)
            model = glm::rotate(model, glm::radians(npc.yawDeg), glm::vec3(0, 1, 0));

            // escala
            model = glm::scale(model, glm::vec3(1.0f));

            ourShader.setMat4("model", model);
            ourModel.Draw(ourShader);
        }


        // ===================== NPC INTERACTION (E) =====================
        bool eDown = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);

        // detectar "tap" (cuando sueltas y vuelves a presionar)
        bool eTapped = (eDown && !eWasDown);
        eWasDown = eDown;

        // distancia entre "jugador" y NPC (jugador = cámara)
        float distToNpc = glm::length(camera.Position - npc.pos);

        // si está visible, tiene parte y estás cerca -> recoger
        if (npc.visible && npc.hasPart && eTapped && distToNpc < 2.0f) {
            npc.hasPart = false;
            partsCollected += 1;

            std::cout << "✅ Parte recolectada! Total: " << partsCollected << std::endl;
        }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool moving = false;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        characterPos.z -= deltaTime * 1.5f;
        currentAnim = WALK;
        moving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        characterPos.z += deltaTime * 1.5f;
        currentAnim = WALK;
        moving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        characterRot += deltaTime * 60.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        characterRot -= deltaTime * 60.0f;
    }

    if (!moving && !picking) {
        currentAnim = IDLE;
    }

    // Recoger objeto (E)
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !picking) {
        picking = true;
        pickupTime = 0.0f;
        currentAnim = PICKUP;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}