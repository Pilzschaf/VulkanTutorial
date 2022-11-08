#include "model.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <stb/stb_image.h>

// Stride in bytes. Element size in bytes
void fillBuffer(uint32_t inputStride, void* inputData, uint32_t outputStride, void* outputData, uint32_t numElements, uint32_t elementSize) {
    uint8_t* output = (uint8_t*)outputData;
    uint8_t* input = (uint8_t*)inputData;
    for(uint32_t i = 0; i < numElements; ++i) {
        for(uint32_t b = 0; b < elementSize; ++b) {
            output[b] = input[b];
        }
        output += outputStride;
        input += inputStride;
    }
}

Model createModel(VulkanContext* context, const char* filename) {
    Model result = {};
    cgltf_options options = {};
    cgltf_data* data = 0;
    cgltf_result error = cgltf_parse_file(&options, filename, &data);
    if(error == cgltf_result_success) {
        error = cgltf_load_buffers(&options, data, "../data/models");
        if(error == cgltf_result_success) {
            assert(data->meshes_count == 1);
            assert(data->meshes[0].primitives_count == 1);
            assert(data->meshes[0].primitives[0].attributes_count > 0);
            assert(data->meshes[0].primitives[0].indices->component_type == cgltf_component_type_r_16u);
            assert(data->meshes[0].primitives[0].indices->stride == sizeof(uint16_t));

            // Indices
            uint8_t* bufferBase = (uint8_t*)data->meshes[0].primitives[0].indices->buffer_view->buffer->data;
            uint64_t indexDataSize = data->meshes[0].primitives[0].indices->buffer_view->size;
            void* indexData = bufferBase + data->meshes[0].primitives[0].indices->buffer_view->offset;

            createBuffer(context, &result.indexBuffer, indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            uploadDataToBuffer(context, &result.indexBuffer, indexData, indexDataSize);
            result.numIndices = data->meshes[0].primitives[0].indices->count;


            // Vertices
            uint64_t outputStride = sizeof(float)*8;
            uint64_t numVertices = data->meshes[0].primitives[0].attributes->data->count;
            uint64_t vertexDataSize = outputStride * numVertices;
            uint8_t* vertexData = new uint8_t[vertexDataSize];
            for(uint64_t i = 0; i < data->meshes[0].primitives[0].attributes_count; ++i) {
                cgltf_attribute* attribute = data->meshes[0].primitives[0].attributes + i;
                bufferBase = (uint8_t*)attribute->data->buffer_view->buffer->data;
                uint64_t inputStride = attribute->data->stride;
                if(attribute->type == cgltf_attribute_type_position) {
                    void* positionData = bufferBase + attribute->data->buffer_view->offset;
                    fillBuffer(inputStride, positionData, outputStride, vertexData, numVertices, sizeof(float)*3);
                } else if(attribute->type == cgltf_attribute_type_normal) {
                    void* normalData = bufferBase + attribute->data->buffer_view->offset;
                    fillBuffer(inputStride, normalData, outputStride, vertexData+(sizeof(float)*3), numVertices, sizeof(float)*3);
                } else if(attribute->type == cgltf_attribute_type_texcoord) {
                    void* texcoordData = bufferBase + attribute->data->buffer_view->offset;
                    fillBuffer(inputStride, texcoordData, outputStride, vertexData+(sizeof(float)*6), numVertices, sizeof(float)*2);
                }
            }
            createBuffer(context, &result.vertexBuffer, vertexDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            uploadDataToBuffer(context, &result.vertexBuffer, vertexData, vertexDataSize);
            delete[] vertexData;

            // Material
            assert(data->materials_count == 1);
            cgltf_material* material = &data->materials[0];
            assert(material->has_pbr_metallic_roughness);
            cgltf_texture_view albedoTextureView = material->pbr_metallic_roughness.base_color_texture;
            assert(!albedoTextureView.has_transform);
            assert(albedoTextureView.texcoord == 0);
            assert(albedoTextureView.texture);
            cgltf_texture* albedoTexture = albedoTextureView.texture;

            // Load texture
            cgltf_buffer_view* bufferView = albedoTexture->image->buffer_view;
            assert(bufferView->size < INT32_MAX);
            int bpp, width, height;
            uint8_t* textureData = stbi_load_from_memory((stbi_uc*) bufferView->buffer->data, (int)bufferView->size, &width, &height, &bpp, 4);
            assert(textureData);
            bpp = 4;
            createImage(context, &result.albedoTexture, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            uploadDataToImage(context, &result.albedoTexture, textureData, width * height * bpp, width, height, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
            stbi_image_free(textureData);
        } else {
            LOG_ERROR("Could not load additional model buffers");
        }
        cgltf_free(data);
    } else {
        LOG_ERROR("Could not load model file");
    }

    return result;
}

void destroyModel(VulkanContext* context, Model* model) {
    destroyBuffer(context, &model->vertexBuffer);
    destroyBuffer(context, &model->indexBuffer);
    destroyImage(context, &model->albedoTexture);
    *model = {};
}