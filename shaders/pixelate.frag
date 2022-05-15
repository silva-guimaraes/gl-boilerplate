#version 330 core 

in vec2 TexCoords; 
out vec4 FragColor;

uniform sampler2D screen_texture; 
uniform vec2 resolution;

void main()
{
    float Pixels = 512.0 * 8;
    float dx = 15.0 * (1.0 / Pixels);
    float dy = 10.0 * (1.0 / Pixels);
    vec2 Coord = vec2(	dx * floor(TexCoords.x / dx),
	    		dy * floor(TexCoords.y / dy));
    FragColor = texture(screen_texture, Coord);
}
