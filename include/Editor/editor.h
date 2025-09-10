#pragma once
#include "Editor/UI/gui_layer.h"
#include "Core/Utils/timer.h"

class Editor {

public:
	void Update();

private:
	void updateContext();
	GuiLayer guiLayer;
	EditorContext& editorContext = EditorContext::Get();
	Utils::Timer::Timer& timer = Utils::Timer::get();
};