/*
    2012 Kaetemi https://blog.kaetemi.be
    2025 TAiGA   https://github.com/metarutaiga/miMAX
*/
#include <stdio.h>
#include <cmath>
#include <functional>
#include <map>
#include <tuple>

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

static std::vector<std::pair<ClassID, bool(*)(Print, Chunk const&, Chunk const&, Chunk const&, miMAXNode&)>> objectMap;

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

static void getObject(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    if (chunk.type == 0x2032 || chunk.type == 0x2033) {
        auto link = getLink<std::greater<uint32_t>>(chunk);
        for (auto [linkIndex, chunkIndex] : link) {
            if (scene.size() <= chunkIndex)
                continue;
            auto& linkedChunk = scene[chunkIndex];
            if (&linkedChunk == &chunk)
                continue;
            for (auto& child : linkedChunk) {
                getObject(log, scene, linkedChunk, child, node);
            }
        }
        return;
    }

    auto classID = chunk.classData.classID;
    auto value = decltype(objectMap)::value_type(classID, nullptr);
    auto it = std::lower_bound(objectMap.begin(), objectMap.end(), value);
    if (it != objectMap.end() && (*it).first == classID) {
        if ((*it).second(log, scene, chunk, child, node)) {
            return;
        }
    }

    auto& classData = chunk.classData;
    node.text = node.text + format("Unknown (%08X-%08X-%08X-%08X) %s\n", classData.dllIndex, classData.classID.first, classData.classID.second, classData.superClassID, chunk.name.c_str());
    checkClass(log, chunk, {}, 0);
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
    float A = std::lerp(left[0], right[1], scale);
    float B = std::lerp(left[1], right[2], scale);
    float C = std::lerp(left[2], right[0], scale);
    float X = std::lerp(A, B, scale);
    float Y = std::lerp(B, C, scale);
    return std::lerp(X, Y, scale);
}

bool miMAXNode::RegisterObject(ClassID classID, bool(*primitive)(Print, Chunk const&, Chunk const&, Chunk const&, miMAXNode&))
{
    objectMap.emplace_back(classID, primitive);
    std::stable_sort(objectMap.begin(), objectMap.end());
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
            if (chunk.type != 0x2032 && chunk.type != 0x2033) {
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
        for (uint32_t i = 0; i < 8; ++i) {
            Chunk const* linkChunk = getLinkChunk(scene, chunk, i);
            if (linkChunk == nullptr)
                continue;
            getObject(log, scene, *linkChunk, Chunk(), node);
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
