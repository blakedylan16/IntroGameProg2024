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

enum Coordinate { x_coordinate, y_coordinate };

constexpr int WINDOW_WIDTH  = 640,
              WINDOW_HEIGHT = 480;

const Coordinate X_COORDINATE = x_coordinate,
                 Y_COORDINATE = y_coordinate;

const float ORTHO_WIDTH = 10.0f,
            ORTHO_HEIGHT = 7.5f;

constexpr float BG_RED     = 0.0f,
                BG_BLUE    = 0.0f,
                BG_GREEN   = 0.0f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

//constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
//               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";
constexpr char V_SHADER_PATH[] = "shaders/vertex.glsl",
               F_SHADER_PATH[] = "shaders/fragment.glsl";

constexpr int NUMBER_OF_TEXTURES = 0;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

constexpr vec3  INIT_PADDLE1_POS    = {4.0f, 0.0f, 0.0f},
                INIT_PADDLE2_POS    = {-4.0f, 0.0f, 0.0f},
                PADDLE_SCALE        = {.15f, 1.0f, 1.0f},
                INIT_BALL_POS       = {0, 0, 0},
                BALL_SCALE          = {.15f, .15f, 1.0f};

constexpr float G_PADDLE_MOVEMENT_SPEED = 3.3f,
                G_BALL_MOVEMENT_SPEED   = 5.0f;

SDL_Window* g_display_window;
SDL_Joystick *g_controller[2] = {NULL};
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

mat4    g_view_matrix,
        g_paddle1_matrix,
        g_paddle2_matrix,
        g_ball_matrix,
        g_projection_matrix;

vec3    g_paddle1_position  = INIT_PADDLE1_POS,
        g_paddle2_position  = INIT_PADDLE2_POS,
        g_ball_position     = INIT_BALL_POS,
        g_paddle1_movement,
        g_paddle2_movement,
        g_ball_movement;

float g_previous_ticks = 0.0f;

bool SINGLE_PLAYER = true;

struct Paddle {
    mat4 model_matrix;
    vec3 position;
    vec3 movement;
    
    Paddle(mat4& matrix, vec3& pos, vec3& mov) :
    model_matrix(matrix), position(pos), movement(mov) { }
};

Paddle Paddle1(g_paddle1_matrix, g_paddle1_position, g_paddle1_movement);
Paddle Paddle2(g_paddle2_matrix, g_paddle2_position, g_paddle2_movement);

Paddle* Paddles[] = {&Paddle1, &Paddle2};

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
void draw_object(mat4 &object_model_matrix);
GLuint load_texture(const char* filepath);
float get_screen_to_ortho(float coordinate, Coordinate axis);
void change_player_mode();
void hit(Paddle &paddle);

int main(int argc, char* argv[]) {
    initialise();

    while (g_app_status == RUNNING) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    g_controller[0] = SDL_JoystickOpen(0);
    g_display_window = SDL_CreateWindow("Pong",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);

    if (g_display_window == nullptr) {
        std::cerr << "Error: SDL window could not be created.\n";
        g_app_status = TERMINATED;
        SDL_Quit();
        exit(1);
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = mat4(1.0f);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    g_projection_matrix = ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    g_shader_program.set_projection_matrix(g_projection_matrix);
    
    for (Paddle *paddle : Paddles) {
        paddle->model_matrix = mat4(1.0f);
        paddle->model_matrix = translate(paddle->model_matrix, paddle->position);
        paddle->model_matrix = scale(paddle->model_matrix, PADDLE_SCALE);
    }
        
    g_ball_matrix = mat4(1.0f);
    g_ball_movement = vec3(0.0f, 0.0f, 0.0f);
    g_ball_matrix = translate(g_ball_matrix, INIT_BALL_POS);
    g_ball_matrix = scale(g_ball_matrix, BALL_SCALE);
    
    g_shader_program.set_colour(.9f, .9f, .9f, 1.0f);
    
    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // enable blending
//    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    for (Paddle *paddle : Paddles)
        paddle->movement.y = 0;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
        }
    }
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    if (key_state[SDL_SCANCODE_UP])
        Paddle1.movement.y = 1;
    else if (key_state[SDL_SCANCODE_DOWN])
        Paddle1.movement.y = -1;
    
    if (key_state[SDL_SCANCODE_T])
        change_player_mode();
    
    if(key_state[SDL_SCANCODE_SPACE] and g_ball_movement.x == 0)
        g_ball_movement.x = 1;
    
    if (not SINGLE_PLAYER) {
        if (key_state[SDL_SCANCODE_W])
            Paddle2.movement.y = 1;
        else if (key_state[SDL_SCANCODE_S])
            Paddle2.movement.y = -1;
    } else {
        if (Paddle2.position.y > g_ball_position.y) Paddle2.movement.y = -1;
        else Paddle2.movement.y = 1;
    }
}


void update() {
    /* Delta time calculations */
    float g_ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = g_ticks - g_previous_ticks;
    g_previous_ticks = g_ticks;
    
    /* Game logic */
    for (Paddle *paddle : Paddles) {
        paddle->model_matrix = mat4(1.0f);
        
        vec3 temp_paddle_pos = paddle->position;
        temp_paddle_pos += paddle->movement * G_PADDLE_MOVEMENT_SPEED * delta_time;
        
        if (ORTHO_HEIGHT/2 - fabs(temp_paddle_pos.y) - PADDLE_SCALE.y / 2 > 0)
            paddle->position = temp_paddle_pos;
        
        paddle->model_matrix = translate(paddle->model_matrix, paddle->position);
        paddle->model_matrix = scale(paddle->model_matrix, PADDLE_SCALE);
    }
    
    g_ball_matrix = mat4(1.0f);
    vec3 temp_ball_pos = g_ball_position;
    temp_ball_pos += g_ball_movement * delta_time * G_BALL_MOVEMENT_SPEED;
    
    // check upper and lower bounds
    if (ORTHO_HEIGHT/2 - fabs(temp_ball_pos.y) - BALL_SCALE.y / 2 > 0)
        g_ball_position.y = temp_ball_pos.y;
    else g_ball_movement.y *= -1;
    
    // check for hit
    vec2    ball_to_paddle1 = vec2(fabs(Paddle1.position.x - temp_ball_pos.x) - (BALL_SCALE.x + PADDLE_SCALE.x) / 2.0f ,
                                     fabs(Paddle1.position.y - temp_ball_pos.y) - (BALL_SCALE.y + PADDLE_SCALE.y) / 2.0f),
            ball_to_paddle2 = vec2(fabs(Paddle2.position.x - temp_ball_pos.x) - (BALL_SCALE.x + PADDLE_SCALE.x) / 2.0f ,
                                     fabs(Paddle2.position.y - temp_ball_pos.y) - (BALL_SCALE.y + PADDLE_SCALE.y) / 2.0f);

    if (((ball_to_paddle1.x < 0 and ball_to_paddle1.y < 0) or
        (ball_to_paddle2.x < 0 and ball_to_paddle2.y < 0)) and
        fabs(temp_ball_pos.x) < INIT_PADDLE1_POS.x) {
        if (g_ball_position.x > 0)
            hit(Paddle1);
        else hit(Paddle2);
        g_ball_position.x = g_ball_position.x < 0 ? Paddle2.position.x + (BALL_SCALE.x + PADDLE_SCALE.x) / 2.0f :
                                                    Paddle1.position.x - (BALL_SCALE.x + PADDLE_SCALE.x) / 2.0f;
    }
    else g_ball_position.x = temp_ball_pos.x;
    
    
    // check for point
    if (ORTHO_WIDTH/2 - fabs(temp_ball_pos.x) - BALL_SCALE.x / 2 < 0) {
        g_ball_position = INIT_BALL_POS;
        g_ball_movement = vec3(0.0f, 0.0f, 0.0f);
    }
    
    g_ball_matrix = translate(g_ball_matrix, g_ball_position);
    g_ball_matrix = scale(g_ball_matrix, BALL_SCALE);
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(),
                          2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    for (Paddle *paddle : Paddles)
        draw_object(paddle->model_matrix);
    
    draw_object(g_ball_matrix);
    

//    float texture_coordinates[] = {
//        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
//        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
//    };
//    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(),
//                          2, GL_FLOAT, false, 0, texture_coordinates);
//    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
//    
//    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
//    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void draw_object(mat4 &object_model_matrix, GLuint &object_texture_id) {
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw_object(mat4 &object_model_matrix) {
    g_shader_program.set_model_matrix(object_model_matrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}



void shutdown() {
    for(size_t i = 0; i < SDL_NumJoysticks(); i++)
        SDL_JoystickClose(g_controller[i]);
    SDL_Quit();
}

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

float get_screen_to_ortho(float coordinate, Coordinate axis) {
    switch(axis) {
        case x_coordinate:
            return ((coordinate / WINDOW_WIDTH) * ORTHO_WIDTH) - (ORTHO_WIDTH / 2.0);
        case y_coordinate:
            return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * ORTHO_HEIGHT) - (ORTHO_HEIGHT / 2.0);
        default: return 0.0f;
    }
}

void change_player_mode() {
    SINGLE_PLAYER = not SINGLE_PLAYER;
    if (not SINGLE_PLAYER)
        g_controller[1] = SDL_JoystickOpen(1);
    else {
        SDL_JoystickClose(g_controller[1]);
        g_controller[1] = NULL;
    }
}

void hit (Paddle &paddle) {
    g_ball_movement.x *= -1;
    float point_of_contact = g_ball_position.y - paddle.position.y;
    float ratio = point_of_contact / (PADDLE_SCALE.y / 2.0f);
    g_ball_movement.y = ratio;
}
