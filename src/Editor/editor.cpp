#include "pch.h"
#include "Editor/editor.h"



void Editor::Update() {

	updateContext();
	guiLayer.Begin();
}

void Editor::updateContext() {

	editorContext.fps = timer.getFPS();
}