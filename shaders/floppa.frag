#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor; 

uniform sampler2D texture1; 
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos; 

struct lighting {
    float ambient_strength;
    float specular_strength;
}; 
uniform lighting light;

void main()
{
    //float ambient_strength = 0.1;
    //float specular_strength = 0.5;

    //ambient
    vec3 ambient = light.ambient_strength * lightColor;
    // 
    //diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor; 
    // 
    //specular
    vec3 viewDir = normalize (viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm); 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3 specular = light.specular_strength * spec * lightColor;
    //
    
    vec3 result = (ambient + diffuse + specular) * vec3(texture(texture1, TexCoord));
    FragColor = vec4(result, 1.0);

    FragColor.rgb = Normal;
    FragColor.a = 1.0f;

    //FragColor = vec4(objectColor * lightColor, 1.0);
}
