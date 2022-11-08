#include "vulkan_base/vulkan_base.h"

struct Model {
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    uint64_t numIndices;
    VulkanImage albedoTexture;
};

Model createModel(VulkanContext* context, const char* filename);
void destroyModel(VulkanContext* context, Model* model);