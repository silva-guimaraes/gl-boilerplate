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

int screen_x;
int screen_y;
unsigned int shader_program; 

struct node {
    struct object * obj;
    struct node * next; 
};

struct object {
    vec3 pos;
    vec3 rot;
    unsigned int vao;
}; 
struct Camera {
    vec3 pos;
    vec3 up;
    vec3 front;

    float yaw, pitch, fov;
}; 

struct object ** object_root;
struct node * root_node;
size_t object_count; 
struct Camera * main_cam; 


void terminate_program();

unsigned int update_shader_program(char * vert_shader, char * frag_shader);

struct Camera * new_camera()
{
    struct Camera *tmp = malloc(sizeof(struct Camera));

    memcpy(tmp->pos, (vec3) {0, 0, 0}, sizeof(vec3));
    memcpy(tmp->up, (vec3) { 0, 1, 0 }, sizeof(vec3));
    memcpy(tmp->front, (vec3) { 0, 0, -1 }, sizeof(vec3)); 

    tmp->pitch = -90; 
    tmp->yaw = 0; 
    tmp->fov = 45;

    return tmp; 
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
void dump_floppas(struct object ** root, size_t count, struct Camera * cam)
{
    FILE * target_file = fopen(DUMP_FILE, "w");

    fwrite(&count, sizeof(size_t), 1, target_file);
    fwrite(cam, sizeof(struct Camera), 1, target_file);

    for (unsigned int i = 0; i < count; ++i){
	if (root[i] != NULL){
	    fwrite(root[i], sizeof(struct object), 1, target_file);
	}
    }
    fclose(target_file); 
}
void load_floppas(struct object ** root, size_t *count, struct Camera * cam)
{
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
void free_floppas(struct object ** root, size_t *count)
{
    size_t i = 0;
    while ( i < *count ) free(root[i++]); 
    //free(root);
    *count = 0;
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



void process_input(GLFWwindow * window, struct Camera * cam)
{
    const float MOVEMENT_SPEED = 10.0 * delta_time;

    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
	terminate_program();
	printf("foobar: ESC pressed. exiting.\n");
	glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS){
	glUseProgram(shader_program);
	shader_program = update_shader_program("shader.vert", "shader.frag");
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS){ 
	dump_floppas(object_root, object_count, main_cam);
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS){ 
	load_floppas(object_root, &object_count, main_cam);
    }
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS){ 
	free_floppas(object_root, &object_count);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
	vec3 horizon_plane = { cam->front[X], 0, cam->front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVEMENT_SPEED, horizon_plane);
	glm_vec3_add(horizon_plane, cam->pos, cam->pos);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
	//vec3 ret = { 0 };
	vec3 horizon_plane = { cam->front[X], 0, cam->front[Z] };
	glm_vec3_normalize(horizon_plane);
	glm_vec3_scale(horizon_plane, MOVEMENT_SPEED, horizon_plane);
	glm_vec3_sub(cam->pos, horizon_plane, cam->pos);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(cam->front, cam->up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVEMENT_SPEED, ret);
	glm_vec3_add(cam->pos, ret, cam->pos);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_cross(cam->front, cam->up, ret);
	glm_vec3_normalize(ret);
	glm_vec3_scale(ret, MOVEMENT_SPEED, ret);
	glm_vec3_sub(cam->pos, ret, cam->pos);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(cam->up, MOVEMENT_SPEED, ret);
	glm_vec3_add(cam->pos, ret, cam->pos);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
	vec3 ret = { 0 };
	glm_vec3_scale(cam->up, MOVEMENT_SPEED, ret);
	glm_vec3_sub(cam->pos, ret, cam->pos);
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){ 
	//object_count = new_floppa(object_root, object_count, main_cam); 
	object_count = new_floppa(object_root, object_count, main_cam->pos);
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){ 
	pop_floppa(object_root, main_cam->pos, object_count);
    }
} 


bool firstMouse = true;
float lastX = 400, lastY = 300;

void mouse_callback(GLFWwindow * window, double xpos, double ypos) //todo: descobrir como isso funciona
{ 
    //if (firstMouse)
    //{
    //    lastX = xpos;
    //    lastY = ypos;
    //    firstMouse = false;
    //}

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    main_cam->yaw += xoffset;
    main_cam->pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (main_cam->pitch > 89.0f)
	main_cam->pitch = 89.0f;
    if (main_cam->pitch < -89.0f)
	main_cam->pitch = -89.0f;

    vec3 front;
    front[X] = cos(glm_rad(main_cam->yaw)) * cos(glm_rad(main_cam->pitch));
    front[Y] = sin(glm_rad(main_cam->pitch));
    front[Z] = sin(glm_rad(main_cam->yaw)) * cos(glm_rad(main_cam->pitch));
    glm_vec3_normalize(front); glm_vec3_copy(front, main_cam->front);

}
void scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
    main_cam->fov -= yoffset * 2.0;
    //if (fov < FOV_MIN) fov = FOV_MIN;
    //if (fov > FOV_MAX) fov = FOV_MAX;
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

    //um shader é uma parte de um processo da pipeline de renderização do opengl
    //
    //todo shader é compilado em run-time e precisa ser passado como string para a GPU
    unsigned int shader_process; //todo shader tambem é um objecto que precisa de uma identificação (referenia/handle)
    shader_process = glCreateShader(target_shader); //criar o objeto 

    glShaderSource(shader_process, 1, &shader_source, NULL); //especificar o código fonte a ser compilado
    glCompileShader(shader_process); //compilar o código fonte do shader

    int success;
    char infolog[521]; //buffer onde erro sera logado
    glGetShaderiv(shader_process, GL_COMPILE_STATUS, &success); //retornar status da compilação
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


unsigned int init_vertex_array()
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


void rand_floppa(struct object ** root, size_t *count, const int MAX_FLOP, const int RANGE)
{
    int i = 0;
    srand(time(NULL));

    while (i < MAX_FLOP) {
	int rand_x = rand() % RANGE; 
	int rand_y = rand() % RANGE; 
	int rand_z = rand() % RANGE; 
	vec3 rand_pos = {rand_x, rand_y, rand_z};

	*count = new_floppa(root, *count, rand_pos);
	++i;
    } 
}


float sier_side(float h){ return (2 * h) / sqrt(3); }
float inscrib_circle(float s){ return (sqrt(3) / 6) * s; }
float cirscum_circle(float s){ return s / sqrt(3); }

void recurse_sier(vec3 * pos_vectors, size_t * vector_count, vec3 peak, float height, int level)
{
    if (level == 0 ) return;

    float half_s = sier_side(height / 2);

    vec3 front_peak = 	{	      peak[X], 	peak[Y] - height / 2, peak[Z] - cirscum_circle(half_s)};
    vec3 left_peak = 	{half_s / 2 + peak[X], 	peak[Y] - height / 2, peak[Z] + inscrib_circle(half_s)};
    vec3 right_peak = 	{half_s / 2 - peak[X], 	peak[Y] - height / 2, peak[Z] + inscrib_circle(half_s)};;

    recurse_sier(pos_vectors, vector_count, left_peak, 		height / 2, level - 1);
    recurse_sier(pos_vectors, vector_count, right_peak, 	height / 2, level - 1);
    recurse_sier(pos_vectors, vector_count, front_peak, 	height / 2, level - 1);
    recurse_sier(pos_vectors, vector_count, peak, 		height / 2, level - 1); 

    memcpy(pos_vectors[*vector_count], peak, sizeof(vec3));
    *vector_count += 1;
    return; 
} 

//struct object * 

int main(void)
{ 
    //glfw fluff 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
    

    GLFWwindow * window = glfwCreateWindow(500, 500, "teste", NULL, NULL);
    if (window == NULL) {
	glfwTerminate();
	return -1;
    } 
    glfwMakeContextCurrent(window);

    if( !gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) )
    {
	return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); 
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfw fluff 

    //inicializacao
    glEnable(GL_DEPTH_TEST); 

    unsigned int VAO = 			init_vertex_array();
    		 shader_program = 	update_shader_program("shader.vert", "shader.frag"); 
    unsigned int texture1 = 		load_texture("floppa.jpg"); 
    		 main_cam = 		new_camera();
    
    glUseProgram(shader_program); 

    object_count = 0;
    object_root = calloc(5000, sizeof(struct object *));

//#define AUTO_LOAD

#ifdef AUTO_LOAD
    if (fopen(DUMP_FILE, "r") != NULL)
    {
	load_floppas(object_root, &object_count, main_cam);
    }
#endif
#ifndef AUTO_LOAD
    //rand_floppa(object_root, &object_count, 1000, 20);

    int levels = 6;
    vec3 * pos_vectors = malloc(sizeof(vec3) * (int) pow(4, levels));
    size_t vector_count = 0;


    recurse_sier(pos_vectors, &vector_count, (vec3) {0, 0, 0}, 50, levels); 

    printf("%ld\n", vector_count);
    for ( size_t i = 0; i < vector_count; ++i ){
	printf("%f, %f, %f\n", pos_vectors[i][X], pos_vectors[i][Y], pos_vectors[i][Z]);
    } 
#endif
    //inicializacao


    while(!glfwWindowShouldClose(window))
    {
	update_delta_time(); 
	glClearColor(	(glfwGetTime() + 10) * 30, (glfwGetTime() + 15) * 30, (glfwGetTime() + 20) * 30, 1); 
	glClear(	GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	process_input(	window, main_cam); 
	glUseProgram(	shader_program); 
	glfwGetWindowSize(window, &screen_x, &screen_y);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(	GL_TEXTURE_2D, texture1);
	glUniform1i(glGetUniformLocation(shader_program, "texture1"), 1); 

	mat4 model = { 0 }; 		glm_mat4_identity(model);
	mat4 projection = { 0 }; 	glm_mat4_identity(projection); 
	mat4 view = { 0 }; 		glm_mat4_identity(view); 

	glm_perspective(glm_rad(main_cam->fov), (float) screen_x / screen_y, 0.01, 400, projection);
	//glm_ortho(		-1.0f, 1, -1, 1, -1, 1, projection); 

	vec3 camera_target; 
	glm_vec3_add(	main_cam->pos, main_cam->front, camera_target); 
	glm_lookat(	main_cam->pos, camera_target, 	main_cam->up, view);

	unsigned int viewloc = 	glGetUniformLocation(shader_program, "view");
	unsigned int projloc = 	glGetUniformLocation(shader_program, "projection"); 

	glUniformMatrix4fv(viewloc, 1, GL_FALSE, (float *) view); 
	glUniformMatrix4fv(projloc, 1, GL_FALSE, (float *) projection); 

	glBindVertexArray(VAO); 
	{ 
	    for (unsigned int i = 0; i < object_count; ++i)
	    {
	        if (object_root[i] != NULL)
	        { 
	            #define floppa_pos pos_vectors[i] 
	            #define relative_pos floppa_pos[X] / 20 , floppa_pos[Y] / 20, floppa_pos[Z] / 20
	    
	            glm_mat4_identity(model);
	            glm_translate(model, floppa_pos); 
	            glUniform3f(glGetUniformLocation(shader_program, "relative_color"),  relative_pos);
	            glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, (float *) model); 
	            glDrawArrays(GL_TRIANGLES, 0, 36);
	        }
	    }
	    for (size_t i = 0; i < vector_count; ++i)
	    {
		glm_mat4_identity(model);
		glm_translate(model, pos_vectors[i]);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, (float *) model); 
		glDrawArrays(GL_TRIANGLES, 0, 36); 
	    }
	}
	glm_mat4_identity(model);
	glm_translate(model, (vec3) { round(main_cam->pos[X]), round(main_cam->pos[Y]), round(main_cam->pos[Z])});
	glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, (float *) model); 
	glDrawArrays(GL_LINE_STRIP, 0, 36); 
	


	glfwSwapBuffers(window);
	glfwPollEvents();
    }

    terminate_program();

}

void terminate_program()
{
#ifdef AUTO_LOAD
    dump_floppas(object_root, object_count, main_cam); 
    free_floppas(object_root, &object_count);
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
