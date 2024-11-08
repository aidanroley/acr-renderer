#include "../include/file_funcs.h"

void compileShader(const std::vector<std::string>& inputFilePaths) {

    for (const auto& filePath : inputFilePaths) {

        // I use an extension that isn't compatible with my shader compile so I have to make a temp with the second line excluded
        copyFileExcludingSecondLine(filePath);
    }

    createCompileBatFile(inputFilePaths);
    system("cmd /c \"cd shaders && compile.bat\"");
}

bool copyFileExcludingSecondLine(const std::string& inputFilePath) {

    // Determine output file name by inserting "Temp" before the file extension
    size_t extensionPos = inputFilePath.find_last_of('.');
    std::string outputFilePath = (extensionPos == std::string::npos)
        ? inputFilePath + "Temp"
        : inputFilePath.substr(0, extensionPos) + "Temp" + inputFilePath.substr(extensionPos);

    // Open the input file for reading
    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) {

        std::cerr << "Failed to open input file: " << inputFilePath << std::endl;
        return false;
    }

    // Open the output file for writing
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {

        std::cerr << "Failed to create output file: " << outputFilePath << std::endl;
        inputFile.close();
        return false;
    }

    // Copy lines, skipping the second line
    std::string line;
    int lineNumber = 0;
    while (std::getline(inputFile, line)) {

        if (lineNumber != 1) {
            // Skip the second line (index 1)
            outputFile << line << "\n";
        }
        ++lineNumber;
    }

    // Close files
    inputFile.close();
    outputFile.close();

    std::cout << "File copied to " << outputFilePath << " with the second line excluded." << std::endl;
    return true;
}

void createCompileBatFile(const std::vector<std::string>& inputFilePaths) {

    const std::string batFilePath = "shaders/compile.bat";

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