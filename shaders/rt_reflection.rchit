#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

struct ReflectionPayload {
    vec3 color;
    float distance;
    vec3 hitPos;
    vec3 hitNormal;
    vec3 albedo;
    float targetLod;
};

struct ShadowPayload {
    int isShadowed;
};

layout(location = 1) rayPayloadInEXT ReflectionPayload payload;
layout(location = 0) rayPayloadEXT ShadowPayload shadowPayload;

hitAttributeEXT vec2 attribs;

layout(set = 0, binding = 2, std140) uniform RTParams {
    mat4 invViewProj;
    vec3 cameraPos;
    vec3 lightDir;
    vec3 lightColor;
    uint frameIndex;
    uint64_t objDescAddress;
} pc;

struct Vertex {
    float pos_x, pos_y, pos_z;
    float u, v;
    float normal_x, normal_y, normal_z;
    float tangent_x, tangent_y, tangent_z;
    float bitangent_x, bitangent_y, bitangent_z;
};

struct ObjDesc {
    uint64_t vertexAddress;
    uint64_t indexAddress;
    uint64_t diffuseTexIndex;
};

struct Light {
    vec4 posAndType;
    vec4 colorAndStrength;
    vec4 radiusData;
};

layout(set = 0, binding = 12, std430) readonly buffer LightBuffer {
    uint lightCount;
    Light lights[];
};

layout(set = 0, binding = 11) uniform sampler2D sceneTextures[];

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uint i[]; };
layout(buffer_reference, scalar) buffer ObjDescs { ObjDesc d[]; };

layout(buffer_reference, scalar) buffer InstanceDescArray {
    uint64_t modelDescAddresses[];
};

void main() {
    InstanceDescArray instanceArray = InstanceDescArray(pc.objDescAddress);
    uint64_t modelAddress = instanceArray.modelDescAddresses[gl_InstanceCustomIndexEXT];

    ObjDescs objDescs = ObjDescs(modelAddress);
    ObjDesc currentObj = objDescs.d[gl_GeometryIndexEXT];

    Vertices vertices = Vertices(currentObj.vertexAddress);
    Indices indices = Indices(currentObj.indexAddress);

    uint ind0 = indices.i[3 * gl_PrimitiveID + 0];
    uint ind1 = indices.i[3 * gl_PrimitiveID + 1];
    uint ind2 = indices.i[3 * gl_PrimitiveID + 2];

    Vertex v0 = vertices.v[ind0];
    Vertex v1 = vertices.v[ind1];
    Vertex v2 = vertices.v[ind2];

    vec3 n0 = vec3(v0.normal_x, v0.normal_y, v0.normal_z);
    vec3 n1 = vec3(v1.normal_x, v1.normal_y, v1.normal_z);
    vec3 n2 = vec3(v2.normal_x, v2.normal_y, v2.normal_z);

    vec2 uv0 = vec2(v0.u, v0.v);
    vec2 uv1 = vec2(v1.u, v1.v);
    vec2 uv2 = vec2(v2.u, v2.v);

    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 objNormal = normalize(n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z);
    vec3 normal = normalize(vec3(gl_ObjectToWorldEXT * vec4(objNormal, 0.0)));

    vec2 uv = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;
    vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    uint texID = uint(currentObj.diffuseTexIndex);
    vec3 albedo = texture(sceneTextures[nonuniformEXT(texID)], uv).rgb;

    vec3 directLighting = vec3(0.0);
    vec3 F0 = vec3(0.04);

    for (uint i = 0; i < lightCount; ++i) {
        Light selectedLight = lights[i];

        if (selectedLight.posAndType.w == 0.0) {
            vec3 dirLightDir = normalize(selectedLight.posAndType.xyz);
            float NdotL_dir = max(dot(normal, dirLightDir), 0.0);

            if (NdotL_dir > 0.0) {
                shadowPayload.isShadowed = 1;
                traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 0, hitPos + normal * 0.05, 0.01, dirLightDir, 1000.0, 0);
                if (shadowPayload.isShadowed == 0) {
                    directLighting += (albedo / 3.141592) * selectedLight.colorAndStrength.xyz * NdotL_dir;
                }
            }
        } else {
            vec3 lightVec = selectedLight.posAndType.xyz - hitPos;
            float dist = length(lightVec);
            vec3 lightDir = lightVec / dist;
            float NdotL = max(dot(normal, lightDir), 0.0);
            if (NdotL > 0.0) {
                shadowPayload.isShadowed = 1;
                traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 0, hitPos + normal * 0.05, 0.01, lightDir, dist - 0.05, 0);
                if (shadowPayload.isShadowed == 0) {
                    float atten = selectedLight.colorAndStrength.w / max(dist * dist, 0.0001);
                    directLighting += (albedo / 3.141592) * selectedLight.colorAndStrength.xyz * NdotL * atten;
                }
            }
        }
    }

    payload.color = directLighting;
    payload.distance = gl_HitTEXT;
    payload.hitPos = hitPos;
    payload.hitNormal = normal;
    payload.albedo = albedo;
}