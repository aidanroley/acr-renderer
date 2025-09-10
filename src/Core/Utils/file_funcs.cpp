#include "pch.h"
#include "Core/Utils/file_funcs.h"

namespace Utils::File {

    void compileShader(const std::vector<std::string>& inputFilePaths) {

        for (const auto& filePath : inputFilePaths) {

            // I use an extension that isn't compatible with my shader compile so I have to make a temp with the second line excluded
            copyFileExcludingSecondLine(filePath);
        }

        createCompileBatFile(inputFilePaths);
        system("cmd /c \"cd shaders\\shaderCompilation && compile.bat\"");
    }

    bool copyFileExcludingSecondLine(const std::string& inputFilePath) {

        size_t extensionPos = inputFilePath.find_last_of('.');
        std::string fileName = (extensionPos == std::string::npos)
            ? inputFilePath + "Temp"
            : inputFilePath.substr(0, extensionPos) + "Temp" + inputFilePath.substr(extensionPos);

        std::string outputDirectory = "shaders/shaderCompilation/";
        std::string outputFilePath = outputDirectory + fileName.substr(fileName.find_last_of("/\\") + 1);

        std::ifstream inputFile(inputFilePath);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open input file: " << inputFilePath << std::endl;
            return false;
        }
        std::ofstream outputFile(outputFilePath);
        if (!outputFile.is_open()) {
            std::cerr << "Failed to create output file: " << outputFilePath << std::endl;
            inputFile.close();
            return false;
        }
        std::string line;
        int lineNumber = 0;
        while (std::getline(inputFile, line)) {
            if (lineNumber != 1) {
                outputFile << line << "\n";
            }
            ++lineNumber;
        }
        inputFile.close();
        outputFile.close();

        std::cout << "File copied to " << outputFilePath << " with the second line excluded." << std::endl;
        return true;
    }


    void createCompileBatFile(const std::vector<std::string>& inputFilePaths) {

        const std::string batFilePath = "shaders/shaderCompilation/compile.bat";

        // Open bat file for writing
        std::ofstream batFile(batFilePath);
        if (!batFile.is_open()) {

            std::cerr << "failed to create" << batFilePath << std::endl;
            return;
        }

        const std::string compilerPath = "C:/VulkanSDK/1.3.296.0/bin/glslc.exe";
        for (const auto& filePath : inputFilePaths) {

            std::string baseName = std::filesystem::path(filePath).stem().string();
            std::string extension = std::filesystem::path(filePath).extension().string();

            std::string tempFileName = baseName + "Temp" + extension;
            std::string outputSpvFileName = baseName + ".spv";
            batFile << compilerPath << " " << tempFileName << " -o " << outputSpvFileName << std::endl;
        }

        batFile.close();
    }

    void deleteAllExceptCompileBat(const std::string& filename) {
        namespace fs = std::filesystem;

        fs::path dir = fs::path(filename).parent_path();   // directory that holds filename
        const fs::path keep{ "compile.bat" };                // name to spare (case-sensitive)

        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().filename() != keep) {
                std::error_code ec;                        // suppress throw on failure
                fs::remove(entry.path(), ec);
                if (ec) {
                    std::cerr << "Could not delete " << entry.path() << ": " << ec.message() << '\n';
                }
                else {
                    std::cout << "Deleted " << entry.path() << '\n';
                }
            }
        }
    }

    std::vector<char> readFile(const std::string& filename) {

        std::cout << "Attempting to open file: " << filename << std::endl;

        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {

            throw std::runtime_error("failed to open file");
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }
}
