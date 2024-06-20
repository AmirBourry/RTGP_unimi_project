#version 410 core

uniform sampler2D texture_diffuse1;

uniform bool texture_diffuse1_present;

uniform struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
} material;

uniform struct AmbientLight {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} ambient;

uniform struct Light {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} lights[25];


in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoords;
flat in uint lightId;

uniform uint backrooms;

uniform uint debugLightId;

uniform vec3 vEyePos;
uniform vec3 vEyeDir;

uniform float ceilingFlicker;

const float gamma = 2.2;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

vec3 getDiffuse() {
    vec3 matDiffuse;
    if (texture_diffuse1_present)
        matDiffuse = texture2D(texture_diffuse1, vTexCoords).rgb;
    else
        matDiffuse = vec3(material.diffuse);
    vec3 diffuseColor = pow(matDiffuse, vec3(gamma));
    return diffuseColor;
    //return matDiffuse;
}

vec3 getSpecular() {
    return pow(material.specular, vec3(gamma));
    //return material.specular;
}

vec3 calcAmbient(AmbientLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = vec3(0.0, -1, 0);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    const float shininess = 0.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // combine results
    vec3 ambient = light.ambient * getDiffuse();
    vec3 diffuse = light.diffuse * diff * getDiffuse();
    vec3 specular = light.specular * spec * getSpecular();
    return (ambient + diffuse + specular);
}

vec3 calcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    /*vec3 lightDir = normalize(light.position - vWorldPos);
    float dotNL = max(dot(vNormal, lightDir), 0.0);

    vec3 view = normalize(vEyePos - vWorldPos);
    //lightDir = -vEyeDir;
    vec3 halfway = normalize(view + lightDir);
    float lightDist = length(light.position - vWorldPos);
    float dotNH = max(dot(vNormal, halfway), 0.0);



    // attenuation value to cover a distance of 32
    // see https://learnopengl.com/Lighting/Light-casters
    float atten = 1.0 / (1.0 + 0.14 * lightDist + 0.07 * lightDist * lightDist);
    vec3 Lambient = light.ambient * getDiffuse();
    vec3 Ldiffuse = light.diffuse * getDiffuse() * dotNL * atten;
    const float shininess = 0.0;
    vec3 Lspecular = light.specular * getSpecular() * pow(dotNH, shininess) * atten;
    vec3 Lo = Lambient + Ldiffuse + Lspecular;*/


    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    const float shininess = 0.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * getDiffuse();
    vec3 diffuse = light.diffuse * diff * getDiffuse();
    vec3 specular = light.specular * spec * getSpecular();
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);


}

void main(void)
{
    vec3 result = calcAmbient(ambient, vNormal, vEyeDir);
    result = vec3(0.0);
    for (int i = 0; i < 25; i++)
    {
//        if (i != debugLightId)
//        continue;
        result += calcPointLight(lights[i], abs(vNormal), vWorldPos, vEyeDir);
    }

    if (backrooms == 1) {
        //if (lightId == debugLightId)
        result += (material.emissive * (lights[lightId].ambient.x * 2.0 / 0.05) * ceilingFlicker);

        float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0)
            BrightColor = vec4(result, 1.0);
        else
            BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        FragColor = vec4(result, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        FragColor = vec4(result, 1.0);
    }
}