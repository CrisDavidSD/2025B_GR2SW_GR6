//#ifndef ANIMATION_H
//#define ANIMATION_H
//
//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//
//#include <glm/glm.hpp>
//#include <string>
//#include <vector>
//#include <map>
//#include <iostream>
//
//#include <learnopengl/model.h>
//#include <learnopengl/bone.h>
//
//struct AssimpNodeData
//{
//    glm::mat4 transformation{ 1.0f };
//    std::string name;
//    int childrenCount = 0;
//    std::vector<AssimpNodeData> children;
//};
//
//class Animation
//{
//public:
//    Animation() = default;
//
//    Animation(const std::string& animationPath, Model* model)
//    {
//        Assimp::Importer importer;
//
//        const aiScene* scene = importer.ReadFile(animationPath,
//            aiProcess_Triangulate |
//            aiProcess_GenSmoothNormals |
//            aiProcess_FlipUVs |
//            aiProcess_CalcTangentSpace
//        );
//
//        if (!scene || !scene->mRootNode)
//        {
//            std::cout << "ERROR::ANIMATION::ASSIMP: " << importer.GetErrorString() << "\n";
//            return;
//        }
//
//        if (scene->mNumAnimations == 0)
//        {
//            std::cout << "WARNING: El archivo NO tiene animaciones: " << animationPath << "\n";
//            return;
//        }
//
//        aiAnimation* animation = scene->mAnimations[0]; // primera animación
//        m_Duration = (float)animation->mDuration;
//        m_TicksPerSecond = (float)(animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
//
//        ReadHeirarchyData(m_RootNode, scene->mRootNode);
//
//        // Copiamos mapa de huesos del modelo (ya extraído al cargar mallas)
//        m_BoneInfoMap = model->GetBoneInfoMap();
//        m_BoneCount = model->GetBoneCount();
//
//        ReadMissingBones(animation, *model);
//    }
//
//    Bone* FindBone(const std::string& name)
//    {
//        for (auto& bone : m_Bones)
//        {
//            if (bone.GetBoneName() == name)
//                return &bone;
//        }
//        return nullptr;
//    }
//
//    float GetTicksPerSecond() const { return m_TicksPerSecond; }
//    float GetDuration() const { return m_Duration; }
//    const AssimpNodeData& GetRootNode() const { return m_RootNode; }
//
//    const std::map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_BoneInfoMap; }
//    int GetBoneCount() const { return m_BoneCount; }
//
//private:
//    float m_Duration = 0.0f;
//    float m_TicksPerSecond = 25.0f;
//    std::vector<Bone> m_Bones;
//    AssimpNodeData m_RootNode;
//
//    std::map<std::string, BoneInfo> m_BoneInfoMap;
//    int m_BoneCount = 0;
//
//    glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
//    {
//        glm::mat4 to;
//        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
//        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
//        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
//        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
//        return to;
//    }
//
//    void ReadMissingBones(const aiAnimation* animation, Model& model)
//    {
//        auto& boneInfoMap = model.GetBoneInfoMap();
//        int& boneCount = model.GetBoneCount();
//
//        int size = (int)animation->mNumChannels;
//        m_Bones.reserve(size);
//
//        for (int i = 0; i < size; i++)
//        {
//            aiNodeAnim* channel = animation->mChannels[i];
//            std::string boneName = channel->mNodeName.C_Str();
//
//            if (boneInfoMap.find(boneName) == boneInfoMap.end())
//            {
//                BoneInfo newBoneInfo;
//                newBoneInfo.id = boneCount;
//                newBoneInfo.offset = glm::mat4(1.0f);
//                boneInfoMap[boneName] = newBoneInfo;
//                boneCount++;
//            }
//
//            m_Bones.emplace_back(boneName, boneInfoMap[boneName].id, channel);
//        }
//
//        // actualizamos cache local
//        m_BoneInfoMap = boneInfoMap;
//        m_BoneCount = boneCount;
//    }
//
//    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
//    {
//        dest.name = src->mName.C_Str();
//        dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
//        dest.childrenCount = (int)src->mNumChildren;
//
//        dest.children.clear();
//        dest.children.reserve(src->mNumChildren);
//
//        for (unsigned int i = 0; i < src->mNumChildren; i++)
//        {
//            AssimpNodeData newData;
//            ReadHeirarchyData(newData, src->mChildren[i]);
//            dest.children.push_back(newData);
//        }
//    }
//};
//
//#endif

#ifndef ANIMATION_H
#define ANIMATION_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include <learnopengl/model.h>
#include <learnopengl/bone.h>

struct AssimpNodeData
{
    glm::mat4 transformation{ 1.0f };
    std::string name;
    int childrenCount = 0;
    std::vector<AssimpNodeData> children;
};

class Animation
{
public:
    Animation() = default;

    Animation(const std::string& animationPath, Model* model)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(animationPath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace
        );

        if (!scene || !scene->mRootNode)
        {
            std::cout << "ERROR::ANIMATION::ASSIMP: " << importer.GetErrorString() << "\n";
            return;
        }

        if (scene->mNumAnimations == 0)
        {
            std::cout << "WARNING: El archivo NO tiene animaciones: " << animationPath << "\n";
            return;
        }

        // ✅ IMPORTANTÍSIMO: inversa del root (arregla el “pincho”)
        glm::mat4 rootTransform = ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation);
        m_GlobalInverseTransform = glm::inverse(rootTransform);

        aiAnimation* animation = scene->mAnimations[0];
        m_Duration = (float)animation->mDuration;
        m_TicksPerSecond = (float)(animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);

        ReadHeirarchyData(m_RootNode, scene->mRootNode);

        m_BoneInfoMap = model->GetBoneInfoMap();
        m_BoneCount = model->GetBoneCount();

        ReadMissingBones(animation, *model);
    }

    Bone* FindBone(const std::string& name)
    {
        for (auto& bone : m_Bones)
        {
            if (bone.GetBoneName() == name)
                return &bone;
        }
        return nullptr;
    }

    float GetTicksPerSecond() const { return m_TicksPerSecond; }
    float GetDuration() const { return m_Duration; }
    const AssimpNodeData& GetRootNode() const { return m_RootNode; }

    const std::map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_BoneInfoMap; }
    int GetBoneCount() const { return m_BoneCount; }

    // ✅ getter
    const glm::mat4& GetGlobalInverseTransform() const { return m_GlobalInverseTransform; }

private:
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 25.0f;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;

    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCount = 0;

    // ✅ NUEVO
    glm::mat4 m_GlobalInverseTransform{ 1.0f };

    glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
    {
        //glm::mat4 to;
        //to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        //to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        //to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        //to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return glm::transpose(glm::make_mat4(&from.a1));
    }

    void ReadMissingBones(const aiAnimation* animation, Model& model)
    {
        auto& boneInfoMap = model.GetBoneInfoMap();
        int& boneCount = model.GetBoneCount();

        int size = (int)animation->mNumChannels;
        m_Bones.reserve(size);

        for (int i = 0; i < size; i++)
        {
            aiNodeAnim* channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.C_Str();

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = boneCount;
                newBoneInfo.offset = glm::mat4(1.0f);
                boneInfoMap[boneName] = newBoneInfo;
                boneCount++;
            }

            m_Bones.emplace_back(boneName, boneInfoMap[boneName].id, channel);
        }

        m_BoneInfoMap = boneInfoMap;
        m_BoneCount = boneCount;
    }

    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        dest.name = src->mName.C_Str();
        dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = (int)src->mNumChildren;

        dest.children.clear();
        dest.children.reserve(src->mNumChildren);

        for (unsigned int i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHeirarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }
};

#endif



