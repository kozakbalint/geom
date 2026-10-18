#include "utils/PropertyGrid.h"
#include "imgui_internal.h"
#include <cstdarg>
#include <vector>

namespace ui {

PropertyGrid::PropertyGrid(const char* tableId, const PropertyGridConfig& config) {
    if (ImGui::BeginTable(tableId, 2, config.flags)) {
        isOpen = true;
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, config.labelWidth);
        ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide);
    }
}

PropertyGrid::~PropertyGrid() {
    end();
}

PropertyGrid::PropertyGrid(PropertyGrid&& other) noexcept : isOpen(other.isOpen) {
    other.isOpen = false;
}

PropertyGrid& PropertyGrid::operator=(PropertyGrid&& other) noexcept {
    if (this != &other) {
        end();
        isOpen = other.isOpen;
        other.isOpen = false;
    }
    return *this;
}

void PropertyGrid::end() {
    if (isOpen) {
        ImGui::EndTable();
        isOpen = false;
    }
}

void PropertyGrid::beginRow(const char* label) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    const char* safeLabel = label ? label : "";
    const char* labelEnd = ImGui::FindRenderedTextEnd(safeLabel);
    ImGui::TextUnformatted(safeLabel, labelEnd);

    ImGui::TableSetColumnIndex(1);
    ImGui::PushItemWidth(-1.0f);
    ImGui::PushID(safeLabel);
}

void PropertyGrid::endRow() {
    ImGui::PopID();
    ImGui::PopItemWidth();
}

bool PropertyGrid::sliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::SliderFloat("###ctrl", v, v_min, v_max, format, flags);
    endRow();
    return changed;
}

bool PropertyGrid::sliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::SliderInt("###ctrl", v, v_min, v_max, format, flags);
    endRow();
    return changed;
}

bool PropertyGrid::dragFloat(const char* label, float* v, float speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::DragFloat("###ctrl", v, speed, v_min, v_max, format, flags);
    endRow();
    return changed;
}

bool PropertyGrid::dragFloat3(const char* label, float v[3], float speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::DragFloat3("###ctrl", v, speed, v_min, v_max, format, flags);
    endRow();
    return changed;
}

bool PropertyGrid::combo(const char* label, int* current_item, const char* const items[], int items_count) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::Combo("###ctrl", current_item, items, items_count);
    endRow();
    return changed;
}

bool PropertyGrid::combo(const char* label, int* current_item, std::initializer_list<const char*> items) {
    std::vector<const char*> vec(items);
    return combo(label, current_item, vec.data(), static_cast<int>(vec.size()));
}

bool PropertyGrid::combo(const char* label, int* current_item, const char* items_separated_by_zeros) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::Combo("###ctrl", current_item, items_separated_by_zeros);
    endRow();
    return changed;
}

bool PropertyGrid::checkbox(const char* label, bool* v) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::Checkbox("###ctrl", v);
    endRow();
    return changed;
}

bool PropertyGrid::colorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags) {
    if (!isOpen) return false;
    beginRow(label);
    bool changed = ImGui::ColorEdit3("###ctrl", col, flags);
    endRow();
    return changed;
}

void PropertyGrid::text(const char* label, const char* fmt, ...) {
    if (!isOpen) return;
    beginRow(label);
    ImGui::AlignTextToFramePadding();
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    endRow();
}

TreeNodeScope::TreeNodeScope(const char* label, ImGuiTreeNodeFlags flags)
    : m_isOpen(ImGui::TreeNodeEx(label, flags)) {
}

TreeNodeScope::TreeNodeScope(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    m_isOpen = ImGui::TreeNodeExV(str_id, flags, fmt, args);
    va_end(args);
}

TreeNodeScope::TreeNodeScope(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    m_isOpen = ImGui::TreeNodeExV(ptr_id, flags, fmt, args);
    va_end(args);
}

TreeNodeScope::~TreeNodeScope() {
    if (m_isOpen) {
        ImGui::TreePop();
    }
}

TreeNodeScope::TreeNodeScope(TreeNodeScope&& other) noexcept : m_isOpen(other.m_isOpen) {
    other.m_isOpen = false;
}

TreeNodeScope& TreeNodeScope::operator=(TreeNodeScope&& other) noexcept {
    if (this != &other) {
        if (m_isOpen) {
            ImGui::TreePop();
        }
        m_isOpen = other.m_isOpen;
        other.m_isOpen = false;
    }
    return *this;
}

} // namespace ui
