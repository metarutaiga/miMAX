/*
    2012 Kaetemi https://blog.kaetemi.be
    2025 TAiGA   https://github.com/metarutaiga/miMAX
*/
#include <stdio.h>
#include <cmath>
#include <functional>
#include <map>
#include <tuple>

#define __MIMAX_INTERNAL__
#include "miMAX.h"
#include "object/chunk.h"
#include "object/format.h"

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

static std::vector<std::pair<ClassID, bool(*)(Print, Chunk const&, Chunk const&, miMAXNode&)>> primitiveMap;

static constexpr uint64_t class64(ClassID classID)
{
    return (uint64_t)classID.first | ((uint64_t)classID.second << 32);
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

static bool checkClass(Print log, Chunk const& chunk, ClassID classID, SuperClassID superClassID)
{
    if (chunk.classData.classID == classID && chunk.classData.superClassID == superClassID)
        return true;
    auto& classData = chunk.classData;
    log("Unknown (%08X-%08X-%08X-%08X) %s\n", classData.dllIndex, classData.classID.first, classData.classID.second, classData.superClassID, chunk.name.c_str());
    return false;
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

static void getPositionRotationScale(Print log, Chunk const& scene, Chunk const& chunk, miMAXNode& node)
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
                log("Value is not found (%s)\n", array->name.c_str());
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
            log("Value is not found (%s)\n", position->name.c_str());
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
                log("Value is not found (%s)\n", array->name.c_str());
            }
            miMAXNode::EulerToQuaternion(node.rotation.data(), node.rotation);
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
                miMAXNode::EulerToQuaternion(propertyFloat.data(), node.rotation);
                continue;
            }
            log("Value is not found (%s)\n", rotation->name.c_str());
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
            log("Value is not found (%s)\n", scale->name.c_str());
            continue;
        }
        default:
            break;
        }
        checkClass(log, *scale, {}, 0);
    }
}

static void getObjectSpaceModifier(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& modifierChunk, miMAXNode& node)
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
        node.text = node.text + format("Normal : %zd", node.normal.size()) + '\n';
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
                node.text = node.text + format("Vertex Color : %zd", node.vertexColor.size()) + '\n';
                break;
            case -1:
                node.vertexIllum = getProperty<Point3>(*pColorChunk, 0x0110);
                node.text = node.text + format("Vertex Illum : %zd", node.vertexIllum.size()) + '\n';
                break;
            case -2:
                node.vertexAlpha = getProperty<Point3>(*pColorChunk, 0x0110);
                node.text = node.text + format("Vertex Alpha : %zd", node.vertexAlpha.size()) + '\n';
                break;
            }
        }
        break;
    default:
        break;
    }
}

static void getPrimitive(Print log, Chunk const& scene, Chunk const& chunk, miMAXNode& node)
{
    auto* pChunk = &chunk;
    if ((*pChunk).classData.superClassID != GEOMOBJECT_SUPERCLASS_ID) {
        if ((*pChunk).type != 0x2032) {
            checkClass(log, *pChunk, {}, 0);
            return;
        }
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

    auto classID = (*pChunk).classData.classID;
    auto value = decltype(primitiveMap)::value_type(classID, nullptr);
    auto it = std::lower_bound(primitiveMap.begin(), primitiveMap.end(), value);
    if (it != primitiveMap.end() && (*it).first == classID) {
        if ((*it).second(log, scene, chunk, node))
            return;
        auto* pParamBlock = getLinkChunk(scene, *pChunk, 0);
        if (pParamBlock) {
            checkClass(log, *pParamBlock, {}, 0);
        }
    }
    auto& classData = chunk.classData;
    node.text = node.text + format("Unknown (%08X-%08X-%08X-%08X) %s\n", classData.dllIndex, classData.classID.first, classData.classID.second, classData.superClassID, chunk.name.c_str());
    checkClass(log, *pChunk, {}, 0);
}

void miMAXNode::EulerToQuaternion(float const euler[3], Quat& quaternion)
{
    float cx = std::cosf(euler[0] * 0.5f);
    float cy = std::cosf(euler[1] * 0.5f);
    float cz = std::cosf(euler[2] * 0.5f);
    float sx = std::sinf(euler[0] * 0.5f);
    float sy = std::sinf(euler[1] * 0.5f);
    float sz = std::sinf(euler[2] * 0.5f);
    quaternion[0] = (sx * cy * cz - cx * sy * sz);
    quaternion[1] = (cx * sy * cz + sx * cy * sz);
    quaternion[2] = (cx * cy * sz - sx * sy * cz);
    quaternion[3] = (cx * cy * cz + sx * sy * sz);
}

float miMAXNode::Bezier(BezierFloat const& left, BezierFloat const& right, float scale)
{
    float A = std::lerp(left[0], left[1], scale);
    float B = std::lerp(left[1], left[2], scale);
    float C = std::lerp(left[2], right[0], scale);
    float X = std::lerp(A, B, scale);
    float Y = std::lerp(B, C, scale);
    return std::lerp(X, Y, scale);
}

bool miMAXNode::RegisterPrimitive(ClassID classID, bool(*primitive)(Print, Chunk const&, Chunk const&, miMAXNode&))
{
    primitiveMap.emplace_back(classID, primitive);
    std::stable_sort(primitiveMap.begin(), primitiveMap.end());
    return true;
}

miMAXNode* miMAXNode::OpenFile(char const* name, Print log)
{
    FILE* file = fopen(name, "rb");
    if (file == nullptr) {
        log("%s is not found\n", name);
        return nullptr;
    }

    miMAXNode* root = nullptr;

    TRY

    root = new miMAXNode;
    if (root == nullptr) {
        log("%s\n", "Out of memory");
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
        cfbReader.EnumFiles(cfbReader.GetRootEntry(), -1, [&](CFB::COMPOUND_FILE_ENTRY const* entry, std::u16string const& dir, int level) {
            std::string name = UTF16ToUTF8(entry->name);
            std::vector<char>* data = nullptr;
            if (name == "ClassData")            data = &dataClassData;
            else if (name == "ClassDirectory")  data = &dataClassDirectory;
            else if (name == "ClassDirectory2") data = &dataClassDirectory;
            else if (name == "ClassDirectory3") data = &dataClassDirectory;
            else if (name == "Config")          data = &dataConfig;
            else if (name == "DllDirectory")    data = &dataDllDirectory;
            else if (name == "Scene")           data = &dataScene;
            else if (name == "VideoPostQueue")  data = &dataVideoPostQueue;
            if (data) {
                if (cfbReader.GetFileInfo()->majorVersion <= 3) {
                    (uint64_t&)entry->size = uint32_t(entry->size);
                }
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
        log("%s\n", "Scene is empty");
        THROW;
    }
    auto& scene = root->scene->front();
    switch (scene.type) {
    case 0x2001:    log("%s\n", "Kinetix 3D Studio MAX R1");    break;  // [x] 3D Studio MAX R1
    case 0x2003:    log("%s\n", "Kinetix 3D Studio MAX R2");    break;  // [x] 3D Studio MAX R2
                                                                        // [x] 3D Studio MAX R3
    case 0x2004:    log("%s\n", "Kinetix 3D Studio MAX R3");    break;  // [x] 3dsmax 4
    case 0x2008:    log("%s\n", "Discreet 3dsmax 5");           break;  // [x] 3dsmax 5
    case 0x2009:    log("%s\n", "Discreet 3dsmax 6");           break;  // [x] 3dsmax 6
    case 0x200A:    log("%s\n", "Discreet 3dsmax 7");           break;  // [x] 3dsmax 7
    case 0x200B:    log("%s\n", "Autodesk 3ds Max 8");          break;  // [x] 3ds Max 8
    case 0x200E:    log("%s\n", "Autodesk 3ds Max 9");          break;  // [x] 3ds Max 9
    case 0x200F:    log("%s\n", "Autodesk 3ds Max 2008");       break;  // [x] 3ds Max 2008
    case 0x2011:    log("%s\n", "Autodesk 3ds Max 2009");       break;  // [x] 3ds Max 2009
    case 0x2012:    log("%s\n", "Autodesk 3ds Max 2010");       break;  // [x] 3ds Max 2010
    case 0x2013:    log("%s\n", "Autodesk 3ds Max 2011");       break;  // [ ] 3ds Max 2011
    case 0x2014:    log("%s\n", "Autodesk 3ds Max 2012");       break;  // [ ] 3ds Max 2012
    case 0x2015:    log("%s\n", "Autodesk 3ds Max 2013");       break;  // [ ] 3ds Max 2013
    case 0x2016:    log("%s\n", "Autodesk 3ds Max 2014");       break;  // [ ] 3ds Max 2014
    case 0x2020:    log("%s\n", "Autodesk 3ds Max 2015");       break;  // [x] 3ds Max 2015
    case 0x2021:    log("%s\n", "Autodesk 3ds Max 2016");       break;  // [ ] 3ds Max 2016
    case 0x2022:    log("%s\n", "Autodesk 3ds Max 2017");       break;  // [ ] 3ds Max 2017
    case 0x2023:    log("%s\n", "Autodesk 3ds Max 2018");       break;  // [ ] 3ds Max 2018
    default:
        if (scene.type >= 0x2000) {
            log("%s (%X)\n", "Autodesk 3ds Max 20??", scene.type);
            break;
        }
        log("Scene type %04X is not supported\n", scene.type);
        THROW;
    }

    // First Pass
    for (uint32_t i = 0; i < scene.size(); ++i) {
        auto& chunk = scene[i];
        auto [className, classData] = getClass(*root->classDirectory, chunk.type);
        if (className.empty()) {
            if (chunk.type != 0x2032) {
                log("Class %04X is not found! (Chunk:%X)\n", chunk.type, i);
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
                log("Parent %d is not found! (Chunk:%d)\n", index, i);
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
            default: checkClass(log, *linkChunk, {}, 0);                    break;
            }
        }

        // Text
        std::vector<uint16_t> propertyText = getProperty<uint16_t>(chunk, 0x0120);
        if (propertyText.empty() == false) {
            node.text = node.text + UTF16ToUTF8(propertyText.data(), propertyText.size());
        }

        // Attach
        parent->emplace_back(std::move(node));
        nodes[i] = &parent->back();
    }

    CATCH (std::exception const& e) {
#if _CPPUNWIND || __cpp_exceptions
        log("Exception : %s\n", e.what());
#endif
        if (file) {
            fclose(file);
        }
        delete root;
        return nullptr;
    }

    return root;
}
