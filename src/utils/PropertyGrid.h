#pragma once

#include "imgui.h"
#include <initializer_list>

namespace ui {

struct PropertyGridConfig {
    float labelWidth = 115.0f;
    ImGuiTableFlags flags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Resizable;

    PropertyGridConfig() = default;
    PropertyGridConfig(float width, ImGuiTableFlags tableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Resizable)
        : labelWidth(width), flags(tableFlags) {}
};

class PropertyGrid {
public:
    PropertyGrid(const char* tableId, const PropertyGridConfig& config = {});
    ~PropertyGrid();

    PropertyGrid(const PropertyGrid&) = delete;
    PropertyGrid& operator=(const PropertyGrid&) = delete;

    PropertyGrid(PropertyGrid&& other) noexcept;
    PropertyGrid& operator=(PropertyGrid&& other) noexcept;

    static PropertyGrid begin(const char* tableId, const PropertyGridConfig& config = {}) {
        return PropertyGrid(tableId, config);
    }

    void end();
    explicit operator bool() const noexcept { return isOpen; }

    void beginRow(const char* label);
    void endRow();

    bool sliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    bool sliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
    bool dragFloat(const char* label, float* v, float speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    bool dragFloat3(const char* label, float v[3], float speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    bool combo(const char* label, int* current_item, const char* const items[], int items_count);
    bool combo(const char* label, int* current_item, std::initializer_list<const char*> items);
    bool combo(const char* label, int* current_item, const char* items_separated_by_zeros);
    bool checkbox(const char* label, bool* v);
    bool colorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
    void text(const char* label, const char* fmt, ...);

private:
    bool isOpen = false;
};

struct ScopeID {
    explicit ScopeID(const char* str_id) { ImGui::PushID(str_id); }
    explicit ScopeID(const char* str_id_begin, const char* str_id_end) { ImGui::PushID(str_id_begin, str_id_end); }
    explicit ScopeID(const void* ptr_id) { ImGui::PushID(ptr_id); }
    explicit ScopeID(int int_id) { ImGui::PushID(int_id); }
    ~ScopeID() { ImGui::PopID(); }

    ScopeID(const ScopeID&) = delete;
    ScopeID& operator=(const ScopeID&) = delete;
    ScopeID(ScopeID&&) = delete;
    ScopeID& operator=(ScopeID&&) = delete;
};

class TreeNodeScope {
public:
    explicit TreeNodeScope(const char* label, ImGuiTreeNodeFlags flags = 0);
    TreeNodeScope(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...);
    TreeNodeScope(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...);
    ~TreeNodeScope();

    static TreeNodeScope begin(const char* label, ImGuiTreeNodeFlags flags = 0) {
        return TreeNodeScope(label, flags);
    }

    TreeNodeScope(const TreeNodeScope&) = delete;
    TreeNodeScope& operator=(const TreeNodeScope&) = delete;
    TreeNodeScope(TreeNodeScope&& other) noexcept;
    TreeNodeScope& operator=(TreeNodeScope&& other) noexcept;

    explicit operator bool() const noexcept { return m_isOpen; }
    bool isOpen() const noexcept { return m_isOpen; }

private:
    bool m_isOpen = false;
};

inline bool Button(const char* label, const ImVec2& size = ImVec2(-1.0f, 0.0f)) {
    return ImGui::Button(label, size);
}

} // namespace ui
