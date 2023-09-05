#ifndef IMGUI_IMPORT_WIN32_H_
#define IMGUI_IMPORT_WIN32_H_

#include <imgui/imgui.h>
#include <imgui/imconfig.h>
#include <imgui/imgui_internal.h>
#include <imgui/imstb_rectpack.h>
#include <imgui/imstb_textedit.h>

#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include <d3d11.h>

void ImGuiInit();
void ImGuiUpdate();

namespace ImGuiEx {

bool SetWindowsTop(ImGuiWindow* window, bool top);
void ExitApplication();

} // ImGuiEx


#endif // IMGUI_IMPORT_WIN32_H_
