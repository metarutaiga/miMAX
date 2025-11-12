#pragma once

#include <map>
#include "miMAX.h"
#include "internal.h"

static constexpr uint64_t class64(ClassID classID)
{
    return (uint64_t)classID.first | ((uint64_t)classID.second << 32);
}

static inline bool checkClass(Print log, Chunk const& chunk, ClassID classID, SuperClassID superClassID)
{
    auto& classData = chunk.classData;
    if (classData.classID == classID && classData.superClassID == superClassID)
        return true;
    char const* format;
    if (classData.dllIndex == 0xFFFFFFFF) {
        format = "Unknown (%08X-%08X-%08X-%08X) %-24s\n";
    }
    else {
        format = "Unknown (%08X-%08X-%08X-%08X) %-24s %-16s %s\n";
    }
    log(format, classData.dllIndex, classData.classID.first, classData.classID.second, classData.superClassID, chunk.name.c_str(), chunk.classDllFile.c_str(), chunk.classDllName.c_str());
    return false;
}

template<typename... Args>
static inline Chunk const* getChunk(Chunk const& chunk, Args&&... args)
{
    auto* output = &chunk;
    for (uint16_t type : { args... }) {
        auto it = std::find_if(output->begin(), output->end(), [type](auto& chunk) {
            return chunk.type == type;
        });
        if (it == output->end())
            return nullptr;
        output = &(*it);
    }
    return output;
}

template<typename T = char, typename... Args>
static inline std::vector<T> getProperty(Chunk const& chunk, Args&&... args)
{
    for (uint16_t type : { args... }) {
        auto* found = getChunk(chunk, type);
        if (found == nullptr)
            continue;
        T* data = (T*)found->property.data();
        size_t size = found->property.size() / sizeof(T);
        return std::vector<T>(data, data + size);
    }
    return std::vector<T>();
}

template<class order = std::less<uint32_t>>
static inline std::map<uint32_t, uint32_t, order> getLink(Chunk const& chunk)
{
    std::map<uint32_t, uint32_t, order> link;
    auto propertyLink2034 = getProperty<uint32_t>(chunk, 0x2034);
    auto propertyLink2035 = getProperty<uint32_t>(chunk, 0x2035);
    for (uint32_t i = 0; i < propertyLink2034.size(); ++i)
        link[i] = propertyLink2034[i];
    for (uint32_t i = 1; i + 1 < propertyLink2035.size(); i += 2)
        link[propertyLink2035[i + 0]] = propertyLink2035[i + 1];
    return link;
}

template<typename... Args>
static inline Chunk const* getLinkChunk(Chunk const& scene, Chunk const& chunk, Args&&... args)
{
    auto* output = &chunk;
    auto link = getLink(*output);
    for (uint32_t index : { args... }) {
        auto it = link.find(index);
        if (it == link.end())
            return nullptr;
        if (scene.size() <= (*it).second)
            return nullptr;
        auto& chunk = scene[(*it).second];
        output = &chunk;
        link = getLink(*output);
    }
    return output;
}

static inline std::vector<std::tuple<float, int, miMAXNode::Point3>> getParamBlock(Chunk const& paramBlock)
{
    std::vector<std::tuple<float, int, Point3>> output;
    switch (paramBlock.classData.superClassID) {
    case PARAMETER_BLOCK_SUPERCLASS_ID: {
        auto propertyCount = getProperty<int>(paramBlock, 0x0001);
        unsigned int count = propertyCount.empty() ? 0 : propertyCount.front();
        for (auto& chunk : paramBlock) {
            if (output.size() >= count)
                break;
            if (chunk.type == 0x0002) {
                auto propertyFloat = getProperty<float>(chunk, 0x0100);
                auto propertyInt = getProperty<int>(chunk, 0x0101);
                auto propertyRGBA = getProperty<Point3>(chunk, 0x0102);
                auto propertyPoint3 = getProperty<Point3>(chunk, 0x0103);
                auto propertyBool = getProperty<bool>(chunk, 0x0104);
                output.push_back({});
                auto& [f, i, p] = output.back();
                if (propertyFloat.empty() == false)     f = propertyFloat.front();
                if (propertyInt.empty() == false)       i = propertyInt.front();
                if (propertyRGBA.empty() == false)      p = propertyRGBA.front();
                if (propertyPoint3.empty() == false)    p = propertyPoint3.front();
                if (propertyBool.empty() == false)      i = propertyBool.front();
            }
        }
        break;
    }
    case PARAMETER_BLOCK2_SUPERCLASS_ID:
        for (auto& chunk : paramBlock) {
            if (chunk.property.size() < 19)
                continue;
            if (chunk.type != 0x000E && chunk.type != 0x100E)
                continue;
            uint16_t index = 0;
            uint32_t type = 0;
            memcpy(&index, chunk.property.data() + 0, sizeof(uint16_t));
            memcpy(&type, chunk.property.data() + 2, sizeof(uint32_t));
            if (output.size() <= index)
                output.resize(index + 1);
            auto& [f, i, p] = output[index];
            switch (type) {
            case 0: // TYPE_FLOAT
            case 5: // TYPE_ANGLE
            case 6: // TYPE_PCNT_FRAC
            case 7: // TYPE_WORLD
                memcpy(&f, chunk.property.data() + 15, sizeof(float));
                break;
            case 1: // TYPE_INT
            case 4: // TYPE_BOOL
                memcpy(&i, chunk.property.data() + 15, sizeof(int));
                break;
            case 2: // TYPE_RGBA
            case 3: // TYPE_POINT3
                memcpy(&p, chunk.property.data() + 15, sizeof(Point3));
                break;
            }
        }
        break;
    default:
        break;
    }
    return output;
}
