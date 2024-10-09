/*
 Author: Dylan Blake
 Assignment: Simple 2D Scene
 Date due: 2023-9-20, 11:59pm
 I pledge that I have completed this assignment without collaborating
 with anyone else, in conformance with the NYU School of Engineering
 Policies and Procedures on Academic Misconduct
 */

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

using namespace glm;

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.1922f,
                BG_BLUE    = 0.549f,
                BG_GREEN   = 0.9059f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

constexpr char SPRITE1[] = "/Users/dylanblake/Developer/IntroGameProg2/SDLProject/SDLProject/Minotaur1.png",
               SPRITE2[] = "/Users/dylanblake/Developer/IntroGameProg2/SDLProject/SDLProject/Minotaur2.png";

constexpr vec3 INIT_SPRITE1_POS = {0.0f, 0.0f, 0.0f};

constexpr float G_SPRITE1_MOVEMENT_SPEED = .5f,
                G_SPRITE2_MOVEMENT_SPEED = 1.0f,
                CIRCLE2_RADIUS = 2.0f,
                CIRCLE1_RADIUS = .5f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

mat4 g_view_matrix,
     g_sprite1_matrix,
     g_sprite2_matrix,
     g_projection_matrix;

vec3 g_sprite1_position,
     g_sprite2_position;
float g_sprite1_rotation = 0.0f,
      g_sprite1_rotation_speed = 35.0f;
float g_sprite2_angle = 0.0f,
      g_sprite1_angle = 0.0f;

float scale_speed = .001f;
float sprite_scale = 1.0f;
bool growing = true;

GLuint g_sprite1_texture_id, g_sprite2_texture_id;

float g_previous_ticks = 0.0f;


// Initialise our program
void initialise();
// Process any player input - pressed button or moved joystick
void process_input();
// Update game state given player input and previous state
void update();
// Once updated, render those changes onto the screen
void render();
// Shutdown protocol once game is over
void shutdown();

void draw_object(mat4 &object_model_matrix, GLuint &object_texture_id);

GLuint load_texture(const char* filepath);

int main(int argc, char* argv[]) {
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Simple 2D Scene",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr) {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = mat4(1.0f);
    g_projection_matrix = ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_sprite1_matrix = mat4(1.0f);
    g_sprite2_matrix = mat4(1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    g_sprite1_matrix = translate(g_sprite1_matrix, INIT_SPRITE1_POS);
    g_sprite2_matrix = translate(g_sprite1_matrix, vec3(CIRCLE2_RADIUS, 0.0f, 0.0f));
    
    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    g_sprite1_texture_id = load_texture(SPRITE1);
    g_sprite2_texture_id = load_texture(SPRITE2);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT or event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update() {
    /* Delta time calculations */
    float g_ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float g_delta_time = g_ticks - g_previous_ticks;
    g_previous_ticks = g_ticks;
    
    g_sprite1_matrix = mat4(1.0f);
    g_sprite2_matrix = mat4(1.0f);
    
    /* Game logic */
    g_sprite2_angle += G_SPRITE2_MOVEMENT_SPEED * g_delta_time;

    if (g_sprite2_angle >= 2 * M_PI) g_sprite2_angle -= 2 * M_PI;

    float x2 = CIRCLE2_RADIUS * cos(g_sprite2_angle);
    float y2 = CIRCLE2_RADIUS * sin(g_sprite2_angle);

    g_sprite2_position = vec3(x2, y2, 0.0f);
    g_sprite2_matrix = translate(g_sprite1_matrix, g_sprite2_position);
    // break
    g_sprite1_angle += G_SPRITE1_MOVEMENT_SPEED * g_delta_time;

    if (g_sprite1_angle >= 2 * M_PI) g_sprite1_angle -= 2 * M_PI;

    float x1 = CIRCLE1_RADIUS * cos(g_sprite1_angle);
    float y1 = CIRCLE1_RADIUS * sin(g_sprite1_angle);

    g_sprite1_position = vec3(x1, y1, 0.0f);
    g_sprite1_matrix = translate(g_sprite1_matrix, g_sprite1_position);
    //break
    if (g_sprite1_rotation > 20 || g_sprite1_rotation < -20) {
        g_sprite1_rotation = g_sprite1_rotation > 20 ? 20 : -20;
        g_sprite1_rotation_speed *= -1;
    }
    g_sprite1_rotation += g_sprite1_rotation_speed * g_delta_time;
    g_sprite1_matrix = rotate(g_sprite1_matrix, radians(g_sprite1_rotation),
                              vec3(0.0f, 0.0f, 1.0f));
    
    if (sprite_scale > 1.2) {
        growing = false;
        sprite_scale = 3;
    }
    else if (sprite_scale < .8) {
        growing = true;
        sprite_scale = 1;
    }
    if (growing)
        sprite_scale += scale_speed * g_delta_time;
    else
        sprite_scale -= scale_speed * g_delta_time;
    glm::vec3 scale_vector = glm::vec3(sprite_scale, sprite_scale, 1.0f);
    g_sprite1_matrix = scale(g_sprite1_matrix, scale_vector);
    
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    g_shader_program.set_model_matrix(g_sprite1_matrix);
    g_shader_program.set_model_matrix(g_sprite2_matrix);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(),
                          2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(),
                          2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    // Bind texture
    draw_object(g_sprite1_matrix, g_sprite1_texture_id);
    draw_object(g_sprite2_matrix, g_sprite2_texture_id);
    
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void draw_object(mat4 &object_model_matrix, GLuint &object_texture_id) {
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void shutdown() { SDL_Quit(); }

GLuint load_texture(const char* filepath) {
    int width, height, number_of_components;
    // "load" dynamically allocates memory
    unsigned char* image = stbi_load(filepath, &width, &height,
                                     &number_of_components, STBI_rgb_alpha);

    if (not image) {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;                               // declaration
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);  // assignment
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexImage2D(
        GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA,
        width, height,
        TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE,
        image
    );
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);

    return textureID;
}
