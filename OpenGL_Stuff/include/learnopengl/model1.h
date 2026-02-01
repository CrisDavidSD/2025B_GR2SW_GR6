#ifndef MODEL_H
#define MODEL_H

#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <cstring>

using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};

class Model
{
public:
    vector<Texture> textures_loaded;
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    // --- SKINNING DATA ---


    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

    //map<string, BoneInfo> m_BoneInfoMap;
    //int m_BoneCounter = 0;

    Model(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    void Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    map<string, BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }

private:
    void SetVertexBoneDataToDefault(Vertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
    {
        // ignora basura
        if (weight <= 0.000001f)
            return;
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (vertex.m_BoneIDs[i] == boneID)
            {
                if (weight > vertex.m_Weights[i])
                    vertex.m_Weights[i] = weight;
                return;
            }
        }
               /* vertex.m_BoneIDs[i] = boneID;
                vertex.m_Weights[i] = weight;*/



                // 1) si hay slot libre, usarlo
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (vertex.m_BoneIDs[i] == -1)
            {
                vertex.m_BoneIDs[i] = boneID;
                vertex.m_Weights[i] = weight;
                return;
            }
        }



        // 2) si no hay slot libre, reemplazar el menor si el nuevo es mayor
        int minIndex = 0;
        for (int i = 1; i < MAX_BONE_INFLUENCE; i++)
        {
            if (vertex.m_Weights[i] < vertex.m_Weights[minIndex])
                minIndex = i;
        }

        if (weight > vertex.m_Weights[minIndex])
        {
            vertex.m_BoneIDs[minIndex] = boneID;
            vertex.m_Weights[minIndex] = weight;
        }

    }

    glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
    {
        //glm::mat4 to;
        //to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        //to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        //to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        //to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return glm::transpose(glm::make_mat4(&from.a1));;
    }


    void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);

                m_BoneInfoMap[boneName] = newBoneInfo;

                boneID = m_BoneCounter;
                m_BoneCounter++;
            }
            else
            {
                boneID = m_BoneInfoMap[boneName].id;
            }

            aiBone* aiBonePtr = mesh->mBones[boneIndex];
            for (unsigned int weightIndex = 0; weightIndex < aiBonePtr->mNumWeights; ++weightIndex)
            {
                int vertexId = aiBonePtr->mWeights[weightIndex].mVertexId;
                float weight = aiBonePtr->mWeights[weightIndex].mWeight;

                if (vertexId >= 0 && vertexId < (int)vertices.size())
                {
                    SetVertexBoneData(vertices[vertexId], boneID, weight);
                }
            }
        }

        // ✅ ORDENAR (por peso) + NORMALIZAR (para estabilidad)
        for (auto& v : vertices)
        {
            // ordenar de mayor a menor peso
            for (int a = 0; a < MAX_BONE_INFLUENCE; a++)
            {
                for (int b = a + 1; b < MAX_BONE_INFLUENCE; b++)
                {
                    if (v.m_Weights[b] > v.m_Weights[a])
                    {
                        std::swap(v.m_Weights[a], v.m_Weights[b]);
                        std::swap(v.m_BoneIDs[a], v.m_BoneIDs[b]);
                    }
                }
            }

            // normalizar
            float sum = 0.0f;
            for (int i = 0; i < MAX_BONE_INFLUENCE; i++) sum += v.m_Weights[i];

            if (sum > 0.000001f)
            {
                for (int i = 0; i < MAX_BONE_INFLUENCE; i++) v.m_Weights[i] /= sum;
            }
            else
            {
                // si no hay pesos válidos, identidad
                v.m_BoneIDs[0] = 0;
                v.m_Weights[0] = 1.0f;
                for (int i = 1; i < MAX_BONE_INFLUENCE; i++)
                {
                    v.m_BoneIDs[i] = -1;
                    v.m_Weights[i] = 0.0f;
                }
            }
        }
    }

    //void ExtractBoneWeightForVertices(vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
    //{
    //    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    //    {
    //        string boneName = mesh->mBones[boneIndex]->mName.C_Str();

    //        int boneID = -1;
    //        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
    //        {
    //            BoneInfo newBoneInfo;
    //            newBoneInfo.id = m_BoneCounter;
    //            newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
    //            m_BoneInfoMap[boneName] = newBoneInfo;
    //            boneID = m_BoneCounter;
    //            m_BoneCounter++;
    //        }
    //        else
    //        {
    //            boneID = m_BoneInfoMap[boneName].id;
    //        }

    //        aiBone* aiBonePtr = mesh->mBones[boneIndex];
    //        for (unsigned int weightIndex = 0; weightIndex < aiBonePtr->mNumWeights; ++weightIndex)
    //        {
    //            unsigned int vertexId = aiBonePtr->mWeights[weightIndex].mVertexId;
    //            float weight = aiBonePtr->mWeights[weightIndex].mWeight;

    //            if (vertexId < vertices.size())
    //                SetVertexBoneData(vertices[vertexId], boneID, weight);
    //        }
    //    }
    //}

    void loadModel(string const& path)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }

        //Windows-safe: / or 
        directory = path.substr(0, path.find_last_of("/\\"));

        //size_t slash = path.find_last_of("/\\");
        //if (slash == string::npos)
        //{
        //    // Si te pasan solo "scene.gltf" sin ruta, usamos el directorio actual
        //    directory = ".";
        //}
        //else
        //{
        //    directory = path.substr(0, slash);
        //}









        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            SetVertexBoneDataToDefault(vertex);

            glm::vec3 vector;

            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            else
                vertex.Normal = glm::vec3(0.0f);

            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

                if (mesh->mTangents)
                {
                    vector.x = mesh->mTangents[i].x;
                    vector.y = mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.Tangent = vector;
                }
                else vertex.Tangent = glm::vec3(0.0f);

                if (mesh->mBitangents)
                {
                    vector.x = mesh->mBitangents[i].x;
                    vector.y = mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
                else vertex.Bitangent = glm::vec3(0.0f);
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f);
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Material textures
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];






        // 1) Diffuse (OBJ / FBX)
        vector<Texture> diffuseMaps = loadMaterialTextures(
            material,
            aiTextureType_DIFFUSE,
            "texture_diffuse"
        );

        // 1b) glTF BaseColor (CLAVE para .gltf / .glb)
        if (diffuseMaps.empty())
        {
            vector<Texture> baseColorMaps = loadMaterialTextures(
                material,
                aiTextureType_BASE_COLOR,
                "texture_diffuse"
            );
            diffuseMaps.insert(diffuseMaps.end(), baseColorMaps.begin(), baseColorMaps.end());
        }

        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());


        //vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");


        //textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // --- Extract bones weights ---
        ExtractBoneWeightForVertices(vertices, mesh, scene);

        std::cout << "V0 BoneIDs: "
            << vertices[0].m_BoneIDs[0] << ", "
            << vertices[0].m_BoneIDs[1] << ", "
            << vertices[0].m_BoneIDs[2] << ", "
            << vertices[0].m_BoneIDs[3] << "\n";

        std::cout << "V0 Weights: "
            << vertices[0].m_Weights[0] << ", "
            << vertices[0].m_Weights[1] << ", "
            << vertices[0].m_Weights[2] << ", "
            << vertices[0].m_Weights[3] << "\n";






        return Mesh(vertices, indices, textures);
    }

    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;

        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            // Skip embedded textures "*0" (tu loader actual no las soporta)
            const char* cpath = str.C_Str();
            if (cpath && cpath[0] == '*')
                continue;

            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }

            if (!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }

        return textures;
    }
};

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;




    std::cout << "[TextureFromFile] directory = " << directory << std::endl;
    std::cout << "[TextureFromFile] uri = " << path << std::endl;
    std::cout << "[TextureFromFile] full = " << filename << std::endl;




    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;



    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 2) format = GL_RG;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        else
        {
            std::cout << "Unsupported nrComponents = " << nrComponents
                << " for texture: " << filename << std::endl;
            stbi_image_free(data);
            return 0;
        }





        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);




        return 0;

    }

    return textureID;
}

#endif
