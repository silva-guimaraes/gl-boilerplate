#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h> 
#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define X 0
#define Y 1
#define Z 2
#define W 3

#define SCROLL_SPEED 4.0
#define KEY_AMOUNT 245
#define MODE_COUNT 4

double xscroll, yscroll;
char key_table[KEY_AMOUNT];

struct object {
    vec3 pos;
    vec3 rot;
    unsigned int vao;
    unsigned int shader;
}; 

enum camera_mode { MODE_NONE = 0, MODE_PLAYER, MODE_AIRPLANE, MODE_STARE, MODE_ORBIT }; 

struct Camera 
{
    vec3 pos;
    vec3 up;
    vec3 front;
    enum camera_mode mode;

    mat4 projection;
    mat4 view;
    float yaw, pitch, fov;
}; 

struct engine_state 
{
    struct object** obj;
    unsigned int shade;
    struct Camera cam;
    GLFWwindow* w;
    
    size_t count;
    size_t floppa_range; 

    int screen_x;
    int screen_y;
};

struct shader_contenxt 
{
    const char* vertex;
    //const char* geometry;
    const char* fragment;
    unsigned int id;
};


void terminate_program();

unsigned int update_shader_program(char* vert_shader, char* frag_shader);

struct Camera new_camera(enum camera_mode mode)
{ 
    return (struct Camera) 
    { 	.projection = {{ 0 }},
	.view = {{ 0 }},
	.pos =  { 0 },
	.front =  {0, 0, -1}, 
	.up =  {0, 1, 0},
	.fov = 45.0f,
	.pitch = -90,
	.yaw = 0,

	.mode = mode,
    }; 
}

float current_frame = 0, last_frame = 0, delta_time; 

void update_delta_time()
{
    delta_time = current_frame - last_frame;
    last_frame = current_frame;
    current_frame = glfwGetTime(); 
}

#define round_vec3(x) (vec3) {round(x[X]), round(x[Y]), round(x[Z])} 
#define root_grid_pos (vec3) { round(root[i]->pos[X]), round(root[i]->pos[Y]), round(root[i]->pos[Z])}
#define obj_grid_pos (vec3) { round(g.obj[i]->pos[X]), round(g.obj[i]->pos[Y]), round(g.obj[i]->pos[Z])}

#define DUMP_FILE "scene.floppa"

void pop_floppa(struct object ** root, vec3 position, size_t count)
{
    for (unsigned int i = 0; i < count; ++i)
    {
	if( root[i] != NULL ) { 
	    if (memcmp(position, root_grid_pos, sizeof(vec3)) == 0)
	    { 
		free(root[i]);
		root[i] = root[count - 1];
		root[count - 1] = NULL;
	    }
	}
    }
}
void dump_floppas(struct object ** root, size_t count, struct Camera cam)
{ 
    FILE* target_file = fopen(DUMP_FILE, "w");

    fwrite(&count, sizeof(size_t), 1, target_file);
    fwrite(&cam, sizeof(struct Camera), 1, target_file);

    for (unsigned int i = 0; i < count; ++i){
	if (root[i] != NULL)
	{
	    fwrite(root[i], sizeof(struct object), 1, target_file);
	}
    }
    fclose(target_file); 
}
void free_floppas(struct object ** root, size_t *count)
{
    size_t i = 0;
    while ( i < *count ) free(root[i++]); 
    *count = 0;
}
void load_floppas(struct object ** root, size_t *count, struct Camera * cam)
{
    free_floppas(root, count);

    FILE * target_file = fopen("scene.floppa", "r");
    if (target_file == NULL){
	fprintf(stderr, "nao foi possivel encontrar arquivo\n");
	return;
    }

    fread(count, sizeof(size_t), 1, target_file);
    fread(cam, sizeof(struct Camera), 1, target_file);

    for (size_t i = 0; i < *count; ++i)
    {
        struct object * tmp = malloc(sizeof(struct object)); 
        fread(tmp, sizeof(struct object), 1, target_file);
        root[i] = tmp; 
    }
    fclose(target_file);
    
} 
bool is_space_free(struct engine_state g, vec3 space)
{
    for (unsigned int i = 0; i < g.count; ++i) {
	if(g.obj[i] != NULL)
	{ 
	    if (memcmp(space, obj_grid_pos, sizeof(vec3)) == 0)
		return false; 
	}
    }
    return true; 
}
struct object* instance_floopa(vec3 position) //ehhhh
{
    return memcpy(malloc(sizeof(struct object)), &(struct object) 
    { .pos = { position[X], position[Y], position[Z] } }, sizeof(struct object)); 
}
struct engine_state 
new_floppa(struct engine_state g, vec3 position)
{ 
    if (!is_space_free(g, position)){
	return g;
    } 
    g.obj[g.count] = instance_floopa(position); 
    g.count += 1;
    return g;
} 
int rand_over_range(const int range) 
{
    return (rand() % range) - (range / (int) 2); 
} 
struct engine_state 
rand_floppa(struct engine_state g, int MAX_FLOP, const int RANGE)
{
    srand(time(NULL));

    while (MAX_FLOP) { 
	g = new_floppa(g, (vec3) { rand_over_range(RANGE), rand_over_range(RANGE), rand_over_range(RANGE)});

	--MAX_FLOP; 
    } 

    return g;
} 

bool firstMouse = true;
float lastX = 400, lastY = 300;

struct Camera fps_camera_update(GLFWwindow * w, struct Camera cam) 
{ 
    double xpos, ypos; glfwGetCursorPos(w, &xpos, &ypos);		

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; 
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    cam.yaw += xoffset;
    cam.pitch += yoffset;

    if (cam.pitch > 89.0f)
	cam.pitch = 89.0f;
    if (cam.pitch < -89.0f)
	cam.pitch = -89.0f;

    vec3 front;
    front[X] = cos(glm_rad(cam.yaw)) * cos(glm_rad(cam.pitch));
    front[Y] = sin(glm_rad(cam.pitch));
    front[Z] = sin(glm_rad(cam.yaw)) * cos(glm_rad(cam.pitch));
    glm_vec3_normalize(front); glm_vec3_copy(front, cam.front);

    return cam; 
}

#define MOVE_SPEED 10 * delta_time
#define ORBIT_SPEED 0.2

struct engine_state
cam_orbit_mode(struct engine_state g)  
{ 

    float radius = 25;


    if (glfwGetKey(g.w, GLFW_KEY_W) == GLFW_PRESS)
    {
	vec3 horizon_plane = { g.cam.front[X], 0, g.cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_add(horizon_plane, g.cam.pos, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_S) == GLFW_PRESS)
    {
	//vec3 ret = { 0 };
	vec3 horizon_plane = { g.cam.front[X], 0, g.cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_sub(g.cam.pos, horizon_plane, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_D) == GLFW_PRESS)
    {
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_A) == GLFW_PRESS)
    {
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_SPACE) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos); 
    } 
    if (glfwGetKey(g.w, GLFW_KEY_Q) == GLFW_PRESS)
    {
	g.cam.yaw += ORBIT_SPEED;
    }
    if (glfwGetKey(g.w, GLFW_KEY_E) == GLFW_PRESS)
    {
	g.cam.yaw -= ORBIT_SPEED;
    } 

    float rot_x = sin(g.cam.yaw * ORBIT_SPEED) * radius;
    float rot_z = cos(g.cam.yaw * ORBIT_SPEED) * radius;

    g.cam = fps_camera_update(g.w, g.cam);

    glm_mat4_identity(g.cam.projection);
    glm_mat4_identity(g.cam.view);

    glm_perspective(glm_rad(g.cam.fov), (float) g.screen_x / g.screen_y, 0.01, 400, g.cam.projection);
    glm_lookat( (vec3) {rot_x + g.cam.pos[X], 
	    		g.cam.pos[Y], 
	    		rot_z + g.cam.pos[Z]}, 
	    	g.cam.pos, g.cam.up, 
	    	g.cam.view); 

    return g;
} 

struct engine_state
cam_stare_mode(struct engine_state g)  
{ 
    glm_mat4_identity(g.cam.projection);
    glm_mat4_identity(g.cam.view);

    glm_perspective(glm_rad(g.cam.fov), (float) g.screen_x / g.screen_y, 0.01, 400, g.cam.projection);
    glm_lookat(g.cam.pos, (vec3) {0, 0, 0}, g.cam.up, g.cam.view); 

    if (glfwGetKey(g.w, GLFW_KEY_W) == GLFW_PRESS)
    {
	vec3 horizon_plane = { g.cam.front[X], 0, g.cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_add(horizon_plane, g.cam.pos, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_S) == GLFW_PRESS)
    {
	//vec3 ret = { 0 };
	vec3 horizon_plane = { g.cam.front[X], 0, g.cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_sub(g.cam.pos, horizon_plane, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_D) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_A) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_SPACE) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos); 
    } 

    return g;
} 


struct engine_state
cam_player_mode(struct engine_state g)
{ 

    if (glfwGetKey(g.w, GLFW_KEY_W) == GLFW_PRESS)
    {
	vec3 horizon_plane = { g.cam.front[X], 0, g.cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_add(horizon_plane, g.cam.pos, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_S) == GLFW_PRESS)
    {
	//vec3 ret = { 0 };
	vec3 horizon_plane = { g.cam.front[X], 0, g.cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_sub(g.cam.pos, horizon_plane, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_D) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_A) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_SPACE) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos); 
    } 
    
    g.cam = fps_camera_update(g.w, g.cam);
    glm_mat4_identity(g.cam.projection);
    glm_mat4_identity(g.cam.view);

    vec3 camera_target; 
    glm_vec3_add(g.cam.pos, g.cam.front, camera_target); 
    glm_perspective(glm_rad(g.cam.fov), (float) g.screen_x / g.screen_y, 0.01, 400, g.cam.projection);
    glm_lookat(g.cam.pos, camera_target, g.cam.up, g.cam.view);

    //glm_lookat(g.cam.pos, (vec3) {0, 0, 0}, g.cam.up, g.cam.view); 

    return g;
} 
struct engine_state
cam_airplane_mode(struct engine_state g)
{ 
    if (glfwGetKey(g.w, GLFW_KEY_W) == GLFW_PRESS)
    {
	vec3 ret = { 0 };
	glm_vec3_copy(g.cam.front, ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_S) == GLFW_PRESS)
    {
	vec3 ret = { 0 };
	glm_vec3_copy(g.cam.front, ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_D) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_A) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(g.cam.front, g.cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_SPACE) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_add(g.cam.pos, ret, g.cam.pos);
    }
    if (glfwGetKey(g.w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(g.cam.up, MOVE_SPEED, ret);
	glm_vec3_sub(g.cam.pos, ret, g.cam.pos); 
    } 

    g.cam = fps_camera_update(g.w, g.cam);
    glm_mat4_identity(g.cam.projection);
    glm_mat4_identity(g.cam.view);

    vec3 camera_target; 
    glm_vec3_add(g.cam.pos, g.cam.front, camera_target); 
    glm_perspective(glm_rad(g.cam.fov), (float) g.screen_x / g.screen_y, 0.01, 400, g.cam.projection);
    glm_lookat(g.cam.pos, camera_target, g.cam.up, g.cam.view);

    //glm_lookat(g.cam.pos, (vec3) {0, 0, 0}, g.cam.up, g.cam.view); 

    return g;
}

#define FLOPPA_COUNT 500

struct engine_state 
process_input(struct engine_state g)
{ 
    switch(g.cam.mode)
    {
	case MODE_STARE:
	    g = cam_stare_mode(g);
	    break;
	case MODE_PLAYER:
	    g = cam_player_mode(g);
	    break;
	case MODE_AIRPLANE:
	    g = cam_airplane_mode(g);
	    break;
	case MODE_ORBIT:
	    g = cam_orbit_mode(g);
	    break;
	default:
	    break; 
    }
    if(glfwGetKey(g.w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
	terminate_program();
	printf("foobar: ESC pressed. exiting.\n");
	glfwSetWindowShouldClose(g.w, true);
    }
    if (glfwGetKey(g.w, GLFW_KEY_F1) == GLFW_PRESS){
	glUseProgram(g.shade);
	g.shade = update_shader_program("shaders/shader.vert", "shaders/shader.frag");
    }
    if (glfwGetKey(g.w, GLFW_KEY_F2) == GLFW_PRESS){ 
	dump_floppas(g.obj, g.count, g.cam);
    }
    if (glfwGetKey(g.w, GLFW_KEY_F3) == GLFW_PRESS){ 
	load_floppas(g.obj, &g.count, &g.cam);
    }
    if (glfwGetKey(g.w, GLFW_KEY_F4) == GLFW_PRESS){ 
	g.floppa_range += 5;
	free_floppas(g.obj, &g.count);
	g = rand_floppa(g, FLOPPA_COUNT, g.floppa_range); 
    } 
    if (glfwGetKey(g.w, GLFW_KEY_F5) == GLFW_PRESS && key_table[glfwGetKeyScancode(GLFW_KEY_F5)] == GLFW_RELEASE) //ehhhhhhhhhh
    { 
	g.floppa_range -= (g.floppa_range > 5) ? 5: 0;
	free_floppas(g.obj, &g.count);
	g = rand_floppa(g, FLOPPA_COUNT, g.floppa_range); 

	key_table[glfwGetKeyScancode(GLFW_KEY_F5)] = GLFW_PRESS; 
    } 
    else if (glfwGetKey(g.w, GLFW_KEY_F5) == GLFW_RELEASE) key_table[glfwGetKeyScancode(GLFW_KEY_F5)] = GLFW_RELEASE; //ehhhhhhhhhh

    if (glfwGetKey(g.w, GLFW_KEY_T) == GLFW_PRESS && key_table[glfwGetKeyScancode(GLFW_KEY_T)] == GLFW_RELEASE) //ehhhhhhhhhh
    { 
	if (g.cam.mode >= MODE_COUNT) g.cam.mode = 1;
	else g.cam.mode += 1; 

	key_table[glfwGetKeyScancode(GLFW_KEY_T)] = GLFW_PRESS; 
    } 
    else if (glfwGetKey(g.w, GLFW_KEY_T) == GLFW_RELEASE) key_table[glfwGetKeyScancode(GLFW_KEY_T)] = GLFW_RELEASE; //ehhhhhhhhhh

    if (glfwGetKey(g.w, GLFW_KEY_X) == GLFW_PRESS && key_table[glfwGetKeyScancode(GLFW_KEY_X)] == GLFW_RELEASE) //ehhhhhhhhhh
    { 
	g = new_floppa(g, g.cam.pos); 
    } 
    else if (glfwGetKey(g.w, GLFW_KEY_X) == GLFW_RELEASE) key_table[glfwGetKeyScancode(GLFW_KEY_X)] = GLFW_RELEASE; //ehhhhhhhhhh

    return g;
} 





void get_scroll(GLFWwindow * window, double xoffset, double yoffset) //fuck callback functions
{
	xscroll = xoffset; 
	yscroll = yoffset;

	window = window; // silenciar erro
} 
struct Camera
update_scroll(struct Camera cam)
{
    cam.fov -= yscroll * SCROLL_SPEED; 

    xscroll = 0; 
    yscroll = 0; 
    //if (fov < FOV_MIN) fov = FOV_MIN;
    //if (fov > FOV_MAX) fov = FOV_MAX;
    return cam;
}

void framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
    glViewport(0, 0, width, height); 
    window = window;
} 


unsigned int load_texture(const char *filename)
{
    unsigned int texture; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);  
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data)
    {
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);


	stbi_image_free(data); return texture; 
    }
    else
    {
	fprintf(stderr, "falha ao carregar imagem\n");
	exit(1);
    } 
} 
char* file_to_string(const char * filename)
{ 
    FILE* tmp = fopen(filename, "r");
    if (tmp == NULL)
    {
	fprintf(stderr, "arquivo %s inexistente\n", filename);
	exit(1);
    }
    fseek(tmp, 0, SEEK_END);
    size_t file_length = ftell(tmp);
    rewind(tmp);

    char * ret = malloc(sizeof(char) * (file_length + 1));

    fread(ret, sizeof(char), file_length, tmp);

    ret[file_length] = '\0';

    return ret; 
} 
unsigned int compile_shader(const char * shader_source, GLenum target_shader)
{ 
    unsigned int shader_process; 
    shader_process = glCreateShader(target_shader);  

    glShaderSource(shader_process, 1, &shader_source, NULL); 
    glCompileShader(shader_process); 

    int success;
    char infolog[521]; 
    glGetShaderiv(shader_process, GL_COMPILE_STATUS, &success); 
    if(!success)
    {
	glGetShaderInfoLog(shader_process, 512, NULL, infolog); 
	fprintf(stderr, "falha ao compilar shader\n");
	printf("%s\n", infolog);
	exit(1);
    }
    
    return shader_process;
}
unsigned int update_shader_program(char * vert_shader, char * frag_shader)
{ 
    char * vert_source = file_to_string(vert_shader);
    char * frag_source = file_to_string(frag_shader);

    unsigned int vert_process = compile_shader(vert_source, GL_VERTEX_SHADER);
    unsigned int frag_process = compile_shader(frag_source, GL_FRAGMENT_SHADER);   

    free(vert_source); free(frag_source);

    unsigned int shader_program;  
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vert_process);
    glAttachShader(shader_program, frag_process);
    glLinkProgram(shader_program);

    int success;
    char infolog[521]; 
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if(!success)
    {
	glGetProgramInfoLog(shader_program, 512, NULL, infolog);
	fprintf(stderr, "falha bo shder link :(");
	printf("%s\n", infolog);
	exit(1);
    } 

    glDeleteShader(vert_process);
    glDeleteShader(frag_process);

    return shader_program;

}

unsigned int normal_cube()
{

float vertices[] = 
{
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};
     


    unsigned int VBO; glGenBuffers(1, &VBO);  
    unsigned int VAO; glGenVertexArrays(1, &VAO); 
    unsigned int EBO; glGenBuffers(1, &EBO); 

    glBindVertexArray(VAO); 

    glBindBuffer(GL_ARRAY_BUFFER, VBO); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);  

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tetra_index), tetra_index, GL_STATIC_DRAW); 

    //aPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *) 0);
    glEnableVertexAttribArray(0); 

    //aTexCoord
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *) (sizeof(float) * 3) );
    glEnableVertexAttribArray(2); 


    return VAO; 
}

unsigned int floppa_cube()
{
    float cube_vertices[] = {
        -0.5f, -0.5f, -0.5f, 	0.0f, 0.0f, 	0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  	1.0f, 0.0f,  	0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  	1.0f, 1.0f,  	0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  	1.0f, 1.0f,  	0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f, 	0.0f, 1.0f, 	0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f, 	0.0f, 0.0f, 	0.0f,  0.0f, -1.0f,
                             	             	                   
        -0.5f, -0.5f,  0.5f, 	0.0f, 0.0f, 	0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  	1.0f, 0.0f,  	0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  	1.0f, 1.0f,  	0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  	1.0f, 1.0f,  	0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 	0.0f, 1.0f, 	0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 	0.0f, 0.0f, 	0.0f,  0.0f, 1.0f,
                             	             	                   
        -0.5f,  0.5f,  0.5f, 	1.0f, 0.0f, 	1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, 	1.0f, 1.0f, 	1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, 	0.0f, 1.0f, 	1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, 	0.0f, 1.0f, 	1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, 	0.0f, 0.0f, 	1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, 	1.0f, 0.0f, 	1.0f,  0.0f,  0.0f,
                             	             	                   
         0.5f,  0.5f,  0.5f,  	1.0f, 0.0f,  	1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  	1.0f, 1.0f,  	1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  	0.0f, 1.0f,  	1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  	0.0f, 1.0f,  	1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  	0.0f, 0.0f,  	1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  	1.0f, 0.0f,  	1.0f,  0.0f,  0.0f,
                             	             	                   
        -0.5f, -0.5f, -0.5f, 	0.0f, 1.0f, 	0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  	1.0f, 1.0f,  	0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  	1.0f, 0.0f,  	0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  	1.0f, 0.0f,  	0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, 	0.0f, 0.0f, 	0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, 	0.0f, 1.0f, 	0.0f, -1.0f,  0.0f,
                             	             	                   
        -0.5f,  0.5f, -0.5f, 	0.0f, 1.0f, 	0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  	1.0f, 1.0f,  	0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  	1.0f, 0.0f,  	0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  	1.0f, 0.0f,  	0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, 	0.0f, 0.0f, 	0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, 	0.0f, 1.0f, 	0.0f,  1.0f,  0.0f
    }; 
     


    unsigned int VBO; glGenBuffers(1, &VBO);  
    unsigned int VAO; glGenVertexArrays(1, &VAO); 
    //unsigned int EBO; glGenBuffers(1, &EBO); 

    glBindVertexArray(VAO); 

    glBindBuffer(GL_ARRAY_BUFFER, VBO); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);  

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tetra_index), tet8a_index, GL_STATIC_DRAW); 

    //aPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void *) 0);
    glEnableVertexAttribArray(0); 

    //aTexCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void *) (sizeof(float) * 3) );
    glEnableVertexAttribArray(1); 

    //aNormal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void *) (sizeof(float) * 5) );
    glEnableVertexAttribArray(2); 

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return VAO; 
}

unsigned int light_cube()
{
    float cube_vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    }; 
     


    unsigned int VBO; glGenBuffers(1, &VBO);  
    unsigned int VAO; glGenVertexArrays(1, &VAO); 

    glBindBuffer(GL_ARRAY_BUFFER, VBO); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);  

    glBindVertexArray(VAO); 

    //aPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) 0);
    glEnableVertexAttribArray(0); 

    return VAO; 
}



float sier_side(float h){ 	return (2 * h) / sqrt(3); }
float inscrib_circle(float s){ 	return (sqrt(3) / 6) * s; }
float cirscum_circle(float s){ 	return s / sqrt(3)	; }

unsigned int tetrahedron_vao()
{ 
    float s = sier_side(2);
    float hs = s / 2;
    float ic = inscrib_circle(s);
    float cs = cirscum_circle(s);

    float vertices[] = {
     0,   1,  0,  //vertice topo 
    -hs, -1,  ic, //vertice esquerdo
     hs, -1,  ic, //vertice direito
     0,  -1, -cs, //vertice traseiro
    };
    int indices[] = {
    0, 1, 2, //face frontal
    0, 2, 3, //face direita
    0, 3, 2, //face esquerda
    1, 2, 3, //base
    }; 

    //for ( int i = 0; i < sizeof(indices) / sizeof(float); ++i ){
    //    printf("%f, ", vertices[i]); 
    //    if ( (i + 1) % 3 == 0 ) printf("\n");
    //}

    unsigned int VAO, VBO, EBO; 
    glGenVertexArrays(1, &VAO); 
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *) 0); 
    glEnableVertexAttribArray(0);

    //glBindVertexArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, 0); 

    return VAO;
    
}

unsigned int hello_triangle()
{ 
    float vertices[] = {
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	0.0f,  0.5f, 0.0f
    };  

    unsigned int VBO, VAO; glGenBuffers(1, &VBO); glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *) 0);
    glEnableVertexAttribArray(0); 

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);
    return VAO;
}
unsigned int framebuffer_quad()
{ 
    float quadVertices[] = {  
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
    };	

    unsigned int VBO, VAO; glGenBuffers(1, &VBO); glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *) 0);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *) 2);
    glEnableVertexAttribArray(1); 

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);
    return VAO;
}

void set_uniform_matrix( int shader_program, const char* uniform_name, mat4 mat)
{ 
    glUseProgram(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, uniform_name), 1, GL_FALSE, (float *) mat); 
}
void set_uniform_3vector( int shader_program, const char* uniform_name, vec3 vec)
{ 
    glUseProgram(shader_program);
    glUniform3f(glGetUniformLocation(shader_program, uniform_name), vec[X], vec[Y], vec[Z]); 
}
void set_uniform_1float( int shader_program, const char* uniform_name, float f)
{ 
    glUseProgram(shader_program);
    glUniform1f(glGetUniformLocation(shader_program, uniform_name), f); 
}

int main(void)
{ 
    struct engine_state game = { 0 };
    game.floppa_range = 40; 
    memset(key_table, GLFW_RELEASE, KEY_AMOUNT);
    yscroll = 0; xscroll = 0; 
    printf("hello world\n");


    //glfw fluff 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
    

    game.w = glfwCreateWindow(500, 500, "caracal", NULL, NULL);
    if (game.w == NULL) {
	glfwTerminate();
	return -1;
    } 
    glfwMakeContextCurrent(game.w);

    if( !gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) )
    {
	return -1;
    }
    glfwGetWindowSize(	game.w, &game.screen_x, &game.screen_y); 

    glfwSetFramebufferSizeCallback(game.w, framebuffer_size_callback); 
    //glfwSetCursorPosCallback(game.w, get_mouse_pos);
    glfwSetScrollCallback(game.w, get_scroll);
    glfwSetInputMode(game.w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfw fluff 

    //inicializacao

    unsigned int FBO;  unsigned int texture_color_buffer; {
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glGenTextures(1, &texture_color_buffer);
	glBindTexture(GL_TEXTURE_2D, texture_color_buffer); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, game.screen_x, game.screen_y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_color_buffer, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned int renderbuffer;
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer); 
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, game.screen_x, game.screen_y);
	glFramebufferRenderbuffer(GL_RENDERBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer); 
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
	fprintf(stderr, "framebuffer imcompleto\n");
	return 1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    unsigned int quad = 		framebuffer_quad();
    unsigned int triangle = 		hello_triangle();
    unsigned int quad_shader = 		update_shader_program( "shaders/quad.vert", "shaders/quad.frag" );
    unsigned int test_shader = 		update_shader_program( "shaders/alt_shader.vert", "shaders/alt_shader.frag" );
    unsigned int VAO = 			normal_cube();
    unsigned int vao_light = 		light_cube();
    unsigned int shader_light = 	update_shader_program( "shaders/shader.vert", "shaders/lightcube.frag" );
    unsigned int texture1 = 		load_texture("floppa.jpg"); 

    game.shade = 	update_shader_program("shaders/shader.vert", "shaders/shader.frag"); 
    game.cam = 		new_camera(MODE_PLAYER); 
    game.count = 	0;
    game.obj = 		calloc(5000, sizeof(struct object *));

    game = rand_floppa(game, FLOPPA_COUNT, 50); 

    //inicializacao 
    while(!glfwWindowShouldClose(game.w))
    {
	update_delta_time(); 
	//glBindFramebuffer(GL_FRAMEBUFFER, FBO); 
	//{ //a cena seguinte inteira é rederizada nesse framebuffer
	//    glClearColor(	0.01,0.01,0.01,0.01); 
	//    glClear(		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	//    glEnable(		GL_DEPTH_TEST); 
	//    glfwGetWindowSize(	game.w, &game.screen_x, &game.screen_y); 
	//    game = 		process_input(game); 
	//    game.cam = 		update_scroll(game.cam); 
	//    glUseProgram(	game.shade); 
	//    glActiveTexture(	GL_TEXTURE1);
	//    glBindTexture(	GL_TEXTURE_2D, texture1); 
	//    glUniform1i(	glGetUniformLocation(game.shade, "texture1"), 1); 
	//    glUniform3f(	glGetUniformLocation(game.shade, "objectColor"), 1, .5, .31); 
	//    glUniform3f(	glGetUniformLocation(game.shade, "lightColor"), 1, 1, 1); 
	//    set_uniform_matrix(	game.shade, "view", game.cam.view);
	//    set_uniform_matrix(	game.shade, "projection", game.cam.projection); 

	//    mat4 model = { 0 }; glm_mat4_identity(model);

	//    glBindVertexArray(VAO); 
	//    { //floppa
	//	for (size_t i = 0; i < game.count; ++i)
	//	{
	//	    if (game.obj[i] != NULL)
	//	    { 
	//		glm_mat4_identity(model);
	//		//glm_rotate(model, glfwGetTime() * 1, (vec3) {0, 1, 0});
	//		glm_translate(model, game.obj[i]->pos); 
	//		{ // lighting 
	//		    set_uniform_3vector(game.shade, "lightPos", (vec3) {0, 0, 0});
	//		    set_uniform_3vector(game.shade, "viewPos", game.cam.pos);

	//		    set_uniform_1float(game.shade, "light.ambient_strength", 0.05);
	//		    set_uniform_1float(game.shade, "light.specular_strength", 0.5);
	//		}
	//		glUniformMatrix4fv(glGetUniformLocation(game.shade, "model"), 1, GL_FALSE, (float *) model); 
	//		glDrawArrays(GL_LINE_STRIP, 0, 36);
	//	    }
	//	}
	//    }
	//    { //"hitbox" da camera
	//	glm_mat4_identity(model); 
	//	glm_translate(model, (vec3) { 
	//		round(game.cam.pos[X]), 
	//		round(game.cam.pos[Y]), 
	//		round(game.cam.pos[Z])});
	//	glUniformMatrix4fv(glGetUniformLocation(game.shade, "model"), 1, GL_FALSE, (float *) model); 
	//	glDrawArrays(GL_LINE_STRIP, 0, 36); 
	//    }
	//    { //lampada
	//	glUseProgram(shader_light);
	//	set_uniform_matrix(shader_light, "view", game.cam.view);
	//	set_uniform_matrix(shader_light, "projection", game.cam.projection); 

	//	glm_mat4_identity(model);
	//	glm_translate(model, (vec3) { 0, 0, 0 });
	//	glUniformMatrix4fv(glGetUniformLocation(shader_light, "model"), 1, GL_FALSE, (float *) model); 

	//	glBindVertexArray(vao_light); 
	//	glDrawArrays(GL_TRIANGLES, 0, 36); 
	//    }

	//}


	glfwGetWindowSize(game.w, &game.screen_x, &game.screen_y); 

	//glBindFramebuffer(GL_FRAMEBUFFER, FBO); 
	{ 
	    glClearColor(	0,1,1,1); 
	    glClear(		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	    glEnable(		GL_DEPTH_TEST); 
	    glUseProgram(	test_shader); 

	    glBindVertexArray(	triangle);
	    glDrawArrays(	GL_TRIANGLES, 0, 3);

	} 
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(		GL_DEPTH_TEST);
	glClearColor(		1,1,1,1); 
	//glClear(		GL_COLOR_BUFFER_BIT); 
	glUseProgram(		quad_shader); 
	glBindVertexArray(	quad); 
	glUniform1i(glGetUniformLocation(quad_shader, "screen_texture"), 0); 
	glBindTexture(		GL_TEXTURE_2D, texture_color_buffer); 
	//glActiveTexture(	GL_TEXTURE0);

	glDrawArrays(GL_LINES, 0, 6);

	game.cam.mode = MODE_NONE;
	game = process_input(game); 

	glfwSwapBuffers(game.w);
	glfwPollEvents();

	//glBindFramebuffer(GL_FRAMEBUFFER, 0); { //o framebuffer anterior é renderizado em uma textura que logo em seguida é renderizada para o framebuffer padrão.
	//    glClearColor(1,1,1,1); 
	//    glClear(GL_COLOR_BUFFER_BIT);

	//    glUseProgram(quad_shader);
	//    glDisable(GL_DEPTH_TEST);
	//    glBindVertexArray(quad);

	//    glUniform1i(glGetUniformLocation(quad_shader, "screen_texture"), 1); 

	//    glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
	//    glDrawArrays(GL_TRIANGLES, 0, 6); 
	//}


    }

    terminate_program();

}

void terminate_program(
#ifdef AUTO_LOAD
struct engine_state g
#endif
)
{

#ifdef AUTO_LOAD
    dump_floppas(g.obj, g.count, g.cam);
#endif 
    printf("vendor: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    int attribs;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &attribs); 
    printf("numbero maximo de attributos suportados: %d\n", attribs);

    glfwTerminate();

    exit(0);

}

void print_matrix(mat4 mat)
{
    for (int i = 0; i < 4; ++i){
	printf("| ");
	for (int j = 0; j < 4; ++j)
	{
	    printf("%.0f ", mat[j][i]);
	}
	printf("|\n");
    }
    printf("\n");
} 
