#ifndef IMGUI_IMGUI_EX_WIN32_H_
#define IMGUI_IMGUI_EX_WIN32_H_

#include <unordered_map>
#include <string>


#ifndef IMGUI_EX_CPP
#include <imgui/imgui.h>
#include <imgui/imconfig.h>
#include <imgui/imgui_internal.h>
#include <imgui/imstb_rectpack.h>
#include <imgui/imstb_textedit.h>

#include <imgui/backends/imgui_impl_dx11.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <d3d11.h>
#endif

void ImGuiInit();
void ImGuiUpdate();
void ImGuiExit();

namespace ImGuiEx {

void ExitApplication();
void SlowDown();

namespace internal {
static HWND GetWindowHwnd(ImGuiWindow* window) {
    if (window == nullptr) return NULL;
    return (HWND)window->Viewport->PlatformHandle;
}
static HWND FindWindowHwndByName(const char* name) {
    auto window = ImGui::FindWindowByName(name);
    return GetWindowHwnd(window);
}
static bool SetWindowTop(ImGuiWindow* window, bool top) {
    //ImGuiWindow* window = ImGui::GetCurrentWindow();
    HWND hwnd = internal::GetWindowHwnd(window);
    if (hwnd == NULL) {
        return false;
    }

    if (top) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    return true;
}
} // namespace internal


#define IMGUI_EX_EXPAND_UNIT_DECLARATION() \
    bool old_expand_; \
    bool expand_; \

#define IMGUI_EX_EXPAND_UNIT_EVENT_DEFINITION() \
    template <typename EventBlock> \
    void ExpandEvent(EventBlock expand) { \
        if (expand_ == true && expand_ != old_expand_) { \
            expand(); \
        } \
    } \
    \
    template <typename EventBlock> \
    void CollapsingEvent(EventBlock collapsing) { \
        if (expand_ == false && expand_ != old_expand_) { \
            collapsing(); \
        } \
    } \

#define IMGUI_EX_EXPAND_UNIT_UPDATE_DEFINITION() \
    template <typename UpdateBlock> \
    void ExpandUpdate(UpdateBlock expand) { \
        if (expand_ == true) { \
            expand(); \
        } \
    } \
    \
    template <typename UpdateBlock> \
    void CollapsingUpdate(UpdateBlock collapsing) { \
        if (expand_ == false) { \
            collapsing(); \
        } \
    } \

class Unit {
public:
    Unit(const std::string& label) : label_(label) {
        end_disabled_ = false;
        disabled_ = false;
    }

    void Begin() {
        if (disabled_) {
            ImGui::BeginDisabled();
            end_disabled_ = true;
        }
        else {
            end_disabled_ = false;
        }
    }

    void End() {
        if (end_disabled_) {
            ImGui::EndDisabled();
        }
    }

    std::string GetLabel() {
        return label_;
    }

    void SetLabel(const std::string& label) {
        label_ = label;
    }

    void SetDisable(bool disabled) {
        disabled_ = disabled;
    }

private:
    std::string label_;

    bool end_disabled_;
    bool disabled_;
};



class Window : public Unit {
public:
    Window(const std::string& label, bool main) : Unit(label) {
        window_ = nullptr;
        main_ = main;
        old_top_ = true;
        top_ = false;
        old_create_ = false;
        create_ = true;

        old_expand_ = false;
        expand_ = false;

        old_flags_ = 0;
        flags_ = 0;
    }

    void Begin() {
        Unit::Begin();
        if (window_ == nullptr) {
            window_ = ImGui::FindWindowByName(GetLabel().c_str());
        }

        if (old_top_ != top_) {
            if (internal::SetWindowTop(window_, top_)) {
                old_top_ = top_;
            }
        }

        if (create_) {
            expand_ = ImGui::Begin(GetLabel().c_str(), &create_, flags_);
        }
        if (create_ == true && create_ != old_create_) {
            // OpenEvent
            // 每次重新开启窗口都要重设top状态
            old_top_ = !top_;
        }
    }

    void End() {
        old_create_ = create_;
        if (create_ == false) {
            if (main_) {
                ExitApplication();
            }
        }
        old_expand_ = expand_;
        ImGui::End();
        Unit::End();
    }

    /*
    * Update
    */
    template <typename UpdateBlock>
    void CreateUpdate(UpdateBlock create) {
        if (create_) {
            create();
        }
    }

    template <typename UpdateBlock>
    void CloseUpdate(UpdateBlock close) {
        if (!create_) {
            close();
        }
    }


    /*
    * Event
    */
    template <typename EventBlock>
    void CreateEvent(EventBlock open) {
        if (old_create_ == false && create_ == true) {
            open();
        }
    }

    template <typename EventBlock>
    void CloseEvent(EventBlock close) {
        if (old_create_ == true && create_ == false) {
            close();
        }
    }

    IMGUI_EX_EXPAND_UNIT_EVENT_DEFINITION();

    
    /*
    * Control
    */
    void Open() {
        create_ = true;
    }

    void Close() {
        create_ = false;
    }


    void SetTop(bool top) {
        top_ = top;
    }

    void SetDocking(bool enable){
        if (enable) {
            flags_ &= (~ImGuiWindowFlags_NoDocking);
        }
        else {
            flags_ |= ImGuiWindowFlags_NoDocking;
        }
    }

private:
    ImGuiWindow* window_;

    bool main_;
    bool old_top_;
    bool top_;
    bool old_create_;
    bool create_;

    IMGUI_EX_EXPAND_UNIT_DECLARATION();

    ImGuiWindowFlags old_flags_;
    ImGuiWindowFlags flags_;
};

class Button : public Unit {
public:
    Button(const std::string& label) : Unit(label) {
        click_ = false;
    }

    void Begin() {
        Unit::Begin();
        click_ = ImGui::Button(GetLabel().c_str());
    }

    void End() {
        click_ = false;
        Unit::End();
    }

    /*
    * Event
    */
    template <typename EventBlock>
    void ClickEvent(EventBlock click) {
        if (click_ == true) {
            click();
        }
    }

private:
    bool click_;
};

template<class Element = std::string>
class Combo : public Unit {
public:
    Combo(const std::string& label) : Unit(label) {
        old_select_index_ = -1;
        select_index_ = -1;

        old_expand_ = false;
        expand_ = false;
    }

    void Begin() {
        Unit::Begin();
        expand_ = ImGui::BeginCombo(GetLabel().c_str(), select_label_.c_str());
    }

    void End() {
        ImGui::EndCombo();
        old_expand_ = expand_;
        old_select_index_ = select_index_;
        Unit::End();
    }

    /*
    * Update
    */
    template <typename UpdateBlock>
    void InsertUpdate(UpdateBlock insert) {
        if (expand_) {
            return;
        }
        for (int i = 0; i < list_.size(); i++) {
            const bool is_selected = (select_index_ == i);
            auto temp = insert(list_[i]);

            if (temp.empty()) {
                continue;
            }

            if (ImGui::Selectable(temp.c_str(), is_selected)) {
                select_index_ = i;
                select_label_ = temp;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        
    }

    /*
    * Event
    */
    IMGUI_EX_EXPAND_UNIT_EVENT_DEFINITION();

    template <typename EventBlock>
    void SelectEvent(EventBlock select) {
        if (old_select_index_ != select_index_) {
            select();
        }
    }


    /*
    * Control
    */
    void SetList(std::vector<Element>&& list) {
        list_ = std::move(list);
    }

    void ClearList() {
        list_.clear();
    }

private:
    std::vector<Element> list_;
    int old_select_index_;
    int select_index_;
    std::string select_label_;

    IMGUI_EX_EXPAND_UNIT_DECLARATION();

};

class InputText : public Unit {
public:
    InputText(const std::string& label, size_t text_size) : Unit(label), text_(text_size, '\0') {
        old_input_ = false;
        input_ = false;
    }

    void Begin() {
        Unit::Begin();
        if (ImGui::InputText(GetLabel().c_str(), (char*)text_.c_str(), text_.size())) {
            input_ = true;
        } else {
            input_ = false;
        }
    }

    void End() {
        old_input_ = input_;
        Unit::End();
    }

    /*
    * Event
    */
    template <typename EventBlock>
    void InputEvent(EventBlock input) {
        if (input_ == true && input_ != old_input_) {
            input();
        }
    }

    std::string GetText() {
        return std::string(text_.c_str(), strlen(text_.c_str()));
    }

private:
    std::string text_;

    bool old_input_;
    bool input_;
};


class Text : public Unit {
public:
    template<typename ... Args>
    Text(const char* fmt, Args... args) : Unit(fmt) {
        
    }

    void Begin() {
        Unit::Begin();
        ImGui::Text(GetLabel().c_str());
    }

    void End() {
        Unit::End();
    }
};

class SeparatorText : public Unit {
public:
    SeparatorText(const std::string& label) : Unit(label) {
        
    }

    void Begin() {
        Unit::Begin();
        ImGui::SeparatorText(GetLabel().c_str());
    }

    void End() {
        Unit::End();
    }
};

class BulletText : public Unit {
public:
    template<typename ... Args>
    BulletText(const char* fmt, Args... args) : Unit(fmt) {
        ImGui:;BulletText(fmt, args...);
    }

    void Begin() {
        Unit::Begin();
        ImGui::BulletText(GetLabel().c_str());
    }

    void End() {
        Unit::End();
    }
};

class HelpMarker : public Unit {
public:
    HelpMarker(const std::string& title, const std::string& desc) : Unit(title), desc_(desc) {
        
    }

    void Begin() {
        Unit::Begin();
        
        ImGui::TextDisabled(GetLabel().c_str());
        if (ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc_.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void End() {
        Unit::End();
    }

private:
    std::string desc_;
};


class CollapsingHeader : public Unit {
public:
    CollapsingHeader(const std::string& label) : Unit(label) {
        old_expand_ = false;
        expand_ = false;
    }

    void Begin() {
        Unit::Begin();
        expand_ = ImGui::CollapsingHeader(GetLabel().c_str());
    }

    void End() {
        old_expand_ = expand_;
        Unit::End();
    }

    /*
    * Update
    */
    IMGUI_EX_EXPAND_UNIT_UPDATE_DEFINITION();

    /*
    * Event
    */
    IMGUI_EX_EXPAND_UNIT_EVENT_DEFINITION();

private:
    IMGUI_EX_EXPAND_UNIT_DECLARATION();
};

class TreeNode : public Unit {
public:
    TreeNode(const std::string& label) : Unit(label) {
        old_expand_ = false;
        expand_ = false;
    }

    void Begin() {
        Unit::Begin();
        expand_ = ImGui::TreeNode(GetLabel().c_str());
    }

    void End() {
        if (expand_) {
            ImGui::TreePop();
        }
        old_expand_ = expand_;
        Unit::End();
    }


    /*
    * Event
    */
    IMGUI_EX_EXPAND_UNIT_EVENT_DEFINITION();
private:
    IMGUI_EX_EXPAND_UNIT_DECLARATION();
};

class CheckBox : public Unit {
public:
    CheckBox(const std::string& label) : Unit(label) {
        old_check_ = false;
        check_ = false;
    }

    void Begin() {
        Unit::Begin();
        ImGui::Checkbox(GetLabel().c_str(), &check_);
    }

    void End() {
        old_check_ = check_;
        Unit::End();
    }


    /*
    * Event
    */
    template <typename EventBlock>
    void CheckEvent(EventBlock check) {
        if (old_check_ == false && check_ == true) {
            check();
        }
    }

    template <typename EventBlock>
    void UncheckEvent(EventBlock uncheck) {
        if (old_check_ == true && check_ == false) {
            uncheck();
        }
    }

    /*
    * Control
    */
    void SetCheck(bool check) {
        check_ = check;
    }

private:
    bool old_check_;
    bool check_;
};

template<class Element = std::string>
class ListBox : public Unit {
public:
    ListBox(const std::string& label) : Unit(label) {
        old_select_index_ = -1;
        select_index_ = -1;
    }

    void Begin() {
        Unit::Begin();
        ImGui::BeginListBox(GetLabel().c_str());
    }

    void End() {
        ImGui::EndListBox();
        Unit::End();
    }

    /*
    * Update
    */
    template <typename UpdateBlock>
    void InsertUpdate(UpdateBlock insert) {
        for (int i = 0; i < list_.size(); i++) {
            const bool is_selected = (select_index_ == i);
            auto temp = insert(list_[i]);

            if (temp.empty()) {
                continue;
            }

            if (ImGui::Selectable(temp.c_str(), is_selected)) {
                select_index_ = i;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
    }

    /*
    * Event
    */
    template <typename EventBlock>
    void SelectEvent(EventBlock select) {
        if (old_select_index_ != select_index_) {
            select();
        }
    }

    /*
    * Control
    */
    void SetList(std::vector<Element>&& list) {
        list_ = std::move(list);
    }

    void ClearList() {
        list_.clear();
    }

private:
    std::vector<Element> list_;
    int old_select_index_;
    int select_index_;
};

class RadioButtonGroup : public Unit {
public:
    RadioButtonGroup(std::vector<std::string> label_list) : Unit(""), label_list_(label_list){
        push_index_ = 0;
        select_index_ = 0;
        old_select_index_ = 0;
    }

    template <typename PushBlock>
    void Begin(PushBlock push) {
        Unit::Begin();
        push_index_ = 0;
        for (size_t i = 0; i < label_list_.size(); i++) {
            ImGui::RadioButton(label_list_[i].c_str(), &select_index_, push_index_++);
            push(i);
        }
    }

    void Begin() {
        Begin([](size_t i) {});
    }

    void End() {
        old_select_index_ = select_index_;
        Unit::End();
    }



    template <typename EventBlock>
    void SelectEvent(EventBlock select) {
        if (old_select_index_ != select_index_) {
            select();
        }
    }

private:
    std::vector<std::string> label_list_;

    int push_index_;
    int old_select_index_;
    int select_index_;
};



namespace layout {
    static void Indent() {
        ImGui::Indent();
    }

    static void Unindent() {
        ImGui::Unindent();
    }

    static void Spacing() {
        ImGui::Spacing();
    }

    static void SameLine() {
        ImGui::SameLine();
    }
}

} // namespace ImGuiEx


#endif // IMGUI_IMGUI_EX_WIN32_H_
