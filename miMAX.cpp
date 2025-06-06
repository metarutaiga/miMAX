/*
    2012 Kaetemi https://blog.kaetemi.be
    2025 TAiGA   https://github.com/metarutaiga/miMAX
*/
#include <stdio.h>
#include <functional>
#include <map>
#include <tuple>

#define __MIMAX_INTERNAL__
#include "miMAX.h"

#if _CPPUNWIND || __cpp_exceptions
#include <exception>
#define TRY         try {
#define CATCH(x)    } catch(x)
#define THROW       throw std::runtime_error(__FILE_NAME__ ":" _LIBCPP_TOSTRING(__LINE__))
#else
#include <setjmp.h>
thread_local jmp_buf compoundfilereader_jmp_buf = {};
#define TRY         if (setjmp(compoundfilereader_jmp_buf) == 0) {
#define CATCH(x)    } else
#define THROW       longjmp(compoundfilereader_jmp_buf, 1);
#define throw       longjmp(compoundfilereader_jmp_buf, 1);
#endif
#include "compoundfilereader/src/include/compoundfilereader.h"
#include "compoundfilereader/src/include/utf.h"

#if defined(__APPLE__)
#include <zlib.h>
#endif

#define FLOAT_TYPE          0x2501
#define POINT3_TYPE         0x2503
#define QUAT_TYPE           0x2504
#define SCALEVALUE_TYPE     0x2505

template <typename... Args>
static std::string format(char const* format, Args&&... args)
{
    std::string output;
    size_t length = snprintf(nullptr, 0, format, args...) + 1;
    output.resize(length);
    snprintf(output.data(), length, format, args...);
    output.pop_back();
    return output;
}

static std::vector<char> uncompress(std::vector<char> const& input)
{
#if defined(__APPLE__)
    if (input.size() < 10 || input[0] != char(0x1F) || input[1] != char(0x8B))
        return input;

    z_stream stream = {};
    stream.next_in = (Bytef*)input.data();
    stream.avail_in = (uInt)input.size();
    inflateInit2(&stream, MAX_WBITS | 32);

    std::vector<char> output(input.size());
    stream.next_out = (Bytef*)output.data();
    stream.avail_out = (uint)output.size();
    for (;;) {
        int result = inflate(&stream, Z_NO_FLUSH);
        if (result == Z_BUF_ERROR) {
            output.push_back(0);
            output.resize(output.capacity());
            stream.next_out = (Bytef*)(output.data() + stream.total_out);
            stream.avail_out = (uint)(output.size() - stream.total_out);
            continue;
        }
        if (result == Z_STREAM_END) {
            output.resize(stream.total_out);
            inflateEnd(&stream);
            return output;
        }
        if (result == Z_OK) {
            continue;
        }
        break;
    }

    inflateEnd(&stream);
    return input;
#else
    return input;
#endif
}

static constexpr uint64_t class64(ClassID classID)
{
    return (uint64_t)classID.first | ((uint64_t)classID.second << 32);
}

static void parseStream(Chunk& chunk, char const* begin, char const* end)
{
    bool children = false;
    uint16_t type = 0;
    uint64_t length = 0;
    for (;;) {
        char const* header = begin;
        if (begin + 6 > end)
            break;
        children = false;
        memcpy(&type, begin, 2); begin += 2;
        memcpy(&length, begin, 4); begin += 4;
        if (length == 0) {
            if (begin + 8 > end)
                break;
            memcpy(&length, begin, 8); begin += 8;
            if (length == 0)
                break;
            if (length & 0x8000000000000000ull) {
                length &= 0x7FFFFFFFFFFFFFFFull;
                children = true;
            }
        }
        else if (length & 0x80000000ull) {
            length &= 0x7FFFFFFFull;
            children = true;
        }
        char const* next = (header + length);
        if (next > end)
            break;
        Chunk child;
        child.type = type;
        child.name = format("%04X", type);
        if (children) {
            parseStream(child, begin, next);
        }
        else {
            child.property.assign(begin, next);
        }
        chunk.emplace_back(std::move(child));
        begin = next;
    }
}

template <typename... Args>
static Chunk const* getChunk(Chunk const& chunk, Args&&... args)
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

template <typename T = char, typename... Args>
static std::vector<T> getProperty(Chunk const& chunk, Args&&... args)
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

static std::tuple<std::string, ClassData> getClass(Chunk const& classDirectory, uint16_t classIndex)
{
    if (classDirectory.size() <= classIndex)
        return {};
    auto& chunk = classDirectory[classIndex];
    auto propertyClassName = getProperty<uint16_t>(chunk, 0x2042);
    auto propertyClassChunk = getProperty<ClassData>(chunk, 0x2060);
    if (propertyClassChunk.empty())
        return {};
    if (propertyClassName.empty())
        return { "(Unnamed)", *propertyClassChunk.data() };
    return { UTF16ToUTF8(propertyClassName.data(), propertyClassName.size()), *propertyClassChunk.data() };
}

static std::tuple<std::string, std::string> getDll(Chunk const& dllDirectory, uint32_t dllIndex)
{
    if (dllIndex == UINT32_MAX)
        return { "(Internal)", "(Internal)" };
    if (dllDirectory.size() <= dllIndex)
        return { "(Unknown)", "(Unknown)" };
    auto& chunk = dllDirectory[dllIndex];
    auto propertyDllFile = getProperty<uint16_t>(chunk, 0x2037);
    auto propertyDllName = getProperty<uint16_t>(chunk, 0x2039);
    if (propertyDllFile.empty() || propertyDllName.empty())
        return { "(Unknown)", "(Unknown)" };
    return { UTF16ToUTF8(propertyDllFile.data(), propertyDllFile.size()), UTF16ToUTF8(propertyDllName.data(), propertyDllName.size()) };
}

template <class order = std::less<uint32_t>>
static std::map<uint32_t, uint32_t, order> getLink(Chunk const& chunk)
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

template <typename... Args>
static Chunk const* getLinkChunk(Chunk const& scene, Chunk const& chunk, Args&&... args)
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

static bool checkClass(int(*log)(char const*, ...), Chunk const& chunk, ClassID classID, SuperClassID superClassID)
{
    if (chunk.classData.classID == classID && chunk.classData.superClassID == superClassID)
        return true;
    auto& classData = chunk.classData;
    log("Unknown (%08X-%08X-%08X-%08X) %s", classData.dllIndex, classData.classID.first, classData.classID.second, classData.superClassID, chunk.name.c_str());
    return false;
}

static std::vector<std::tuple<float, int, Point3>> getParamBlock(Chunk const& paramBlock)
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

static void getPositionRotationScale(int(*log)(char const*, ...), Chunk const& scene, Chunk const& chunk, miMAXNode& node)
{
    // FFFFFFFF-00002005-00000000-00009008 Position/Rotation/Scale  PRS_CONTROL_CLASS_ID + MATRIX3_SUPERCLASS_ID
    if (checkClass(log, chunk, PRS_CONTROL_CLASS_ID, MATRIX3_SUPERCLASS_ID) == false)
        return;

    // ????????-00002007-00000000-00009003 Bezier Float     HYBRIDINTERP_FLOAT_CLASS_ID + FLOAT_SUPERCLASS_ID

    // FFFFFFFF-00002002-00000000-0000900B Linear Position  LININTERP_POSITION_CLASS_ID + POSITION_SUPERCLASS_ID
    // ????????-00002008-00000000-0000900B Bezier Position  HYBRIDINTERP_POSITION_CLASS_ID + POSITION_SUPERCLASS_ID
    // ????????-118F7E02-FFEE238A-0000900B Position XYZ     IPOS_CONTROL_CLASS_ID + POSITION_SUPERCLASS_ID
    // FFFFFFFF-00442312-00000000-0000900B TCB Position     TCBINTERP_POSITION_CLASS_ID + POSITION_SUPERCLASS_ID
    for (uint32_t i = 0; i < 1; ++i) {
        auto* position = getLinkChunk(scene, chunk, 0);
        if (position == nullptr)
            continue;
        auto& classData = position->classData;
        switch (class64(classData.classID) | (classData.superClassID == POSITION_SUPERCLASS_ID ? 0 : -1)) {
        case class64(IPOS_CONTROL_CLASS_ID):
            for (uint32_t i = 0; i < 3; ++i) {
                auto* array = getLinkChunk(scene, *position, i);
                if (array == nullptr)
                    continue;
                if (checkClass(log, *array, HYBRIDINTERP_FLOAT_CLASS_ID, FLOAT_SUPERCLASS_ID) == false)
                    continue;
                auto* chunk7127 = getChunk(*array, 0x7127);
                if (chunk7127)
                    array = chunk7127;
                auto propertyBezierFloat = getProperty<std::tuple<uint32_t, uint32_t, BezierFloat>>(*array, 0x2525);
                for (auto& [time, flags, bezierFloat] : propertyBezierFloat) {
                    node.keyPosition[i].emplace_back(time, bezierFloat);
                }
                auto propertyFloat = getProperty<float>(*array, FLOAT_TYPE);
                if (propertyFloat.size() >= 1) {
                    node.position[i] = propertyFloat[0];
                    continue;
                }
                log("Value is not found (%s)", array->name.c_str());
            }
            continue;
        case class64(LININTERP_POSITION_CLASS_ID):
        case class64(HYBRIDINTERP_POSITION_CLASS_ID):
        case class64(TCBINTERP_POSITION_CLASS_ID): {
            auto* chunk7127 = getChunk(*position, 0x7127);
            if (chunk7127)
                position = chunk7127;
            auto propertyFloat = getProperty<float>(*position, POINT3_TYPE);
            if (propertyFloat.size() >= 3) {
                node.position[0] = propertyFloat[0];
                node.position[1] = propertyFloat[1];
                node.position[2] = propertyFloat[2];
                continue;
            }
            log("Value is not found (%s)", position->name.c_str());
            continue;
        }
        default:
            break;
        }
        checkClass(log, *position, {}, 0);
    }

    // FFFFFFFF-00002003-00000000-0000900C Linear Rotation  LININTERP_ROTATION_CLASS_ID + ROTATION_SUPERCLASS_ID
    // ????????-00002012-00000000-0000900C Euler XYZ        HYBRIDINTERP_POINT4_CLASS_ID + ROTATION_SUPERCLASS_ID
    // FFFFFFFF-00442313-00000000-0000900C TCB Rotation     TCBINTERP_ROTATION_CLASS_ID + ROTATION_SUPERCLASS_ID
    for (uint32_t i = 0; i < 1; ++i) {
        auto* rotation = getLinkChunk(scene, chunk, 1);
        if (rotation == nullptr)
            continue;
        auto& classData = rotation->classData;
        switch (class64(classData.classID) | (classData.superClassID == ROTATION_SUPERCLASS_ID ? 0 : -1)) {
        case class64(HYBRIDINTERP_POINT4_CLASS_ID):
            for (uint32_t i = 0; i < 3; ++i) {
                auto* array = getLinkChunk(scene, *rotation, i);
                if (array == nullptr)
                    continue;
                if (checkClass(log, *array, HYBRIDINTERP_FLOAT_CLASS_ID, FLOAT_SUPERCLASS_ID) == false)
                    continue;
                auto* chunk7127 = getChunk(*array, 0x7127);
                if (chunk7127)
                    array = chunk7127;
                auto propertyBezierFloat = getProperty<std::tuple<uint32_t, uint32_t, BezierFloat>>(*array, 0x2525);
                for (auto& [time, flags, bezierFloat] : propertyBezierFloat) {
                    node.keyRotation[i].emplace_back(time, bezierFloat);
                }
                auto propertyFloat = getProperty<float>(*array, FLOAT_TYPE);
                if (propertyFloat.size() >= 1) {
                    node.rotation[i] = propertyFloat[0];
                    continue;
                }
                log("Value is not found (%s)", array->name.c_str());
            }
            miMAXEulerToQuaternion(node.rotation.data(), node.rotation);
            continue;
        case class64(LININTERP_ROTATION_CLASS_ID):
        case class64(TCBINTERP_ROTATION_CLASS_ID): {
            auto* chunk7127 = getChunk(*rotation, 0x7127);
            if (chunk7127)
                rotation = chunk7127;
            auto propertyFloat = getProperty<float>(*rotation, POINT3_TYPE, QUAT_TYPE);
            if (propertyFloat.size() >= 4) {
                node.rotation[0] = propertyFloat[0];
                node.rotation[1] = propertyFloat[1];
                node.rotation[2] = propertyFloat[2];
                node.rotation[3] = propertyFloat[3];
                continue;
            }
            if (propertyFloat.size() >= 3) {
                miMAXEulerToQuaternion(propertyFloat.data(), node.rotation);
                continue;
            }
            log("Value is not found (%s)", rotation->name.c_str());
            continue;
        }
        default:
            break;
        }
        checkClass(log, *rotation, {}, 0);
    }

    // FFFFFFFF-00002004-00000000-0000900D Linear Scale LININTERP_SCALE_CLASS_ID + SCALE_SUPERCLASS_ID
    // FFFFFFFF-00002010-00000000-0000900D Bezier Scale HYBRIDINTERP_SCALE_CLASS_ID + SCALE_SUPERCLASS_ID
    // FFFFFFFF-00442315-00000000-0000900D TCB Scale    TCBINTERP_SCALE_CLASS_ID + SCALE_SUPERCLASS_ID
    for (uint32_t i = 0; i < 1; ++i) {
        auto* scale = getLinkChunk(scene, chunk, 2);
        if (scale == nullptr)
            continue;
        auto& classData = scale->classData;
        switch (class64(classData.classID) | (classData.superClassID == SCALE_SUPERCLASS_ID ? 0 : -1)) {
        case class64(LININTERP_SCALE_CLASS_ID):
        case class64(HYBRIDINTERP_SCALE_CLASS_ID):
        case class64(TCBINTERP_SCALE_CLASS_ID): {
            auto* chunk7127 = getChunk(*scale, 0x7127);
            if (chunk7127)
                scale = chunk7127;
            auto propertyBezierFloat = getProperty<std::tuple<uint32_t, uint32_t, BezierFloat>>(*scale, 0x2525);
            for (auto& [time, flags, bezierFloat] : propertyBezierFloat) {
                node.keyScale.emplace_back(time, bezierFloat);
            }
            auto propertyFloat = getProperty<float>(*scale, FLOAT_TYPE, SCALEVALUE_TYPE);
            if (propertyFloat.size() >= 3) {
                node.scale[0] = propertyFloat[0];
                node.scale[1] = propertyFloat[1];
                node.scale[2] = propertyFloat[2];
                continue;
            }
            if (propertyFloat.size() >= 1) {
                node.scale[0] = node.scale[1] = node.scale[2] = propertyFloat[0];
                continue;
            }
            log("Value is not found (%s)", scale->name.c_str());
            continue;
        }
        default:
            break;
        }
        checkClass(log, *scale, {}, 0);
    }
}

static void getObjectSpaceModifier(int(*log)(char const*, ...), Chunk const& scene, Chunk const& chunk, Chunk const& modifierChunk, miMAXNode& node)
{
    if (chunk.classData.superClassID != OSM_SUPERCLASS_ID)
        return;
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return;
    auto paramBlock = getParamBlock(*pParamBlock);

    // ????????-4AA52AE3-35CA1CDE-00000810  EDIT_NORMALS_CLASS_ID + OSM_SUPERCLASS_ID
    // ????????-7EBB4645-7BE2044B-00000810  PAINTLAYERMOD_CLASS_ID + OSM_SUPERCLASS_ID
    switch (class64(chunk.classData.classID)) {
    case class64(EDIT_NORMALS_CLASS_ID): {
        auto* pNormalChunk = getChunk(modifierChunk, 0x2512, 0x0240);
        if (pNormalChunk == nullptr)
            pNormalChunk = getChunk(modifierChunk, 0x2512, 0x0250);
        if (pNormalChunk == nullptr)
            break;
        auto normals = getProperty<float>(*pNormalChunk, 0x0110);
        if (normals.empty())
            break;
        for (size_t i = 1; i + 2 < normals.size(); i += 3) {
            node.normal.push_back({normals[i], normals[i + 1], normals[1 + 2]});
        }
        node.text += format("Normal : %zd", node.normal.size()) + '\n';
        break;
    }
    case class64(PAINTLAYERMOD_CLASS_ID):
        if (paramBlock.size() > 1) {
            auto* pColorChunk = getChunk(modifierChunk, 0x2512);
            if (pColorChunk == nullptr)
                break;
            switch (std::get<int>(paramBlock[1])) {
            default:
                node.vertexColor = getProperty<Point3>(*pColorChunk, 0x0110);
                node.text += format("Vertex Color : %zd", node.vertexColor.size()) + '\n';
                break;
            case -1:
                node.vertexIllum = getProperty<Point3>(*pColorChunk, 0x0110);
                node.text += format("Vertex Illum : %zd", node.vertexIllum.size()) + '\n';
                break;
            case -2:
                node.vertexAlpha = getProperty<Point3>(*pColorChunk, 0x0110);
                node.text += format("Vertex Alpha : %zd", node.vertexAlpha.size()) + '\n';
                break;
            }
        }
        break;
    default:
        break;
    }
}

static void getPrimitive(int(*log)(char const*, ...), Chunk const& scene, Chunk const& chunk, miMAXNode& node)
{
    auto* pChunk = &chunk;
    if ((*pChunk).classData.superClassID != GEOMOBJECT_SUPERCLASS_ID) {
        if ((*pChunk).type != 0x2032)
            return;
        auto link = getLink<std::greater<uint32_t>>(*pChunk);
        for (auto [linkIndex, chunkIndex] : link) {
            if (scene.size() <= chunkIndex)
                continue;
            auto& chunk = scene[chunkIndex];
            if (pChunk == &chunk)
                continue;
            if (chunk.classData.superClassID == OSM_SUPERCLASS_ID) {
                size_t index = 0;
                Chunk const* pModifierChunk = nullptr;
                for (auto& child : (*pChunk)) {
                    if (child.type == 0x2500) {
                        if (index == linkIndex) {
                            pModifierChunk = &child;
                            break;
                        }
                        index++;
                    }
                }
                if (pModifierChunk == nullptr)
                    continue;
                getObjectSpaceModifier(log, scene, chunk, *pModifierChunk, node);
                continue;
            }
            getPrimitive(log, scene, chunk, node);
        }
        return;
    }
    auto* pParamBlock = getLinkChunk(scene, *pChunk, 0);
    if (pParamBlock == nullptr)
        return;
    auto paramBlock = getParamBlock(*pParamBlock);

    // ????????-00000010-00000000-00000010 Box              BOXOBJ_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-00000011-00000000-00000010 Sphere           SPHERE_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-00000012-00000000-00000010 Cylinder         CYLINDER_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-00000020-00000000-00000010 Torus            TORUS_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-a86c23dd-00000000-00000010 Cone             CONE_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-00000000-00007f9e-00000010 GeoSphere        GSPHERE_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-00007b21-00000000-00000010 Tube             TUBE_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-76bf318a-4bf37b10-00000010 Pyramid          PYRAMID_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-081f1dfc-77566f65-00000010 Plane            PLANE_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-e44f10b3-00000000-00000010 Editable Mesh    EDITTRIOBJ_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    // ????????-1bf8338d-192f6098-00000010 Editable Poly    EPOLYOBJ_CLASS_ID + GEOMOBJECT_SUPERCLASS_ID
    switch (class64((*pChunk).classData.classID)) {
    case class64(BOXOBJ_CLASS_ID):
        if (paramBlock.size() > 5) {
            float length = std::get<float>(paramBlock[0]);
            float width = std::get<float>(paramBlock[1]);
            float height = std::get<float>(paramBlock[2]);
            int lengthSegments = std::get<int>(paramBlock[3]);
            int widthSegments = std::get<int>(paramBlock[4]);
            int heightSegments = std::get<int>(paramBlock[5]);

            node.vertex = {
                { -length, -width, -height },
                {  length, -width, -height },
                { -length,  width, -height },
                {  length,  width, -height },
                { -length, -width,  height },
                {  length, -width,  height },
                { -length,  width,  height },
                {  length,  width,  height },
            };

            node.text += format("Primitive : %s", "Box") + '\n';
            node.text += format("Length : %f", length) + '\n';
            node.text += format("Width : %f", width) + '\n';
            node.text += format("Height : %f", height) + '\n';
            node.text += format("Length Segments : %d", lengthSegments) + '\n';
            node.text += format("Width Segments : %d", widthSegments) + '\n';
            node.text += format("Height Segments : %d", heightSegments) + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(SPHERE_CLASS_ID):
        if (paramBlock.size() > 4) {
            float radius = std::get<float>(paramBlock[0]);
            int segments = std::get<int>(paramBlock[1]);
            bool smooth = std::get<int>(paramBlock[2]);
            float hemisphere = std::get<float>(paramBlock[3]);
            int chopSquash = std::get<int>(paramBlock[4]);

            node.text += format("Primitive : %s", "Sphere") + '\n';
            node.text += format("Radius : %f", radius) + '\n';
            node.text += format("Segments : %d", segments) + '\n';
            node.text += format("Smooth : %s", smooth ? "true" : "false") + '\n';
            node.text += format("Hemisphere : %f", hemisphere) + '\n';
            node.text += format("ChopSquash : %s", chopSquash == 0 ? "Chop" : "Squash") + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(CYLINDER_CLASS_ID):
        if (paramBlock.size() > 5) {
            float radius = std::get<float>(paramBlock[0]);
            float height = std::get<float>(paramBlock[1]);
            int heightSegments = std::get<int>(paramBlock[2]);
            int capSegments = std::get<int>(paramBlock[3]);
            int sides = std::get<int>(paramBlock[4]);
            bool smooth = std::get<int>(paramBlock[5]);

            node.text += format("Primitive : %s", "Cylinder") + '\n';
            node.text += format("Radius : %f", radius) + '\n';
            node.text += format("Height : %f", height) + '\n';
            node.text += format("Height Segments : %d", heightSegments) + '\n';
            node.text += format("Cap Segments : %d", capSegments) + '\n';
            node.text += format("Sides : %d", sides) + '\n';
            node.text += format("Smooth : %s", smooth ? "true" : "false") + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(TORUS_CLASS_ID):
        if (paramBlock.size() > 6) {
            float radius1 = std::get<float>(paramBlock[0]);
            float radius2 = std::get<float>(paramBlock[1]);
            float rotation = std::get<float>(paramBlock[2]);
            float twist = std::get<float>(paramBlock[3]);
            int segments = std::get<int>(paramBlock[4]);
            int sides = std::get<int>(paramBlock[5]);
            int smooth = std::get<int>(paramBlock[6]);

            node.text += format("Primitive : %s", "Torus") + '\n';
            node.text += format("Radius1 : %f", radius1) + '\n';
            node.text += format("Radius2 : %f", radius2) + '\n';
            node.text += format("Rotation : %f", rotation) + '\n';
            node.text += format("Twist : %f", twist) + '\n';
            node.text += format("Segments : %d", segments) + '\n';
            node.text += format("Sides : %d", sides) + '\n';
            node.text += format("Smooth : %d", smooth) + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(CONE_CLASS_ID):
        if (paramBlock.size() > 6) {
            float radius1 = std::get<float>(paramBlock[0]);
            float radius2 = std::get<float>(paramBlock[1]);
            float height = std::get<float>(paramBlock[2]);
            int heightSegments = std::get<int>(paramBlock[3]);
            int capSegments = std::get<int>(paramBlock[4]);
            int sides = std::get<int>(paramBlock[5]);
            bool smooth = std::get<int>(paramBlock[6]);

            node.text += format("Primitive : %s", "Cone") + '\n';
            node.text += format("Radius1 : %f", radius1) + '\n';
            node.text += format("Radius2 : %f", radius2) + '\n';
            node.text += format("Height : %f", height) + '\n';
            node.text += format("Height Segments : %d", heightSegments) + '\n';
            node.text += format("Cap Segments : %d", capSegments) + '\n';
            node.text += format("Sides : %d", sides) + '\n';
            node.text += format("Smooth : %s", smooth ? "true" : "false") + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(GSPHERE_CLASS_ID):
        if (paramBlock.size() > 4) {
            float radius = std::get<float>(paramBlock[0]);
            int segments = std::get<int>(paramBlock[1]);
            int geodesicBaseType = std::get<int>(paramBlock[2]);
            bool smooth = std::get<int>(paramBlock[3]);
            bool hemisphere = std::get<int>(paramBlock[4]);

            node.text += format("Primitive : %s", "GeoSphere") + '\n';
            node.text += format("Radius : %f", radius) + '\n';
            node.text += format("Segments : %d", segments) + '\n';
            node.text += format("Geodesic Base Type : %d", geodesicBaseType) + '\n';
            node.text += format("Smooth : %f", smooth ? "true" : "false") + '\n';
            node.text += format("Hemisphere : %f", hemisphere ? "true" : "false") + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(TUBE_CLASS_ID):
        if (paramBlock.size() > 6) {
            float radius1 = std::get<float>(paramBlock[0]);
            float radius2 = std::get<float>(paramBlock[1]);
            float height = std::get<float>(paramBlock[2]);
            int heightSegments = std::get<int>(paramBlock[3]);
            int capSegments = std::get<int>(paramBlock[4]);
            int sides = std::get<int>(paramBlock[5]);
            bool smooth = std::get<int>(paramBlock[6]);

            node.text += format("Primitive : %s", "Tube") + '\n';
            node.text += format("Radius1 : %f", radius1) + '\n';
            node.text += format("Radius2 : %f", radius2) + '\n';
            node.text += format("Height : %f", height) + '\n';
            node.text += format("Height Segments : %d", heightSegments) + '\n';
            node.text += format("Cap Segments : %d", capSegments) + '\n';
            node.text += format("Sides : %d", sides) + '\n';
            node.text += format("Smooth : %s", smooth ? "true" : "false") + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(PYRAMID_CLASS_ID):
        if (paramBlock.size() > 5) {
            float width = std::get<float>(paramBlock[0]);
            float depth = std::get<float>(paramBlock[1]);
            float height = std::get<float>(paramBlock[2]);
            int widthSegments = std::get<int>(paramBlock[3]);
            int depthSegments = std::get<int>(paramBlock[4]);
            int heightSegments = std::get<int>(paramBlock[5]);

            node.text += format("Primitive : %s", "Pyramid") + '\n';
            node.text += format("Width : %f", width) + '\n';
            node.text += format("Depth : %f", depth) + '\n';
            node.text += format("Height : %f", height) + '\n';
            node.text += format("Width Segments : %d", widthSegments) + '\n';
            node.text += format("Depth Segments : %d", depthSegments) + '\n';
            node.text += format("Height Segments : %d", heightSegments) + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(PLANE_CLASS_ID):
        if (paramBlock.size() > 3) {
            float length = std::get<float>(paramBlock[0]);
            float width = std::get<float>(paramBlock[1]);
            int lengthSegments = std::get<int>(paramBlock[2]);
            int widthSegments = std::get<int>(paramBlock[3]);

            node.vertex = {
                { -length, -width, 0 },
                {  length, -width, 0 },
                { -length,  width, 0 },
                {  length,  width, 0 },
            };

            node.text += format("Primitive : %s", "Plane") + '\n';
            node.text += format("Length : %f", length) + '\n';
            node.text += format("Width : %f", width) + '\n';
            node.text += format("Length Segments : %d", lengthSegments) + '\n';
            node.text += format("Width Segments : %d", widthSegments) + '\n';
            return;
        }
        checkClass(log, *pParamBlock, {}, 0);
        break;
    case class64(EDITTRIOBJ_CLASS_ID): {
        auto* pPolyChunk = getChunk(*pChunk, 0x08FE);
        if (pPolyChunk == nullptr)
            break;
        auto& polyChunk = (*pPolyChunk);

        auto vertexArray = getProperty<int>(polyChunk, 0x0912);
        for (size_t i = 1; i + 4 < vertexArray.size(); i += 5) {
            node.vertexArray.push_back({});
            node.vertexArray.back().push_back(vertexArray[i]);
            node.vertexArray.back().push_back(vertexArray[i + 1]);
            node.vertexArray.back().push_back(vertexArray[i + 2]);
        }

        auto vertex = getProperty<float>(polyChunk, 0x0914);
        for (size_t i = 1; i + 2 < vertex.size(); i += 3) {
            node.vertex.push_back({vertex[i], vertex[i + 1], vertex[i + 2]});
        }

        auto texture = getProperty<float>(polyChunk, 0x0916);
        if (texture.empty())
            texture = getProperty<float>(polyChunk, 0x2394);
        for (size_t i = 1; i + 2 < texture.size(); i += 3) {
            node.texture.push_back({texture[i], texture[i + 1], texture[i + 2]});
        }

        auto textureArray = getProperty<int>(polyChunk, 0x0918);
        if (textureArray.empty())
            textureArray = getProperty<int>(polyChunk, 0x2396);
        for (size_t i = 1; i + 2 < textureArray.size(); i += 3) {
            node.textureArray.push_back({});
            node.textureArray.back().push_back(textureArray[i]);
            node.textureArray.back().push_back(textureArray[i + 1]);
            node.textureArray.back().push_back(textureArray[i + 2]);
        }

        if (node.vertexArray.size() && node.textureArray.size()) {
            if (node.vertexArray.size() != node.textureArray.size()) {
                log("%s is corrupted (%zd:%zd)", "Editable Mesh", node.vertexArray.size(), node.textureArray.size());
            }
        }

        size_t totalVertexArray = 0;
        size_t totalTextureArray = 0;
        for (auto& array : node.vertexArray) {
            totalVertexArray += array.size();
        }
        for (auto& array : node.textureArray) {
            totalTextureArray += array.size();
        }

        node.text += format("Primitive : %s", "Editable Mesh") + '\n';
        node.text += format("Vertex : %zd", node.vertex.size()) + '\n';
        node.text += format("Texture : %zd", node.texture.size()) + '\n';
        node.text += format("Vertex Array : %zd (%zd)", node.vertexArray.size(), totalVertexArray) + '\n';
        node.text += format("Texture Array : %zd (%zd)", node.textureArray.size(), totalTextureArray) + '\n';
        return;
    }
    case class64(EPOLYOBJ_CLASS_ID): {
        auto* pPolyChunk = getChunk(*pChunk, 0x08FE);
        if (pPolyChunk == nullptr)
            break;
        auto& polyChunk = (*pPolyChunk);

        auto vertex = getProperty<float>(polyChunk, 0x0100);
        for (size_t i = 1; i + 3 < vertex.size(); i += 4) {
            node.vertex.push_back({vertex[i + 1], vertex[i + 2], vertex[1 + 3]});
        }

        auto vertexArray = getProperty<uint16_t>(polyChunk, 0x011A);
        for (size_t i = 2; i + 1 < vertexArray.size(); i += 2) {
            uint32_t count = (vertexArray[i] | vertexArray[i + 1] << 16) * 2;
            if (i + 2 + count + 1 > vertexArray.size()) {
                log("%s is corrupted", "Editable Poly");
                break;
            }
            i += 2;
            node.vertexArray.push_back({});
            for (size_t j = i, list = i + count; j < list; j += 2) {
                node.vertexArray.back().push_back(vertexArray[j] | vertexArray[j + 1] << 16);
            }
            i += count;
            uint16_t flags = vertexArray[i];
            i += 1;
            if (flags & 0x01)   i += 2;
            if (flags & 0x08)   i += 1;
            if (flags & 0x10)   i += 2;
            if (flags & 0x20)   i += 2 * (count - 6);
            i -= 2;
        }

        auto texture = getProperty<float>(polyChunk, 0x0128);
        for (size_t i = 1; i + 2 < texture.size(); i += 3) {
            node.texture.push_back({texture[i], texture[i + 1], texture[1 + 2]});
        }

        auto textureArray = getProperty<uint32_t>(polyChunk, 0x012B);
        for (size_t i = 0; i < textureArray.size(); ++i) {
            uint32_t count = textureArray[i];
            if (i + 1 + count > textureArray.size()) {
                log("%s is corrupted", "Editable Poly");
                break;
            }
            i += 1;
            node.textureArray.push_back({});
            for (size_t j = i, list = i + count; j < list; ++j) {
                node.textureArray.back().push_back(textureArray[j]);
            }
            i += count;
            i -= 1;
        }

        if (node.vertexArray.size() && node.textureArray.size()) {
            bool corrupted = (node.vertexArray.size() != node.textureArray.size());
            if (corrupted == false) {
                for (size_t i = 0; i < node.vertexArray.size() && i < node.textureArray.size(); ++i) {
                    if (node.vertexArray[i].size() != node.textureArray[i].size()) {
                        corrupted = true;
                        break;
                    }
                }
            }
            if (corrupted) {
                log("%s is corrupted (%zd:%zd)", "Editable Poly", node.vertexArray.size(), node.textureArray.size());
            }
        }

        size_t totalVertexArray = 0;
        size_t totalTextureArray = 0;
        for (auto& array : node.vertexArray) {
            totalVertexArray += array.size();
        }
        for (auto& array : node.textureArray) {
            totalTextureArray += array.size();
        }

        node.text += format("Primitive : %s", "Editable Poly") + '\n';
        node.text += format("Vertex : %zd", node.vertex.size()) + '\n';
        node.text += format("Texture : %zd", node.texture.size()) + '\n';
        node.text += format("Vertex Array : %zd (%zd)", node.vertexArray.size(), totalVertexArray) + '\n';
        node.text += format("Texture Array : %zd (%zd)", node.textureArray.size(), totalTextureArray) + '\n';
        return;
    }
    default:
        break;
    }
    checkClass(log, *pChunk, {}, 0);
}

void miMAXEulerToQuaternion(float const euler[3], Quat& quaternion)
{
    float cx = cosf(euler[0] * 0.5f);
    float cy = cosf(euler[1] * 0.5f);
    float cz = cosf(euler[2] * 0.5f);
    float sx = sinf(euler[0] * 0.5f);
    float sy = sinf(euler[1] * 0.5f);
    float sz = sinf(euler[2] * 0.5f);
    quaternion[0] = (sx * cy * cz - cx * sy * sz);
    quaternion[1] = (cx * sy * cz + sx * cy * sz);
    quaternion[2] = (cx * cy * sz - sx * sy * cz);
    quaternion[3] = (cx * cy * cz + sx * sy * sz);
}

float miMAXBezier(BezierFloat const& left, BezierFloat const& right, float scale)
{
    float A = std::lerp(left[0], left[1], scale);
    float B = std::lerp(left[1], left[2], scale);
    float C = std::lerp(left[2], right[0], scale);
    float X = std::lerp(A, B, scale);
    float Y = std::lerp(B, C, scale);
    return std::lerp(X, Y, scale);
}

miMAXNode* miMAXOpenFile(char const* name, int(*log)(char const*, ...))
{
    FILE* file = fopen(name, "rb");
    if (file == nullptr) {
        log("File is not found", name);
        return nullptr;
    }

    miMAXNode* root = nullptr;

    TRY

    root = new miMAXNode;
    if (root == nullptr) {
        log("Out of memory");
        THROW;
    }

    std::vector<char> dataClassData;
    std::vector<char> dataClassDirectory;
    std::vector<char> dataConfig;
    std::vector<char> dataDllDirectory;
    std::vector<char> dataScene;
    std::vector<char> dataVideoPostQueue;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::vector<char> buffer(size);
    fread(buffer.data(), 1, size, file);
    fclose(file);
    file = nullptr;

    if (buffer.empty() == false) {
        CFB::CompoundFileReader cfbReader(buffer.data(), buffer.size());
        cfbReader.EnumFiles(cfbReader.GetRootEntry(), -1, [&](CFB::COMPOUND_FILE_ENTRY const* entry, CFB::utf16string const& dir, int level) {
            std::string name = UTF16ToUTF8(entry->name);
            std::vector<char>* data = nullptr;
            if (name == "ClassData")            data = &dataClassData;
            else if (name == "ClassDirectory")  data = &dataClassDirectory;
            else if (name == "ClassDirectory3") data = &dataClassDirectory;
            else if (name == "Config")          data = &dataConfig;
            else if (name == "DllDirectory")    data = &dataDllDirectory;
            else if (name == "Scene")           data = &dataScene;
            else if (name == "VideoPostQueue")  data = &dataVideoPostQueue;
            if (data) {
                data->resize(entry->size);
                cfbReader.ReadFile(entry, 0, data->data(), data->size());
                (*data) = uncompress(*data);
            }
        });
    }

    // Parse
    root->classData = new Chunk;
    root->classDirectory = new Chunk;
    root->config = new Chunk;
    root->dllDirectory = new Chunk;
    root->scene = new Chunk;
    root->videoPostQueue = new Chunk;
    parseStream(*root->classData, dataClassData.data(), dataClassData.data() + dataClassData.size());
    parseStream(*root->classDirectory, dataClassDirectory.data(), dataClassDirectory.data() + dataClassDirectory.size());
    parseStream(*root->config, dataConfig.data(), dataConfig.data() + dataConfig.size());
    parseStream(*root->dllDirectory, dataDllDirectory.data(), dataDllDirectory.data() + dataDllDirectory.size());
    parseStream(*root->scene, dataScene.data(), dataScene.data() + dataScene.size());
    parseStream(*root->videoPostQueue, dataVideoPostQueue.data(), dataVideoPostQueue.data() + dataVideoPostQueue.size());

    // Root
    if (root->scene->empty()) {
        log("Scene is empty");
        THROW;
    }
    auto& scene = root->scene->front();
    switch (scene.type) {
                    // [ ] 3ds Max 8
    case 0x200E:    // [x] 3ds Max 9
    case 0x200F:    // [x] 3ds Max 2008
                    // [ ] 3ds Max 2009
    case 0x2012:    // [x] 3ds Max 2010
                    // [ ] 3ds Max 2011
                    // [ ] 3ds Max 2012
                    // [ ] 3ds Max 2013
                    // [ ] 3ds Max 2014
    case 0x2020:    // [x] 3ds Max 2015
                    // [ ] 3ds Max 2016
                    // [ ] 3ds Max 2017
    case 0x2023:    // [x] 3ds Max 2018
        break;
    default:
        if (scene.type >= 0x2000)
            break;
        log("Scene type %04X is not supported", scene.type);
        THROW;
    }

    // First Pass
    for (uint32_t i = 0; i < scene.size(); ++i) {
        auto& chunk = scene[i];
        auto [className, classData] = getClass(*root->classDirectory, chunk.type);
        if (className.empty()) {
            if (chunk.type != 0x2032) {
                log("Class %04X is not found! (Chunk:%X)", chunk.type, i);
            }
            continue;
        }
        auto [dllFile, dllName] = getDll(*root->dllDirectory, classData.dllIndex);
        chunk.name = className;
        chunk.classData = classData;
        chunk.classDllFile = dllFile;
        chunk.classDllName = dllName;
    }

    // Second Pass
    std::map<uint32_t, miMAXNode*> nodes;
    for (uint32_t i = 0; i < scene.size(); ++i) {
        auto& chunk = scene[i];
        auto& className = chunk.name;
        auto& classData = chunk.classData;

        // FFFFFFFF-00000001-00000000-00000001 - Node
        // FFFFFFFF-00000002-00000000-00000001 - RootNode   BASENODE_SUPERCLASS_ID
        if (classData.superClassID != BASENODE_SUPERCLASS_ID)
            continue;

        miMAXNode node;

        // Parent
        std::vector<uint32_t> propertyParent = getProperty<uint32_t>(chunk, 0x0960);
        miMAXNode* parent = root;
        if (propertyParent.empty() == false) {
            uint32_t index = *propertyParent.data();
            miMAXNode* found = nodes[index];
            if (found) {
                parent = found;
            }
            else {
                log("Parent %d is not found! (Chunk:%d)", index, i);
            }
        }

        // Name
        std::vector<uint16_t> propertyName = getProperty<uint16_t>(chunk, 0x0962);
        if (propertyName.empty() == false) {
            node.name = UTF16ToUTF8(propertyName.data(), propertyName.size());
        }
        else {
            node.name = className;
        }

        // Link
        for (uint32_t i = 0; i < 4; ++i) {
            Chunk const* linkChunk = getLinkChunk(scene, chunk, i);
            if (linkChunk == nullptr)
                continue;
            switch (i) {
            case 0: getPositionRotationScale(log, scene, *linkChunk, node); break;
            case 1: getPrimitive(log, scene, *linkChunk, node);             break;
            }
        }

        // Text
        std::vector<uint16_t> propertyText = getProperty<uint16_t>(chunk, 0x0120);
        if (propertyText.empty() == false) {
            node.text = UTF16ToUTF8(propertyText.data(), propertyText.size());
        }

        // Attach
        parent->emplace_back(std::move(node));
        nodes[i] = &parent->back();
    }

    CATCH (std::exception const& e) {
#if _CPPUNWIND || __cpp_exceptions
        log("Exception : %s", e.what());
        log("\n");
#endif
        if (file) {
            fclose(file);
        }
        delete root;
        return nullptr;
    }

    return root;
}
