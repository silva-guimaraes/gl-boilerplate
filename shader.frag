#version 330 core
in vec2 TexCoord;

out vec4 FragColor; 

uniform sampler2D texture1;
uniform float time;
uniform vec3 relative_color;
//uniform sampler2D texture2;

void main()
{
    //vec4 texture_sample = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 1.0);

    
    //FragColor = texture(texture1, TexCoord) * vec4(1.0, 1.0, 1.0, 1.0);
    FragColor = vec4(1, 1, 0, 1.0);
}
