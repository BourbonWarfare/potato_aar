// aar_map_converter
// Takes in the map's SVG and compresses it into a pre-triangulated binary file for streaming
#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>
#include <charconv>
#include <array>
#include <chrono>
#include "rapidxml/rapidxml.hpp"
#include "mapbox/earcut.hpp"

using coord = float;
using index = unsigned int;

constexpr float g_defaultLineWidth = 0.5f;
constexpr float g_defaultPolylineWidth = 0.5f;

struct rgb {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
};

float parseFloat(const std::string &str) {
    float result;
    std::from_chars(str.data(), str.data() + str.size(), result);
    return result;
}

rgb hexToRGB(const std::string &hex) {
    rgb colour;

    int counter = 0;

    unsigned char *workingColour = reinterpret_cast<unsigned char*>(&colour);
    for (auto &c : hex) {
        if (c == '#') { continue; }

        char number = 0;
        if (c >= '0' && c <= '9') {
            number += (c - '0');
        } else {
            number += 10 + (c - 'A');
        }

        // 1 - counter since we are working from left to right. We know each colour is 8 bytes, so we can assume this to be true and constant
        *workingColour += number * std::pow(16, 1 - counter);

        if (counter++ == 1) {
            workingColour++;
            counter = 0;
        }
    }

    return colour;
}

void handleDefs(rapidxml::xml_node<> *defs, std::unordered_map<std::string, rgb> &colourMap) {
    rapidxml::xml_node<> *node = defs->first_node();
    while (node) {
        if (std::string(node->name()) == "linearGradient") {
            rapidxml::xml_attribute<> *idAttribute = node->first_attribute("id");
            auto foo = node->first_node();
            rapidxml::xml_attribute<> *hexAttribute = node->first_node()->first_attribute("stop-color");

            std::string id(idAttribute->value(), idAttribute->value_size());
            std::string hex(hexAttribute->value(), hexAttribute->value_size());

            colourMap[id] = hexToRGB(hex);
        }
        node = node->next_sibling();
    }
}

rgb getColourFromFill(const std::unordered_map<std::string, rgb> &colourMap, rapidxml::xml_attribute<> *attribute, rgb defaultColour = { 255, 255, 255 }) {
    if (!attribute) {
        return defaultColour;
    }
    std::string colour = attribute->value();

    std::size_t urlIndex = colour.find("url(#");
    if (urlIndex != std::string::npos) {
        urlIndex += 5;

        std::string id;
        for (auto &c : colour.substr(urlIndex)) {
            if (c == ')') { break; }
            id += c;
        }

        return colourMap.at(id);
    }

    return hexToRGB(colour);
}

void createRect(rapidxml::xml_node<> *rect, const std::unordered_map<std::string, rgb> &colourMap, std::vector<coord> &vertices, std::vector<index> &indices, std::vector<rgb> &colours, rgb defaultColour) {
    coord x = parseFloat(rect->first_attribute("x")->value());
    coord y = parseFloat(rect->first_attribute("y")->value());
    coord w = parseFloat(rect->first_attribute("width")->value());
    coord h = parseFloat(rect->first_attribute("height")->value());

    rgb colour = getColourFromFill(colourMap, rect->first_attribute("fill"), defaultColour);

    index indexOffset = vertices.size() / 2;
    vertices.push_back(x + 0); vertices.push_back(y + 0);
    vertices.push_back(x + w); vertices.push_back(y + 0);
    vertices.push_back(x + w); vertices.push_back(y + h);
    vertices.push_back(x + 0); vertices.push_back(y + h);

    indices.push_back(indexOffset + 0);
    indices.push_back(indexOffset + 1);
    indices.push_back(indexOffset + 2);
    indices.push_back(indexOffset + 2);
    indices.push_back(indexOffset + 3);
    indices.push_back(indexOffset + 0);

    colours.push_back(colour);
    colours.push_back(colour);
    colours.push_back(colour);
    colours.push_back(colour);
}

void createPolygon(rapidxml::xml_node<> *polygon, const std::unordered_map<std::string, rgb> &colourMap, std::vector<coord> &vertices, std::vector<index> &indices, std::vector<rgb> &colours, rgb defaultColour) {
    rgb colour = getColourFromFill(colourMap, polygon->first_attribute("fill"), defaultColour);
    unsigned int indexOffset = vertices.size() / 2;

    std::string svgPoints = polygon->first_attribute("points")->value();

    // the points are given in format "x0,y0 x1,y1, ... xn,yn" so we split according to spaces and commas, and then we know the layout for later
    std::string workingPoint = "";
    std::vector<coord> xyPairs;
    for (auto &c : svgPoints) {
        if (c == ' ' || c == ',') {
            xyPairs.push_back(parseFloat(workingPoint));
            workingPoint = "";
        }
        else {
            workingPoint += c;
        }
    }
    if (workingPoint != "") {
        xyPairs.push_back(parseFloat(workingPoint));
    }

    if (xyPairs.empty()) {
        return;
    }

    std::vector<std::vector<std::array<coord, 2>>> adjustedXYPairs;
    adjustedXYPairs.emplace_back();
    for (unsigned int i = 0; i < xyPairs.size(); i += 2) {
        vertices.push_back(xyPairs[i + 0]);
        vertices.push_back(xyPairs[i + 1]);
        colours.push_back(colour);

        adjustedXYPairs[0].emplace_back(std::array<coord, 2>{ xyPairs[i + 0], xyPairs[i + 1] });
    }

    std::vector<index> newIndices = mapbox::earcut<index>(adjustedXYPairs);
    for (auto &index : newIndices) {
        index += indexOffset;
    }
    indices.insert(indices.end(), newIndices.begin(), newIndices.end());
}

void createLineShape(coord x0, coord y0, coord x1, coord y1, std::vector<coord> &vertices, std::vector<index> &indices, std::vector<rgb> &colours, rgb colour, float width) {
    coord directionX = x1 - x0;
    coord directionY = y1 - y0;

    coord magnitude = std::sqrt(directionX * directionX + directionY * directionY);
    if (magnitude == 0) {
        return;
    }

    directionX /= magnitude;
    directionY /= magnitude;

    coord normalDirectionX = directionY;
    coord normalDirectionY = -directionX;

    index indexOffset = vertices.size() / 2;
    vertices.push_back(x0 + (-normalDirectionX - directionX) * width);  vertices.push_back(y0 + (-normalDirectionY - directionY) * width);
    vertices.push_back(x1 + (directionX - normalDirectionX) * width);   vertices.push_back(y1 + (directionY - normalDirectionY) * width);
    vertices.push_back(x1 + (directionX + normalDirectionX) * width);   vertices.push_back(y1 + (directionY + normalDirectionY) * width);
    vertices.push_back(x0 + (normalDirectionX - directionX) * width);   vertices.push_back(y0 + (normalDirectionY - directionY) * width);

    indices.push_back(indexOffset + 0);
    indices.push_back(indexOffset + 1);
    indices.push_back(indexOffset + 2);
    indices.push_back(indexOffset + 2);
    indices.push_back(indexOffset + 3);
    indices.push_back(indexOffset + 0);

    colours.push_back(colour);
    colours.push_back(colour);
    colours.push_back(colour);
    colours.push_back(colour);
}

void createLine(rapidxml::xml_node<> *line, const std::unordered_map<std::string, rgb> &colourMap, std::vector<coord> &vertices, std::vector<index> &indices, std::vector<rgb> &colours, rgb defaultColour) {
    coord x0 = parseFloat(line->first_attribute("x1")->value());
    coord y0 = parseFloat(line->first_attribute("y1")->value());

    coord x1 = parseFloat(line->first_attribute("x2")->value());
    coord y1 = parseFloat(line->first_attribute("y2")->value());

    rgb colour = getColourFromFill(colourMap, line->first_attribute("fill"), rgb{ 0, 0, 0 });

    createLineShape(x0, y0, x1, y1, vertices, indices, colours, colour, g_defaultLineWidth);
}

void createPolyline(rapidxml::xml_node<> *polyline, const std::unordered_map<std::string, rgb> &colourMap, std::vector<coord> &vertices, std::vector<index> &indices, std::vector<rgb> &colours, rgb defaultColour) {
    rgb colour = getColourFromFill(colourMap, polyline->first_attribute("stroke"), defaultColour);
    std::string svgPoints = polyline->first_attribute("points")->value();

    // the points are given in format "x0,y0 x1,y1, ... xn,yn" so we split according to spaces and commas, and then we know the layout for later
    std::string workingPoint = "";
    std::vector<coord> xyPairs;
    for (auto &c : svgPoints) {
        if (c == ' ' || c == ',') {
            xyPairs.push_back(parseFloat(workingPoint));
            workingPoint = "";
        } else {
            workingPoint += c;
        }
    }
    if (workingPoint != "") {
        xyPairs.push_back(parseFloat(workingPoint));
    }

    if (xyPairs.empty()) { return; }

    for (unsigned int i = 0; i < xyPairs.size() - 2; i += 2) {
        coord x0 = xyPairs[i + 0];
        coord y0 = xyPairs[i + 1];

        coord x1 = xyPairs[i + 2];
        coord y1 = xyPairs[i + 3];

        createLineShape(x0, y0, x1, y1, vertices, indices, colours, colour, g_defaultPolylineWidth);
    }
}

void handleGroup(rapidxml::xml_node<> *group, const std::unordered_map<std::string, rgb> &colourMap, std::vector<coord> &vertices, std::vector<index> &indices, std::vector<rgb> &colours, rgb defaultColour) {
    defaultColour = getColourFromFill(colourMap, group->first_attribute("fill"), defaultColour);

    rapidxml::xml_node<> *child = group->first_node();
    while (child) {
        std::string name = child->name();
        if (name == "rect") {
            createRect(child, colourMap, vertices, indices, colours, defaultColour);
        } else if (name == "polygon") {
            createPolygon(child, colourMap, vertices, indices, colours, defaultColour);
        } else if (name == "line") {
            createLine(child, colourMap, vertices, indices, colours, defaultColour);
        } else if (name == "polyline") {
            createPolyline(child, colourMap, vertices, indices, colours, defaultColour);
        } else if (name == "g") {
            handleGroup(child, colourMap, vertices, indices, colours, defaultColour);
        } else {
            std::cout << "'" << child->name() << "' not defined SVG tag within module\n";
        }

        child = child->next_sibling();
    }
}

void parseSVGTree(rapidxml::xml_node<> *head, std::vector<coord> &vertices, std::vector<index> &indices, std::vector<rgb> &colours) {
    std::stack<rapidxml::xml_node<>*> nodes;
    nodes.push(head->first_node());

    std::unordered_map<std::string, rgb> colourMap;

    while (!nodes.empty()) {
        rapidxml::xml_node<> *node = nodes.top();
        nodes.pop();

        if (node->next_sibling()) {
            nodes.push(node->next_sibling());
        }

        std::string nodeName(node->name(), node->name_size());
        if (nodeName == "defs") {
            handleDefs(node, colourMap);
        } else if (nodeName == "g") {
            handleGroup(node, colourMap, vertices, indices, colours, rgb{255, 255, 255});
        } else {
            std::cerr << "Unknown Tag: " << nodeName << "\n";
        }
    }
}

void processSVG(std::filesystem::path file, std::filesystem::path outPath) {
    std::unique_ptr<rapidxml::xml_document<>> doc = std::make_unique<rapidxml::xml_document<>>();

    auto a = file.filename();

    // the XML parser needs to keep a reference to the string, so we can't destroy this
    std::string content;
    {
        std::ifstream in(file);
        std::stringstream buffer;
        buffer << in.rdbuf();
        in.close();

        content = buffer.str();
        doc->parse<0>(&content[0]);
    }

    std::vector<coord> vertices;
    std::vector<index> indices;
    std::vector<rgb> colours;

    parseSVGTree(doc->first_node(), vertices, indices, colours);

    {
        std::vector<char> totalInfo;

        // copying data to byte array for binary outputs
        /*
            TRIANGULATED FILE FORMAT:
                sizeof(coord) - always 1 byte
                coord count  - always 4 bytes
                coords

                sizeof(index) - always 1 byte
                index count  - always 4 bytes
                indices

                sizeof(rgb) - always 1 byte
                rgb count  - always 4 bytes
                rgbs
        */
        {
            std::uint8_t vertexSize = sizeof(coord);
            for (int i = 0; i < sizeof(vertexSize); i++) {
                totalInfo.push_back(*(reinterpret_cast<char *>(&vertexSize) + i));
            }

            std::uint32_t vertexByteCount = vertices.size();
            for (int i = 0; i < sizeof(vertexByteCount); i++) {
                totalInfo.push_back(*(reinterpret_cast<char*>(&vertexByteCount) + i));
            }

            for (auto &vertex : vertices) {
                for (int i = 0; i < sizeof(vertex); i++) {
                    totalInfo.push_back(*(reinterpret_cast<char*>(&vertex) + i));
                }
            }
        }

        {
            std::uint8_t indexSize = sizeof(index);
            for (int i = 0; i < sizeof(indexSize); i++) {
                totalInfo.push_back(*(reinterpret_cast<char*>(&indexSize) + i));
            }

            std::uint32_t indexByteCount = indices.size();
            for (int i = 0; i < sizeof(indexByteCount); i++) {
                totalInfo.push_back(*(reinterpret_cast<char*>(&indexByteCount) + i));
            }

            for (auto &index : indices) {
                for (int i = 0; i < sizeof(index); i++) {
                    totalInfo.push_back(*(reinterpret_cast<char*>(&index) + i));
                }
            }
        }

        {
            std::uint8_t rgbSize = sizeof(rgb);
            for (int i = 0; i < sizeof(rgbSize); i++) {
                totalInfo.push_back(*(reinterpret_cast<char*>(&rgbSize) + i));
            }

            std::uint32_t colourByteCount = colours.size() * 3;
            for (int i = 0; i < sizeof(colourByteCount); i++) {
                totalInfo.push_back(*(reinterpret_cast<char*>(&colourByteCount) + i));
            }

            for (auto &rgb : colours) {
                for (int i = 0; i < sizeof(rgb); i++) {
                    totalInfo.push_back(*(reinterpret_cast<char*>(&rgb) + i));
                }
            }
        }

        std::ofstream out((outPath / file.filename()).replace_extension(".triangulated"), std::ios::binary);
        out.write(totalInfo.data(), totalInfo.size());
        out.close();
    }
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
            std::cout << "Processing " << file.path().filename() << "... ";
            auto begin = std::chrono::high_resolution_clock::now();
            processSVG(file.path(), output);
            auto delta = std::chrono::high_resolution_clock::now() - begin;
            std::cout << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() << " milliseconds\n";
        }
    }

    return 0;
}
