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

double xscroll, yscroll;

#define KEY_AMOUNT 245
char key_table[KEY_AMOUNT];

struct object {
    vec3 pos;
    vec3 rot;
    unsigned int vao;
}; 

enum camera_mode { MODE_NONE = 0, MODE_PLAYER, MODE_AIRPLANE  }; 

struct Camera {
    vec3 pos;
    vec3 up;
    vec3 front;
    enum camera_mode mode;

    float yaw, pitch, fov;
}; 

struct engine_context {
    struct object ** obj;
    unsigned int shade;
    struct Camera cam;
    GLFWwindow * w;
    
    size_t count;
    size_t floppa_range; 

    int screen_x;
    int screen_y;
};


void terminate_program();

unsigned int update_shader_program(char * vert_shader, char * frag_shader);

struct Camera new_camera(enum camera_mode mode)
{ 
    return (struct Camera) { 	.fov = 45,
    				.front =  {0, 0, -1}, 
    				.mode = mode,
    				.pitch = -90,
    				.pos =  {0, 0, 0},
    				.up =  {0, 1, 0},
    				.yaw = 0 }; 
}

float current_frame = 0, last_frame = 0, delta_time; 

void update_delta_time()
{
    delta_time = current_frame - last_frame;
    last_frame = current_frame;
    current_frame = glfwGetTime(); 
}

#define root_grid_pos (vec3) { round(root[i]->pos[X]), round(root[i]->pos[Y]), round(root[i]->pos[Z])}

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
    FILE * target_file = fopen(DUMP_FILE, "w");

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
size_t new_floppa(struct object **root, size_t count, vec3 position)
{ 
    for (unsigned int i = 0; i < count; ++i)
    {
	if(root[i] != NULL) { 
	    if (memcmp(position, root_grid_pos, sizeof(vec3)) == 0)
		return count;
	}
    }

    struct object * floppa = malloc(sizeof(struct object));
    memcpy(floppa->pos, (vec3) { 
	    round(position[X]), 
	    round(position[Y]), 
	    round(position[Z])}, sizeof(vec3)); 
    //floppa->vao = vao;

    root[count] = floppa; 
    return count + 1;
} 

void rand_floppa(struct object ** root, size_t *count, int MAX_FLOP, const int RANGE)
{
    srand(time(NULL));

    while (MAX_FLOP) {
	int rand_x = (rand() % RANGE) - (RANGE / (int) 2); 
	int rand_y = (rand() % RANGE) - (RANGE / (int) 2); 
	int rand_z = (rand() % RANGE) - (RANGE / (int) 2); 
	vec3 rand_pos = {rand_x, rand_y, rand_z};

	*count = new_floppa(root, *count, rand_pos);

	--MAX_FLOP; 
    } 
} 
#define MOVE_SPEED 50 * delta_time

struct Camera cam_player_mode(GLFWwindow * w, struct Camera cam)
{ 

    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
    {
	vec3 horizon_plane = { cam.front[X], 0, cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_add(horizon_plane, cam.pos, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
    {
	//vec3 ret = { 0 };
	vec3 horizon_plane = { cam.front[X], 0, cam.front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVE_SPEED, horizon_plane);
	glm_vec3_sub(cam.pos, horizon_plane, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(cam.front, cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(cam.front, cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(cam.up, MOVE_SPEED, ret);
	glm_vec3_add(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(cam.up, MOVE_SPEED, ret);
	glm_vec3_sub(cam.pos, ret, cam.pos); 
    } 

    return cam;
} 
struct Camera cam_airplane_mode(GLFWwindow * w, struct Camera cam)
{ 
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
    {
	vec3 ret = { 0 };
	glm_vec3_copy(cam.front, ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
    {
	vec3 ret = { 0 };
	glm_vec3_copy(cam.front, ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(cam.front, cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_add(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(cam.front, cam.up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVE_SPEED, ret);
	glm_vec3_sub(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(cam.up, MOVE_SPEED, ret);
	glm_vec3_add(cam.pos, ret, cam.pos);
    }
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(cam.up, MOVE_SPEED, ret);
	glm_vec3_sub(cam.pos, ret, cam.pos); 
    } 

    return cam;
}

#define FLOPPA_COUNT 1000

struct engine_context 
process_input(struct engine_context g)
{ 
    switch(g.cam.mode)
    {
	case MODE_PLAYER:
	    g.cam = cam_player_mode(g.w, g.cam);
	    break;
	case MODE_AIRPLANE:
	    g.cam = cam_airplane_mode(g.w, g.cam);
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
	g.shade = update_shader_program("shader.vert", "shader.frag");
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
	rand_floppa(g.obj, &g.count, FLOPPA_COUNT, g.floppa_range); 
    } 
    if (glfwGetKey(g.w, GLFW_KEY_F5) == GLFW_PRESS && key_table[glfwGetKeyScancode(GLFW_KEY_F5)] == GLFW_RELEASE){ 
	g.floppa_range -= (g.floppa_range > 5) ? 5: 0;
	free_floppas(g.obj, &g.count);
	rand_floppa(g.obj, &g.count, FLOPPA_COUNT, g.floppa_range); 

	key_table[glfwGetKeyScancode(GLFW_KEY_F5)] = GLFW_PRESS; 
    } 
    else if (glfwGetKey(g.w, GLFW_KEY_F5) == GLFW_RELEASE) key_table[glfwGetKeyScancode(GLFW_KEY_F5)] = GLFW_RELEASE;

    return g;
} 


bool firstMouse = true;
float lastX = 400, lastY = 300;

struct Camera mouse_callback(double xpos, double ypos, struct Camera cam) 
{ 
    //if (firstMouse)
    //{
    //    lastX = xpos;
    //    lastY = ypos;
    //    firstMouse = false;
    //}

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
void get_scroll(GLFWwindow * window, double xoffset, double yoffset) //fuck callback functions
{
	xscroll = xoffset; 
	yscroll = yoffset;
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
	fprintf(stderr, "deu pra carregar a imagem nao chefia\n");
	exit(1);
    } 
} 
char * file_to_string(const char * filename)
{ 
    FILE* tmp = fopen(filename, "r");
    if (tmp == NULL)
    {
	fprintf(stderr, "esse nome aqui %s nao existe nao negao\n", filename);
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
	glGetShaderInfoLog(shader_process, 512, NULL, infolog); //log do erro
	fprintf(stderr, "falha bo shder :(");
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
    char infolog[521]; //buffer onde erro sera logado
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


unsigned int floppa_cube()
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
    unsigned int EBO; glGenBuffers(1, &EBO); 

    glBindVertexArray(VAO); 

    glBindBuffer(GL_ARRAY_BUFFER, VBO); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);  

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tetra_index), tetra_index, GL_STATIC_DRAW); 

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) 0);
    glEnableVertexAttribArray(0); 

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) (sizeof(float) * 3) );
    glEnableVertexAttribArray(1); 

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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

int main(void)
{ 
    struct engine_context game = { 0 };
    game.floppa_range = 40; 
    memset(key_table, GLFW_RELEASE, KEY_AMOUNT);
    yscroll = 0; xscroll = 0; 
    printf("hello world\n");

    //glfw fluff 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
    

    game.w = glfwCreateWindow(500, 500, "teste", NULL, NULL);
    if (game.w == NULL) {
	glfwTerminate();
	return -1;
    } 
    glfwMakeContextCurrent(game.w);

    if( !gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) )
    {
	return -1;
    }

    glfwSetFramebufferSizeCallback(game.w, framebuffer_size_callback); 
    //glfwSetCursorPosCallback(game.w, mouse_callback);
    glfwSetScrollCallback(game.w, get_scroll);
    glfwSetInputMode(game.w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfw fluff 

    //inicializacao
    glEnable(GL_DEPTH_TEST); 

    unsigned int VAO = 			floppa_cube();
    		 game.shade = 		update_shader_program("shader.vert", "shader.frag"); 
    unsigned int texture1 = 		load_texture("floppa.jpg"); 
    		 game.cam = 		new_camera(MODE_AIRPLANE);
    
    glUseProgram(game.shade); 

    game.count = 0;
    game.obj = calloc(5000, sizeof(struct object *));

//#define AUTO_LOAD

#ifdef AUTO_LOAD
    if (fopen(DUMP_FILE, "r") != NULL)
    {
	load_floppas(game.obj, &game.count, &game.cam);
    }
#else
    rand_floppa(game.obj, &game.count, FLOPPA_COUNT, 50); 
#endif
    //inicializacao


    while(!glfwWindowShouldClose(game.w))
    {
	update_delta_time(); 
	glClearColor(		(glfwGetTime() + 10) * 30, (glfwGetTime() + 15) * 30, (glfwGetTime() + 20) * 30, 1); 
	glClear(		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	game = 			process_input(game); 
	game.cam = 		update_scroll(game.cam);
	glUseProgram(		game.shade); 
	glfwGetWindowSize(	game.w, &game.screen_x, &game.screen_y); 

	glActiveTexture(	GL_TEXTURE1);
	glBindTexture(		GL_TEXTURE_2D, texture1); 
	glUniform1i(		glGetUniformLocation(game.shade, "texture1"), 1); 

	mat4 model = { 0 }; 		glm_mat4_identity(model);
	mat4 projection = { 0 }; 	glm_mat4_identity(projection); 
	mat4 view = { 0 }; 		glm_mat4_identity(view); 

	glm_perspective(glm_rad(game.cam.fov), (float) game.screen_x / game.screen_y, 0.01, 400, projection);
	//glm_ortho(		-1.0f, 1, -1, 1, -1, 1, projection); 

	vec3 camera_target; 
	//glm_vec3_add(	main_cam->pos, main_cam->front, camera_target); 
	
	float speed = 0.5;
	float radius = 25;
	float rot_x = sin(glfwGetTime() * speed) * radius;
	float rot_z = cos(glfwGetTime() * speed) * radius;

	glm_lookat((vec3) {rot_x, 0, rot_z}, (vec3) {0, 0, 0}, game.cam.up, view);

	unsigned int viewloc = 	glGetUniformLocation(game.shade, "view");
	unsigned int projloc = 	glGetUniformLocation(game.shade, "projection"); 

	glUniformMatrix4fv(viewloc, 1, GL_FALSE, (float *) view); 
	glUniformMatrix4fv(projloc, 1, GL_FALSE, (float *) projection); 


	glBindVertexArray(VAO); 
	{ 
	    for (size_t i = 0; i < game.count; ++i)
	    {
	        if (game.obj[i] != NULL)
	        { 
	            #define floppa_pos game.obj[i]->pos
	            #define relative_pos floppa_pos[X] / 20 , floppa_pos[Y] / 20, floppa_pos[Z] / 20
	    
	            glm_mat4_identity(model);
	            glm_translate(model, floppa_pos); 
	            glUniform3f(glGetUniformLocation(game.shade, "relative_color"),  relative_pos);
	            glUniformMatrix4fv(glGetUniformLocation(game.shade, "model"), 1, GL_FALSE, (float *) model); 
	            glDrawArrays(GL_TRIANGLES, 0, 36);
	        }
	    }
	}
	glm_mat4_identity(model);
	glm_translate(model, (vec3) { round(game.cam.pos[X]), round(game.cam.pos[Y]), round(game.cam.pos[Z])});
	glUniformMatrix4fv(glGetUniformLocation(game.shade, "model"), 1, GL_FALSE, (float *) model); 
	glDrawArrays(GL_LINE_STRIP, 0, 36); 
	

	glfwSwapBuffers(game.w);
	glfwPollEvents();
    }

    terminate_program();

}

void terminate_program(struct engine_context g)
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
