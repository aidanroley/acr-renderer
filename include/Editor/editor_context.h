#pragma once

struct EditorContext {

	int winWidth, winHeight;
	float fps;

	static EditorContext& Get() {

		static EditorContext instance;
		return instance;
	}

	EditorContext(const EditorContext&) = delete;
	EditorContext& operator=(const EditorContext&) = delete;

	EditorContext() = default;

	void setWindowRes(int w, int h) { winWidth = w, winHeight = h; }
};