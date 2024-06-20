/*
09_illumination_models.vert: Vertex shader for the Lambert, Phong, Blinn-Phong and GGX illumination models

N. B.) the shader considers a simplified situation, with a single point light.
For more point lights, a for cycle is needed to sum the contribution of each light
For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2023/2024
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// vertex normal in world coordinate
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat3 normalMatrix;



out vec3 vWorldPos;

// the transformed normal (in view coordinate) is set as an output variable, to be "passed" to the fragment shader
// this means that the normal values in each vertex will be interpolated on each fragment created during rasterization between two vertices
out vec3 vNormal;

out vec2 vTexCoords;
flat out uint lightId;



void main(){

    // vertex position in ModelView coordinate (see the last line for the application of projection)
    // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
    vec4 mvPosition = viewMatrix * modelMatrix * vec4( position, 1.0 );

    vWorldPos = vec3(modelMatrix * vec4(position, 1.0));


    // transformations are applied to the normal
    //vNormal = normalize( normalMatrix * normal );
    //vNormal = normal;
    vNormal = (mat3(transpose(inverse(modelMatrix))) * normal).zyx;

    // we apply the projection transformation
    gl_Position = projectionMatrix * mvPosition;

    // we pass the texture coordinates to the fragment shader
    vTexCoords = texCoord;

    lightId = gl_VertexID / 6u;

}
