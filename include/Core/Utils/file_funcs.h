#pragma once

namespace Utils::File {

	void compileShader(const std::vector<std::string>& inputFilePaths);
	bool copyFileExcludingSecondLine(const std::string& inputFilePath);
	void createCompileBatFile(const std::vector<std::string>& inputFilePaths);
	std::vector<char> readFile(const std::string& filename);
	void deleteAllExceptCompileBat(const std::string& filename);
}
