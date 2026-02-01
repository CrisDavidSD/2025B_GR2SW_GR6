#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    mat4 boneTransform = mat4(0.0);
    boneTransform += finalBonesMatrices[aBoneIDs[0]] * aWeights[0];
    boneTransform += finalBonesMatrices[aBoneIDs[1]] * aWeights[1];
    boneTransform += finalBonesMatrices[aBoneIDs[2]] * aWeights[2];
    boneTransform += finalBonesMatrices[aBoneIDs[3]] * aWeights[3];

    vec4 localPos = boneTransform * vec4(aPos, 1.0);
    gl_Position = projection * view * model * localPos;

    TexCoords = aTexCoords;
}
