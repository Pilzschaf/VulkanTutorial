#include "model.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

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
            assert(data->meshes[0].primitives[0].attributes[0].type == cgltf_attribute_type_position);
            assert(data->meshes[0].primitives[0].attributes[0].data->stride == sizeof(float)*3);
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
            bufferBase = (uint8_t*)data->meshes[0].primitives[0].attributes[0].data->buffer_view->buffer->data;
            uint64_t vertexDataSize = data->meshes[0].primitives[0].attributes[0].data->buffer_view->size;
            void* vertexData = bufferBase + data->meshes[0].primitives[0].attributes[0].data->buffer_view->offset;

            createBuffer(context, &result.vertexBuffer, vertexDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            uploadDataToBuffer(context, &result.vertexBuffer, vertexData, vertexDataSize);
        }
        cgltf_free(data);
    }

    return result;
}

void destroyModel(VulkanContext* context, Model* model) {
    destroyBuffer(context, &model->vertexBuffer);
    destroyBuffer(context, &model->indexBuffer);
    *model = {};
}