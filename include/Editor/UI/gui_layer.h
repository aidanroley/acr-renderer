#pragma once
#include "Editor/editor_context.h"

class GuiLayer {

public:
	void Begin();

private:
	void setupGuiFrame();
	void DrawFPSCounter();
	EditorContext& editorContext = EditorContext::Get();
};