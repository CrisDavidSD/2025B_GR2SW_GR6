#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <cmath>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <algorithm>

#include <learnopengl/animation.h>

class Animator
{
public:
    Animator(Animation* animation)
    {
        m_CurrentTime = 0.0f;
        m_CurrentAnimation = animation;
        ResizeFinalBoneMatrices();  // clave
    }

    void UpdateAnimation(float dt)
    {
        if (!m_CurrentAnimation) return;

        float ticksPerSecond = m_CurrentAnimation->GetTicksPerSecond();
        float duration = m_CurrentAnimation->GetDuration();

        m_CurrentTime += ticksPerSecond * dt;
        if (duration > 0.0f)
            m_CurrentTime = fmod(m_CurrentTime, duration);

        // por si cambió animación/modelo
        ResizeFinalBoneMatrices();

        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
    }

    void PlayAnimation(Animation* animation)
    {
        m_CurrentAnimation = animation;
        m_CurrentTime = 0.0f;
        ResizeFinalBoneMatrices(); // clave
    }

    const std::vector<glm::mat4>& GetFinalBoneMatrices() const
    {
        return m_FinalBoneMatrices;
    }

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation = nullptr;
    float m_CurrentTime = 0.0f;

    void ResizeFinalBoneMatrices()
    {
        if (!m_CurrentAnimation) return;

        //  usa el boneCount REAL del modelo/animación
        int boneCount = std::max(1, m_CurrentAnimation->GetBoneCount());

        if ((int)m_FinalBoneMatrices.size() != boneCount)
        {
            m_FinalBoneMatrices.assign(boneCount, glm::mat4(1.0f));
        }
    }

    void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform)
    {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        Bone* bone = m_CurrentAnimation->FindBone(nodeName);
        if (bone)
        {
            bone->Update(m_CurrentTime);
            nodeTransform = bone->GetLocalTransform();
        }

        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        const auto& boneInfoMap = m_CurrentAnimation->GetBoneInfoMap();
        auto it = boneInfoMap.find(nodeName);
        if (it != boneInfoMap.end())
        {
            int index = it->second.id;
            const glm::mat4& offset = it->second.offset;

            if (index >= 0 && index < (int)m_FinalBoneMatrices.size())
            {
                const glm::mat4& invRoot = m_CurrentAnimation->GetGlobalInverseTransform();
                m_FinalBoneMatrices[index] = invRoot * globalTransformation * offset;

            }
        }

        for (int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransform(&node->children[i], globalTransformation);
    }
};

#endif
