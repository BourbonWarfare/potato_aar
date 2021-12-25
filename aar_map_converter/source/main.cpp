// aar_map_converter
// Takes in the map's SVG and compresses it into a pre-triangulated binary file for streaming
#include <filesystem>
#include <iostream>
#include <string>
#include "svgpp/svgpp.hpp"

void processSVG(std::filesystem::path file) {
    
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        std::cerr << "No input or output provided\n";
        return 1;
    }
    else if (argc == 2) {
        std::cerr << "No output provided\n";
        return 2;
    }

    std::filesystem::path input = argv[1];
    std::filesystem::path output = argv[2];

    if (!std::filesystem::exists(input)) {
        std::cerr << "Input path not found\n";
        return 3;
    }

    if (!std::filesystem::exists(output)) {
        std::cerr << "Output path not found - Creating\n";
        std::filesystem::create_directory(output);
    }

    for (auto &file : std::filesystem::directory_iterator{ input }) {
        if (file.path().extension() == ".svg") {
            std::cout << "Processing " << file << "\n";
            processSVG(file.path());
        }
    }

    return 0;
}
