#version 330 core 

in vec2 TexCoords; 
out vec4 FragColor;

uniform sampler2D screen_texture; 
uniform vec2 resolution;

void main()
{ 
    vec3 col = texture(screen_texture, TexCoords).rgb;
    FragColor = vec4(col, 1.0);
}
