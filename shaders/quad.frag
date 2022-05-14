#version 330 core 

in vec2 TexCoords; 
out vec4 FragColor;

uniform sampler2D screen_texture; 

void main()
{
    vec3 col = vec3(1.0 - texture(screen_texture, TexCoords));
    FragColor = vec4(col, 1.0);
}
