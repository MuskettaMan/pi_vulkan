#pragma once

#include <array>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "vk_mem_alloc.h"
#include <memory>
#include <optional>

struct Vertex
{
    enum Enumeration {
        ePOSITION,
        eNORMAL,
        eTANGENT,
        eCOLOR,
        eTEX_COORD
    };

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription GetBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 5> GetAttributeDescriptions();
};

struct MeshPrimitive
{
    vk::PrimitiveTopology topology;

    vk::IndexType indexType;
    std::vector<std::byte> indicesBytes;
    std::vector<Vertex> vertices;

    std::optional<uint32_t> materialIndex;
};

struct Mesh
{
    std::vector<MeshPrimitive> primitives;
};

struct Texture
{
    uint32_t width, height, numChannels;
    std::vector<std::byte> data;

    vk::Format GetFormat() const
    {
        return vk::Format::eR8G8B8A8Srgb;
    }
};

struct Material
{
    std::optional<uint32_t> albedoIndex;
    glm::vec4 albedoFactor;
    uint32_t albedoUVChannel;

    std::optional<uint32_t> metallicRoughnessIndex;
    float metallicFactor;
    float roughnessFactor;
    std::optional<uint32_t> metallicRoughnessUVChannel;

    std::optional<uint32_t> normalIndex;
    float normalScale;
    uint32_t normalUVChannel;

    std::optional<uint32_t> occlusionIndex;
    float occlusionStrength;
    uint32_t occlusionUVChannel;

    std::optional<uint32_t> emissiveIndex;
    glm::vec3 emissiveFactor;
    uint32_t emissiveUVChannel;
};

struct TextureHandle
{
    std::string name;
    vk::Image image;
    VmaAllocation imageAllocation;
    vk::ImageView imageView;
    uint32_t width, height, numChannels;
    vk::Format format;
};

struct MaterialHandle
{
    struct MaterialInfo
    {
        alignas(16)
        glm::vec4 albedoFactor{0.0f};

        alignas(16)
        float metallicFactor{0.0f};
        float roughnessFactor{0.0f};
        float normalScale{0.0f};
        float occlusionStrength{0.0f};

        alignas(16)
        bool useAlbedoMap{false};
        bool useMRMap{false};
        bool useNormalMap{false};
        bool useOcclusionMap{false};

        alignas(16)
        glm::vec3 emissiveFactor{0.0f};
        bool useEmissiveMap{false};
    };

    const static uint32_t TEXTURE_COUNT = 5;

    vk::DescriptorSet descriptorSet;
    vk::Buffer materialUniformBuffer;
    VmaAllocation materialUniformAllocation;


    std::array<std::shared_ptr<TextureHandle>, TEXTURE_COUNT> textures;

    static std::array<vk::DescriptorSetLayoutBinding, 7> GetLayoutBindings()
    {
        std::array<vk::DescriptorSetLayoutBinding, 7> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorType = vk::DescriptorType::eSampler;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = vk::ShaderStageFlagBits::eFragment;
        for(size_t i = 1; i < TEXTURE_COUNT + 1; ++i)
        {
            bindings[i].binding = i;
            bindings[i].descriptorType = vk::DescriptorType::eSampledImage;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = vk::ShaderStageFlagBits::eFragment;
        }

        const uint32_t infoUniformIndex = TEXTURE_COUNT + 1;
        bindings[infoUniformIndex].binding = infoUniformIndex;
        bindings[infoUniformIndex].descriptorType = vk::DescriptorType::eUniformBuffer;
        bindings[infoUniformIndex].descriptorCount = 1;
        bindings[infoUniformIndex].stageFlags = vk::ShaderStageFlagBits::eFragment;

        return bindings;
    }
};

struct MeshPrimitiveHandle
{
    vk::PrimitiveTopology topology;
    vk::IndexType indexType;
    uint32_t indexCount;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
    VmaAllocation vertexBufferAllocation;
    VmaAllocation indexBufferAllocation;

    std::shared_ptr<MaterialHandle> material;
};

struct MeshHandle
{
    std::vector<MeshPrimitiveHandle> primitives;
};

struct Hierarchy
{
    struct Node
    {
        glm::mat4 transform;
        std::shared_ptr<MeshHandle> mesh;
    };

    std::vector<Node> allNodes;
};

struct ModelHandle
{
    std::vector<std::shared_ptr<MeshHandle>> meshes;
    std::vector<std::shared_ptr<MaterialHandle>> materials;
    std::vector<std::shared_ptr<TextureHandle>> textures;

    Hierarchy hierarchy;
};
