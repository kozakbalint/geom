#include "demos/subdivision/SubdivisionDemo.h"
#include "geometry/subdivision/LoopSubdivision.h"
#include "geometry/subdivision/ButterflySubdivision.h"
#include "geometry/subdivision/CatmullClarkSubdivision.h"
#include "demos/subdivision/FlatShader.h"
#include "utils/FileDialog.h"
#include "utils/STLLoader.h"
#include <rlgl.h>
#include <raymath.h>
#include <imgui.h>
#include <algorithm>
#include <cctype>

void SubdivisionDemo::init() {
    camera.position = { 0.0f, 1.5f, 3.5f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    flatShader = LoadShaderFromMemory(flat_vs, flat_fs);
    lightDirLoc = GetShaderLocation(flatShader, "lightDir");
    lightIntensityLoc = GetShaderLocation(flatShader, "lightIntensity");

    material = LoadMaterialDefault();
    material.shader = flatShader;
    material.maps[MATERIAL_MAP_DIFFUSE].color = diffuseColor;

    wireMaterial = LoadMaterialDefault();
    wireMaterial.maps[MATERIAL_MAP_DIFFUSE].color = { 30, 30, 30, 255 }; // Dark gray wireframe
}


void SubdivisionDemo::destroy() {
    cache.Unload();
    if (isModelLoaded) {
        UnloadModel(loadedModel);
        isModelLoaded = false;
    } else if (meshLoaded) {
        UnloadMesh(currentBaseMesh);
    }
    UnloadShader(flatShader);
}

void SubdivisionDemo::rebuildCache() {
    cache.Unload();
    
    // Only rebuild if we actually have a mesh loaded
    if (!isModelLoaded && !meshLoaded) {
        return;
    }
    
    core::HalfEdgeMesh baseHe = core::HalfEdgeConversion::ToHalfEdge(currentBaseMesh);
    
    cache.heLevels.push_back(baseHe);
    Mesh baseMeshCopy = core::HalfEdgeConversion::FromHalfEdge(baseHe);
    UploadMesh(&baseMeshCopy, false);
    cache.raylibLevels.push_back(baseMeshCopy);
    
    for (int i = 0; i < currentIteration; ++i) {
        core::HalfEdgeMesh subHe;
        if (currentSchema == SCHEMA_LOOP) {
            subHe = core::LoopSubdivision::Subdivide(cache.heLevels.back());
        } else if (currentSchema == SCHEMA_BUTTERFLY) {
            subHe = core::ButterflySubdivision::Subdivide(cache.heLevels.back());
        } else if (currentSchema == SCHEMA_CATMULL_CLARK) {
            subHe = core::CatmullClarkSubdivision::Subdivide(cache.heLevels.back());
        }
        cache.heLevels.push_back(subHe);
        
        Mesh subMesh = core::HalfEdgeConversion::FromHalfEdge(subHe);
        UploadMesh(&subMesh, false);
        cache.raylibLevels.push_back(subMesh);
    }
}

void SubdivisionDemo::normalizeMesh(Mesh& mesh) {
    if (mesh.vertexCount == 0 || mesh.vertices == nullptr) return;

    float minX = mesh.vertices[0], maxX = mesh.vertices[0];
    float minY = mesh.vertices[1], maxY = mesh.vertices[1];
    float minZ = mesh.vertices[2], maxZ = mesh.vertices[2];

    for (int i = 1; i < mesh.vertexCount; ++i) {
        float x = mesh.vertices[i * 3 + 0];
        float y = mesh.vertices[i * 3 + 1];
        float z = mesh.vertices[i * 3 + 2];

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }

    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;

    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;

    float maxDim = std::max({ sizeX, sizeY, sizeZ });
    if (maxDim < 1e-6f) maxDim = 1.0f;

    // Scale so that maximum dimension is 2.0 units (fitting neatly in [-1, 1])
    float scale = 2.0f / maxDim;

    for (int i = 0; i < mesh.vertexCount; ++i) {
        mesh.vertices[i * 3 + 0] = (mesh.vertices[i * 3 + 0] - centerX) * scale;
        mesh.vertices[i * 3 + 1] = (mesh.vertices[i * 3 + 1] - centerY) * scale;
        mesh.vertices[i * 3 + 2] = (mesh.vertices[i * 3 + 2] - centerZ) * scale;
    }
}

bool SubdivisionDemo::loadFile(const std::string& path) {
    if (path.empty()) return false;

    std::string ext = GetFileExtension(path.c_str());
    for (char& c : ext) c = (char)tolower(c);
    if (ext != ".obj" && ext != ".stl") {
        statusMessage = "Unsupported file type: " + ext;
        return false;
    }

    Model newModel = { 0 };
    if (ext == ".stl") {
        newModel = utils::LoadModelSTL(path);
    } else {
        newModel = LoadModel(path.c_str());
    }

    if (newModel.meshCount == 0) {
        statusMessage = "Failed to load model: " + path;
        return false;
    }

    if (isModelLoaded) {
        UnloadModel(loadedModel);
    } else if (meshLoaded) {
        UnloadMesh(currentBaseMesh);
    }

    loadedModel = newModel;
    isModelLoaded = true;
    meshLoaded = false;

    currentBaseMesh = loadedModel.meshes[0];
    normalizeMesh(currentBaseMesh);

    // Reset camera to nicely frame the centered, normalized model
    camera.position = { 0.0f, 1.5f, 3.5f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };

    loadedFileName = GetFileName(path.c_str());
    loadedVertexCount = currentBaseMesh.vertexCount;
    loadedTriangleCount = currentBaseMesh.triangleCount;
    statusMessage = "Loaded: " + loadedFileName;

    currentIteration = 0;
    rebuildCache();

    if (!cache.raylibLevels.empty()) {
        loadedVertexCount = cache.raylibLevels[0].vertexCount;
        loadedTriangleCount = cache.raylibLevels[0].triangleCount;
    }
    return true;
}

void SubdivisionDemo::render() {
    if (!ImGui::GetIO().WantCaptureMouse) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mouseDelta = GetMouseDelta();
            float rotationSpeed = 0.005f;

            Vector3 view = Vector3Subtract(camera.position, camera.target);
            
            // Yaw
            Matrix yawMatrix = MatrixRotate(camera.up, -mouseDelta.x * rotationSpeed);
            view = Vector3Transform(view, yawMatrix);
            
            // Pitch
            Vector3 right = Vector3CrossProduct(camera.up, view);
            right = Vector3Normalize(right);
            Matrix pitchMatrix = MatrixRotate(right, -mouseDelta.y * rotationSpeed);
            Vector3 newView = Vector3Transform(view, pitchMatrix);
            
            float dot = Vector3DotProduct(Vector3Normalize(newView), camera.up);
            if (dot < 0.99f && dot > -0.99f) {
                view = newView;
            }
            
            camera.position = Vector3Add(camera.target, view);
        }
        
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            Vector3 view = Vector3Subtract(camera.position, camera.target);
            float length = Vector3Length(view);
            // Proportional zooming is more granular when close
            length *= (1.0f - wheel * 0.1f);
            if (length < 0.1f) length = 0.1f;
            view = Vector3Normalize(view);
            view = Vector3Scale(view, length);
            camera.position = Vector3Add(camera.target, view);
        }
    }

    BeginMode3D(camera);
    
    if (!cache.raylibLevels.empty()) {
        Mesh& m = cache.raylibLevels.back();
        
        float yawRad = lightYaw * DEG2RAD;
        float pitchRad = lightPitch * DEG2RAD;
        lightDir.x = cosf(pitchRad) * sinf(yawRad);
        lightDir.y = sinf(pitchRad);
        lightDir.z = cosf(pitchRad) * cosf(yawRad);

        Vector3 normalizedLightDir = Vector3Normalize(lightDir);
        SetShaderValue(flatShader, lightDirLoc, &normalizedLightDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(flatShader, lightIntensityLoc, &lightIntensity, SHADER_UNIFORM_FLOAT);

        diffuseColor.r = (unsigned char)(diffuseColorF[0] * 255.0f);
        diffuseColor.g = (unsigned char)(diffuseColorF[1] * 255.0f);
        diffuseColor.b = (unsigned char)(diffuseColorF[2] * 255.0f);
        diffuseColor.a = (unsigned char)(faceOpacity * 255.0f);
        material.maps[MATERIAL_MAP_DIFFUSE].color = diffuseColor;

        if (faceOpacity > 0.0f) {
            DrawMesh(m, material, MatrixIdentity());
        }

        if (wireframe) {
            rlEnableWireMode();
            DrawMesh(m, wireMaterial, MatrixIdentity());
            rlDisableWireMode();
        }
    }
    
    EndMode3D();
}

void SubdivisionDemo::renderUI() {
    if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Browse File... (.obj, .stl)", ImVec2(-1, 28))) {
            std::string selected = utils::OpenModelFileDialog(GetWindowHandle());
            if (!selected.empty()) {
                loadFile(selected);
            }
        }

        if (!statusMessage.empty()) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", statusMessage.c_str());
        }

        if (isModelLoaded || meshLoaded) {
            ImGui::Text("Base Verts: %d | Faces: %d", loadedVertexCount, loadedTriangleCount);
            if (ImGui::Button("Reset Camera", ImVec2(-1, 0))) {
                camera.position = { 0.0f, 1.5f, 3.5f };
                camera.target = { 0.0f, 0.0f, 0.0f };
                camera.up = { 0.0f, 1.0f, 0.0f };
            }
        }
    }

    if (ImGui::CollapsingHeader("Subdivision Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("controls_table", 2)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthFixed, 180.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Schema");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            int schema = currentSchema;
            if (ImGui::Combo("##Schema", &schema, "Loop\0Butterfly\0Catmull-Clark\0")) {
                currentSchema = schema;
            }
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Iterations");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::SliderInt("##Iterations", &currentIteration, 0, maxIterations);
            ImGui::PopItemWidth();
            
            ImGui::EndTable();
        }
        
        if (ImGui::Button("Apply Subdivision", ImVec2(-1, 0))) {
            rebuildCache();
        }
    }

    if (ImGui::CollapsingHeader("Material Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("mat_controls_table", 2)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthFixed, 180.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Wireframe");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##Wireframe", &wireframe);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Face Opac.");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##FaceOpacity", &faceOpacity, 0.0f, 1.0f);
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Light Yaw");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##LightYaw", &lightYaw, -180.0f, 180.0f);
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Light Pitch");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##LightPitch", &lightPitch, -90.0f, 90.0f);
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Intensity");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##Intensity", &lightIntensity, 0.0f, 5.0f);
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Color");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::ColorEdit3("##Color", diffuseColorF);
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }
    }
}

