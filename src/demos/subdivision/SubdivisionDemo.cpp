#include "demos/subdivision/SubdivisionDemo.h"
#include "geometry/subdivision/LoopSubdivision.h"
#include "geometry/subdivision/ButterflySubdivision.h"
#include "demos/subdivision/FlatShader.h"
#include <rlgl.h>
#include <raymath.h>
#include <imgui.h>

void SubdivisionDemo::init() {
    camera.position = { 0.0f, 2.0f, 4.0f };
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
        } else {
            subHe = core::ButterflySubdivision::Subdivide(cache.heLevels.back());
        }
        cache.heLevels.push_back(subHe);
        
        Mesh subMesh = core::HalfEdgeConversion::FromHalfEdge(subHe);
        UploadMesh(&subMesh, false);
        cache.raylibLevels.push_back(subMesh);
    }
}

void SubdivisionDemo::render() {
    if (IsFileDropped()) {
        FilePathList droppedFiles = LoadDroppedFiles();
        if (droppedFiles.count > 0) {
            const char* path = droppedFiles.paths[0];
            if (IsFileExtension(path, ".obj") || IsFileExtension(path, ".stl")) {
                if (isModelLoaded) {
                    UnloadModel(loadedModel);
                } else if (meshLoaded) {
                    UnloadMesh(currentBaseMesh);
                }
                loadedModel = LoadModel(path);
                isModelLoaded = true;
                meshLoaded = false;
                
                if (loadedModel.meshCount > 0) {
                    currentBaseMesh = loadedModel.meshes[0];
                    currentIteration = 0;
                    rebuildCache();
                }
            }
        }
        UnloadDroppedFiles(droppedFiles);
    }

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
            if (ImGui::Combo("##Schema", &schema, "Loop\0Butterfly\0")) {
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
        
        ImGui::Separator();
        ImGui::TextWrapped("Drag & Drop .obj or .stl files");
        ImGui::TextWrapped("into the window to load them.");
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

