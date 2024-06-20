/*
work06b

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2023/2024
Master degree in Computer Science
Universita' degli Studi di Milano
*/


// Std. Includes
#include <string>

// Loader estensioni OpenGL
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, for FPS camera, and for physical simulation
#include <utils/shader.h>
#include <utils/camera.h>
#include <utils/physics.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>

#include "util3d/model.h"

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <vector>
#include <stb_image/stb_image.h>

GLuint screenWidth = 1200, screenHeight = 900;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

// we initialize an array of booleans for each keyboard key
bool keys[1024];

// we set the initial position of mouse cursor in the application window
GLfloat lastX = 400.0f, lastY = 300.0f;

// we will use these value to "pass" the cursor position to the keyboard callback, in order to determine the bullet trajectory
double cursorX, cursorY;

// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

// view and projection matrices (global because we need to use them in the keyboard callback)
glm::mat4 view, projection;

// we create a camera. We pass the initial position as a parameter to the constructor. In this case, we use a "floating" camera (we pass false as last parameter)
Camera camera(glm::vec3(0.0f, 0.0f, 9.0f), GL_FALSE);

// Uniforms to be passed to shaders
// point light position
glm::vec3 lightPos0 = glm::vec3(5.0f, 10.0f, 10.0f);

// weight for the diffusive component
GLfloat Kd = 3.0f;
// roughness index for GGX shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// color of the falling objects
GLfloat diffuseColor[] = {1.0f, 0.0f, 0.0f};
// color of the plane
GLfloat planeMaterial[] = {0.0f, 0.5f, 0.0f};
// color of the bullets
GLfloat shootColor[] = {1.0f, 1.0f, 0.0f};

const struct PauseShaderSettings {
    float vertJerkOpt;
    float vertMovementOpt;
    float bottomStaticOpt ;
    float scalinesOpt;
    float rgbOffsetOpt;
    float horzFuzzOpt ;
    float desaturate;
} inGame{0.1, 0.0, 0.1, 0.2, 0.1, 0.3, 0},
paused{1, 1, 1, 1, 1, 1, 0.5};

bool isPaused = false;

void renderQuad();

btRigidBody *playerBody;

Physics bulletSimulation;

struct SphereData {
    btRigidBody *body;
};

std::vector<SphereData> spheres;

struct SplatData {
    glm::vec3 pos;
    glm::mat4 rot;
};

std::vector<SplatData> splats;

bool mousepressed = false;

class MyContactResultCallback : public btCollisionWorld::ContactResultCallback {
public:
    bool collided;
    btVector3 position;
    btVector3 normal;

    MyContactResultCallback() : collided(false) {}

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
                                     const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override {
        collided = true;
        // store collision point, and normal angle of obj 1
        normal = cp.m_normalWorldOnB;

        // position is coll position offset a little by in the direction of the normal
        position = cp.m_positionWorldOnB + cp.m_normalWorldOnB * 0.1;

        return 0; // Not used
    }
};

bool bloom = true;
bool showBlurBuffer = false;
float exposure = 1.0f;
////////////////// MAIN function ///////////////////////
int main() {
    // Initialization of OpenGL context using GLFW
    glfwInit();
    // We set OpenGL specifications required for this application
    // In this case: 4.1 Core
    // If not supported by your graphics HW, the context will not be created and the application will close
    // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
    // in relation also to the values indicated in these GLFW commands
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // we create the application's window
    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "RTGP Project - Press ESC to Pause", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
    // we disable the mouse cursor
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }


    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // the Shader Program for the objects used in the application
    Shader object_shader = Shader("shader.vert", "shader.frag");
    Shader blur_shader = Shader("blur.vert", "blur.frag");
    Shader bloom_shader = Shader("basic.vert", "bloom.frag");
    Shader tex_shader = Shader("basic.vert", "tex.frag");
    Shader pause_shader = Shader("basic.vert", "pause.frag");

    GLuint crosshair = TextureFromFile("crosshair.png", "textures");
    GLuint pauseTex = TextureFromFile("pause.png", "textures");

    Model backrooms("backrooms_map/backrooms.obj");

    Model sphere_model("models/sphere.obj");

    Model splat_model("models/newscene.obj");
    //Model backrooms("backrooms_map3/untitled.obj");
    //Model backrooms("backrooms_map2/Sketchfab_2022_04_30_13_07_42.obj");
    //Model backrooms("test_obj/capsule.obj");

    auto *envMesh = new btTriangleMesh();
    for (auto &value: backrooms.meshes) {
        for (int i = 0; i < value.vertices.size() / 3; i++) {
            auto &v1 = value.vertices[i * 3];
            auto &v2 = value.vertices[i * 3 + 1];
            auto &v3 = value.vertices[i * 3 + 2];
            envMesh->addTriangle(
                btVector3(v1.Position.x, v1.Position.y, v1.Position.z),
                btVector3(v2.Position.x, v2.Position.y, v2.Position.z),
                btVector3(v3.Position.x, v3.Position.y, v3.Position.z)
            );
        }
    }
    auto triMeshShape = new btBvhTriangleMeshShape(envMesh, true);
    auto backroomBody = new btRigidBody(0, new btDefaultMotionState(), triMeshShape);
    backroomBody->setFriction(0.9);
    bulletSimulation.dynamicsWorld->addRigidBody(backroomBody);

    // player is a cube
    playerBody = bulletSimulation.createRigidBody(
        BOX, glm::vec3(11, 0.2, 11), glm::vec3(0.2f, 0.9f, 0.2f), glm::vec3(0.0f, 0.0f, 0.0f), 1.0f, 0.9f, 0.0f);
    playerBody->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));

    //btCollisionShape* shape = new btBvhTriangleMeshShape(backrooms.meshes[0].m_meshes[0].m_mesh->m_btMeshInterface, true);

    glm::vec3 plane_pos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 plane_size = glm::vec3(1.0, 1.0, 1.0);
    projection = glm::perspective(45.0f, (float) screenWidth / (float) screenHeight, 0.1f, 10000.0f);

    glm::mat4 planeModelMatrix = glm::mat4(1.0f);
    glm::mat3 planeNormalMatrix = glm::mat3(1.0f);

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
        );
    }

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    unsigned int pingpongFBO[3];
    unsigned int pingpongColorbuffers[3];
    glGenFramebuffers(3, pingpongFBO);
    glGenTextures(3, pingpongColorbuffers);
    for (unsigned int i = 0; i < 3; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    struct AmbientLight {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    } ambient;

    struct Light {
        glm::vec3 position;
        float constant;
        float linear;
        float quadratic;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    } lights[25];

    glm::vec3 pos(-8.6, 1.25, 7.93); {
        int i = 0;
        for (int y = 0; y < 4; y++) {
            pos.x = -8.6;
            int x = 0;
            if (y == 2) {
                x--;
                pos.x -= 3.42;
            }
            for (; x < 6; x++) {
                lights[i].position = pos;
                lights[i].constant = 1;
                lights[i].linear = 0.09;
                lights[i].quadratic = 0.032;
                lights[i].ambient = glm::vec3(-0.1);
                lights[i].diffuse = glm::vec3(0);
                lights[i].specular = glm::vec3(0);
                pos += glm::vec3(3.42, 0, 0);
                i++;
            }
            pos += glm::vec3(0, 0, y == 2 ? -5.67 : -4.54);
        }
    }

    AmbientLight &light = ambient;

    light.ambient = glm::vec3(0.35f);
    light.diffuse = glm::vec3(0.25f);
    light.specular = glm::vec3(1.0f);

    // Define random generator with Gaussian distribution
    const double mean = 0.0;
    const double stddev = 0.1;
    std::default_random_engine generator;
    std::normal_distribution<double> dist(mean, stddev);

    std::uniform_int_distribution<int> lightdist(0, 24);

    // camera.MovementSpeed = 5.0;
    //camera.onGround = true;
    //camera.Position = glm::vec3(11, 0.2, 11);

    camera.ProcessMouseMovement(-176, -3);

    int warmingUp = 0;
    int warmingUpIdx = 0;
    float warmingUpDuration = 0.2;

    float lightFlicker = 0;
    float lightFlickerDuration = 0;
    float lightFlickerBase;
    float lightPointFlickerBase;
    float ceilingFlickerBase;
    int flickerLight = -1;
    float flickerLightDuration = 0;

    int debugLightId = 0;
    bool debouncelight = false;

    float lastbullet = 0;

    GLfloat maxSecPerFrame = 1.0f / 60.0f;
    // Rendering loop: this code is executed at each frame
    while (!glfwWindowShouldClose(window)) {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        lightFlicker += deltaTime;

        // Check is an I/O event is happening
        glfwPollEvents();

        if (keys[GLFW_KEY_O]) {
            if (!debouncelight) {
                debugLightId = (debugLightId + 1) % 25;
                std::cout << "light: " << debugLightId << std::endl;
                debouncelight = true;
            }
        } else {
            debouncelight = false;
        }



        if (!isPaused) {
            // we apply FPS camera movements
            apply_camera_movements();


            if (mousepressed) {
                if (currentFrame - lastbullet > 0.1) {
                    glm::vec3 bulletPos = camera.Position + camera.Front * 0.5f;
                    auto body = bulletSimulation.createRigidBody(SPHERE, bulletPos, glm::vec3(0.13f), camera.Front, 1.0f, 0.9f, 0.0f);
                    // give bullet front speed
                    body->setLinearVelocity(body->getLinearVelocity() + btVector3(camera.Front.x, camera.Front.y, camera.Front.z) * 20);
                    spheres.push_back({body});
                    lastbullet = currentFrame;
                }
            }


            bulletSimulation.dynamicsWorld->stepSimulation(min(deltaTime, maxSecPerFrame), 10);

            std::vector<SphereData> newSpheres;
            for (auto& sphere : spheres) {
                MyContactResultCallback callback;
                bulletSimulation.dynamicsWorld->contactPairTest(sphere.body, backroomBody, callback);
                if (callback.collided) {
                    auto normal = glm::vec3(callback.normal.x(), callback.normal.y(), callback.normal.z());
                    normal = glm::normalize(normal);

                    // dot normal with bullet speed so we can check if the normal is flipped
                    auto speed = sphere.body->getLinearVelocity();
                    auto speedVec = glm::vec3(speed.x(), speed.y(), speed.z());
                    if (glm::dot(normal, speedVec) > 0) {
                        normal = -normal;
                    }

                    auto initial = glm::vec3(0, 0, 1);
                    auto angle = glm::acos(glm::dot(initial, normal));

                    auto axis = glm::normalize(glm::cross(initial, normal));

                    glm::mat4 rot;
                    if (angle > 0.001 && axis.x == axis.x) {
                        rot = glm::rotate(glm::mat4(1.0f), angle, axis);
                    } else {
                        rot = glm::mat4(1.0f);
                    }
                    auto pos = glm::vec3(callback.position.x(), callback.position.y(), callback.position.z()) +
                        normal * -0.04f;
                    splats.push_back(SplatData{
                        pos,
                        rot});
                    bulletSimulation.dynamicsWorld->removeRigidBody(sphere.body);
                    delete sphere.body;
                } else {
                    newSpheres.push_back(sphere);
                }
            }

            spheres = newSpheres;
        }

        auto ppos = playerBody->getCenterOfMassPosition();
        camera.Position = glm::vec3(ppos.x(), ppos.y() + 0.3, ppos.z());

        // View matrix (=camera): position, view direction, camera "up" vector
        // in this example, it has been defined as a global variable (we need it in the keyboard callback function)
        view = camera.GetViewMatrix();

        // render
        {
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

            // we "clear" the frame and z buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            // we set the rendering mode
            if (wireframe)
                // Draw in wireframe
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            /////////////////// OBJECTS ////////////////////////////////////////////////
            // We "install" the selected Shader Program as part of the current rendering process
            object_shader.Use();

            // vEyePos
            glUniform3f(glGetUniformLocation(object_shader.Program, "vEyePos"), camera.Position.x, camera.Position.y,
                        camera.Position.z);

            float ceilingFlicker = 1.0;
            if (warmingUp < 4) {
                warmingUpDuration += deltaTime;
                if (warmingUpDuration > 1.5) {
                    int i;
                    for (i = 0; i < (warmingUp == 2 ? 7 : 6); i++) {
                        lights[warmingUpIdx + i].ambient = glm::vec3(0.05f);
                        lights[warmingUpIdx + i].diffuse = glm::vec3(0.8f);
                        lights[warmingUpIdx + i].specular = glm::vec3(1.0f);
                    }
                    warmingUpIdx += i;
                    warmingUp++;
                    warmingUpDuration = 0;
                }
            } else {
                if (lightFlickerDuration > 0) {
                    float delta = deltaTime;
                    do {
                        float gen = abs(dist(generator));
                        float amb = lightFlickerBase - gen * 0.5f;
                        light.ambient = glm::vec3(amb);
                        for (int i = 0; i < 25; i++) {
                            if (ceilingFlickerBase < 1 && flickerLight == i)
                                continue;
                            lights[i].ambient = glm::vec3(lightPointFlickerBase - abs(dist(generator)) * 1.5f * 0.05f);
                            lights[i].diffuse = glm::vec3(0);
                        }
                        if (ceilingFlickerBase < 1) {
                            ceilingFlicker = ceilingFlickerBase - gen * 8;

                            flickerLightDuration += deltaTime;
                            if (flickerLightDuration > 0.3) {
                                if (dist(generator) > 0) {
                                    flickerLight = lightdist(generator);
                                    lights[flickerLight].ambient = glm::vec3(0.2f);
                                }
                                flickerLightDuration = 0;
                            }
                        }
                        delta -= 0.01;
                    } while (delta >= 0.01);
                    lightFlickerDuration -= deltaTime;
                } else {
                    light.ambient = glm::vec3(0.35f);
                    for (int i = 0; i < 25; i++) {
                        lights[i].ambient = glm::vec3(0.05f);
                        lights[i].diffuse = glm::vec3(0.8f);
                    }
                    ceilingFlickerBase = 1.0;
                    if (lightFlicker > (1 / 60.0f)) {
                        lightFlickerDuration = (dist(generator) - 0.20) * 7;
                        if (lightFlickerDuration > 0) {
                            flickerLight = lightdist(generator);
                            if (dist(generator) > 0.1) {
                                lightFlickerBase = 0.02f;
                                lightFlickerDuration += 0.5;
                                lightFlickerDuration *= 1.8;
                                lightPointFlickerBase = 0.005f;
                                ceilingFlickerBase = 0.35f;
                            } else {
                                lightFlickerBase = 0.35f;
                                lightPointFlickerBase = 0.05f;
                                ceilingFlickerBase = 0.7f;
                            }
                        }

                        lightFlicker = 0;
                    }
                }
            }
            glUniform1f(glGetUniformLocation(object_shader.Program, "ceilingFlicker"), ceilingFlicker);


            // calculate vEyeDir from yaw and pitch
            glm::vec3 front;
            front.x = cos(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch));
            front.y = sin(glm::radians(camera.Pitch));
            front.z = sin(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch));
            glm::vec3 vEyeDir = glm::normalize(front);

            if (keys[GLFW_KEY_M]) {
                std::cout << "position:" << camera.Position.x << " " << camera.Position.y << " " << camera.Position.z
                        << " direction:" << camera.Yaw << " " << camera.Pitch << std::endl;
            }

            glUniform3f(glGetUniformLocation(object_shader.Program, "vEyeDir"), vEyeDir.x, vEyeDir.y, vEyeDir.z);

            // we pass projection and view matrices to the Shader Program
            glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "projectionMatrix"), 1, GL_FALSE,
                               glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "viewMatrix"), 1, GL_FALSE,
                               glm::value_ptr(view));


            planeModelMatrix = glm::mat4(1.0f);
            planeNormalMatrix = glm::mat3(1.0f);
            planeModelMatrix = glm::translate(planeModelMatrix, plane_pos);
            planeModelMatrix = glm::scale(planeModelMatrix, plane_size);
            planeNormalMatrix = glm::inverseTranspose(glm::mat3(view * planeModelMatrix));
            glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE,
                               glm::value_ptr(planeModelMatrix));
            glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE,
                               glm::value_ptr(planeNormalMatrix));

            glUniform3fv(glGetUniformLocation(object_shader.Program, "ambient.ambient"), 1, &light.ambient[0]);
            glUniform3fv(glGetUniformLocation(object_shader.Program, "ambient.diffuse"), 1, &light.diffuse[0]);
            glUniform3fv(glGetUniformLocation(object_shader.Program, "ambient.specular"), 1, &light.specular[0]);

            for (int i = 0; i < 25; i++) {
                glUniform3fv(
                    glGetUniformLocation(object_shader.Program, ("lights[" + std::to_string(i) + "].position").c_str()),
                    1, &lights[i].position[0]);
                glUniform1f(glGetUniformLocation(object_shader.Program,
                                                 ("lights[" + std::to_string(i) + "].constant").c_str()),
                            lights[i].constant);
                glUniform1f(glGetUniformLocation(object_shader.Program,
                                                 ("lights[" + std::to_string(i) + "].linear").c_str()),
                            lights[i].linear);
                glUniform1f(glGetUniformLocation(object_shader.Program,
                                                 ("lights[" + std::to_string(i) + "].quadratic").c_str()),
                            lights[i].quadratic);
                glUniform3fv(
                    glGetUniformLocation(object_shader.Program, ("lights[" + std::to_string(i) + "].ambient").c_str()),
                    1, &lights[i].ambient[0]);
                glUniform3fv(
                    glGetUniformLocation(object_shader.Program, ("lights[" + std::to_string(i) + "].diffuse").c_str()),
                    1, &lights[i].diffuse[0]);
                glUniform3fv(
                    glGetUniformLocation(object_shader.Program, ("lights[" + std::to_string(i) + "].specular").c_str()),
                    1, &lights[i].specular[0]);
            }

            glUniform1ui(glGetUniformLocation(object_shader.Program, "debugLightId"), debugLightId);

            glUniform1ui(glGetUniformLocation(object_shader.Program, "backrooms"), 1);


            backrooms.Draw(object_shader);

            glUniform1ui(glGetUniformLocation(object_shader.Program, "backrooms"), 0);

            for (auto& sphere : spheres) {
                auto modelMatrix = glm::mat4(1.0f);
                auto normalMatrix = glm::mat3(1.0f);
                auto pos = sphere.body->getCenterOfMassPosition();
                auto shape = (btSphereShape*)sphere.body->getCollisionShape();
                modelMatrix = glm::translate(modelMatrix, glm::vec3(pos.x(), pos.y(), pos.z()));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.05f));
                normalMatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));
                glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(modelMatrix));
                glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(normalMatrix));
                sphere_model.Draw(object_shader);
            }

            for (auto& splat : splats) {
                auto modelMatrix = glm::mat4(1.0f);
                auto normalMatrix = glm::mat3(1.0f);
                modelMatrix = glm::translate(modelMatrix, splat.pos);
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.002));
                modelMatrix = modelMatrix * splat.rot;
                normalMatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));
                glUniformMatrix4fv(glGetUniformLocation(object_shader.Program, "modelMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(modelMatrix));
                glUniformMatrix3fv(glGetUniformLocation(object_shader.Program, "normalMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(normalMatrix));

                splat_model.Draw(object_shader);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        bool horizontal = true, first_iteration = true;

        // blur bloom
        {
            unsigned int amount = 10;
            blur_shader.Use();
            for (unsigned int i = 0; i < amount; i++) {
                glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
                glUniform1i(glGetUniformLocation(blur_shader.Program, "horizontal"), horizontal);
                glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
                // bind texture of other framebuffer (or scene if first iteration)
                renderQuad();
                horizontal = !horizontal;
                if (first_iteration)
                    first_iteration = false;
            }
        }

        // draw finalized framebuffer
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[2]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            bloom_shader.Use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
            glUniform1i(glGetUniformLocation(bloom_shader.Program, "scene"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
            glUniform1i(glGetUniformLocation(bloom_shader.Program, "bloomBlur"), 1);

            glUniform1i(glGetUniformLocation(bloom_shader.Program, "bloom"), bloom);
            glUniform1f(glGetUniformLocation(bloom_shader.Program, "exposure"), exposure);
            renderQuad();
        }

        // draw crosshair
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongColorbuffers[2]);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            tex_shader.Use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, crosshair);
            glUniform1i(glGetUniformLocation(tex_shader.Program, "tex"), 0);

            renderQuad();
        }

        /*// draw pause background
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongColorbuffers[2]);
            tex_shader.Use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pauseTex);
            glUniform1i(glGetUniformLocation(tex_shader.Program, "tex"), 0);

            renderQuad();
        }*/

        // draw pause
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_BLEND);
            pause_shader.Use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[2]);
            glUniform1i(glGetUniformLocation(pause_shader.Program, "iChannel0"), 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pauseTex);
            glUniform1i(glGetUniformLocation(pause_shader.Program, "iChannel1"), 1);
            glUniform1f(glGetUniformLocation(pause_shader.Program, "iTime"), currentFrame);

            auto& settings = isPaused ? paused : inGame;

            glUniform1f(glGetUniformLocation(pause_shader.Program, "vertJerkOpt"), settings.vertJerkOpt);
            glUniform1f(glGetUniformLocation(pause_shader.Program, "vertMovementOpt"), settings.vertMovementOpt);
            glUniform1f(glGetUniformLocation(pause_shader.Program, "bottomStaticOpt"), settings.bottomStaticOpt);
            glUniform1f(glGetUniformLocation(pause_shader.Program, "scalinesOpt"), settings.scalinesOpt);
            glUniform1f(glGetUniformLocation(pause_shader.Program, "rgbOffsetOpt"), settings.rgbOffsetOpt);
            glUniform1f(glGetUniformLocation(pause_shader.Program, "horzFuzzOpt"), settings.horzFuzzOpt);
            glUniform1f(glGetUniformLocation(pause_shader.Program, "desaturate"), settings.desaturate);

            renderQuad();
        }

        // Faccio lo swap tra back e front buffer
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    object_shader.Delete();

    // we close and delete the created context
    glfwTerminate();
    return 0;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;

void renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements() {

    const float MOVE_SPEED = 4;

    glm::vec3 movement{};
    if (keys[GLFW_KEY_W])
        movement += glm::vec3(0, 0, -MOVE_SPEED);
    else if (keys[GLFW_KEY_S])
        movement += glm::vec3(0, 0, MOVE_SPEED);
    if (keys[GLFW_KEY_A])
        movement += glm::vec3(-MOVE_SPEED, 0, 0);
    else if (keys[GLFW_KEY_D])
        movement += glm::vec3(MOVE_SPEED, 0, 0);
    if (movement == glm::vec3{})
        return;
    // apply movement rotated by camera yaw
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(-camera.Yaw - 90), glm::vec3(0, 1, 0));
    movement = glm::vec3(rot * glm::vec4(movement, 1));

    playerBody->activate();
    playerBody->setLinearVelocity(
        playerBody->getLinearVelocity() * btVector3{0, 1, 0} + btVector3{movement.x, 0, movement.z});
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;


    // if ESC is pressed, we close the application
    // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    //     glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (!isPaused) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPos(window, lastX, lastY);
        }
        isPaused = !isPaused;
    }

    if (isPaused)
        return;

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe = !wireframe;

    if (key == GLFW_KEY_B && action == GLFW_PRESS)
        bloom = !bloom;

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        showBlurBuffer = !showBlurBuffer;

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        playerBody->activate();

        playerBody->setLinearVelocity(playerBody->getLinearVelocity() + btVector3{0, 3, 0});
    }

}

//////////////////////////////////////////
// callback for mouse events
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    // we move the camera view following the mouse cursor
    // we calculate the offset of the mouse cursor from the position in the last frame
    // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)


    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }


    // we save the current cursor position in 2 global variables, in order to use the values in the keyboard callback function
    cursorX = xpos;
    cursorY = ypos;


    // offset of mouse cursor position
    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    // the new position will be the previous one for the next frame
    lastX = xpos;
    lastY = ypos;
    if (isPaused)
        return;

    // we pass the offset to the Camera class instance in order to update the rendering
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (isPaused)
        return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mousepressed = true;


        //splats.push_back(SplatData{bulletPos, glm::radians(rand() % 360 + 1.0f)});
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mousepressed = false;
    }
}