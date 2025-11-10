/*
    2012 Kaetemi https://blog.kaetemi.be
    2025 TAiGA   https://github.com/metarutaiga/miMAX
*/
#pragma once

#include <array>
#include <list>
#include <string>
#include <vector>

struct miMAXNode : public std::list<miMAXNode>
{
public:
    typedef std::array<float, 3> Point3;
    typedef std::array<float, 4> Quat;
    typedef std::array<float, 5> BezierFloat;

public:
    miMAXNode* parent = nullptr;

    std::string name;
    std::string text;

    Point3 position = { 0, 0, 0 };
    Quat rotation = { 0, 0, 0, 1 };
    Point3 scale = { 1, 1, 1 };

    std::vector<std::pair<uint32_t, BezierFloat>> keyPosition[3];
    std::vector<std::pair<uint32_t, BezierFloat>> keyRotation[3];
    std::vector<std::pair<uint32_t, BezierFloat>> keyScale;

    std::vector<Point3> vertex;
    std::vector<Point3> texture;

    std::vector<Point3> normal;
    std::vector<Point3> vertexColor;
    std::vector<Point3> vertexIllum;
    std::vector<Point3> vertexAlpha;

    std::vector<std::vector<uint32_t>> vertexArray;
    std::vector<std::vector<uint32_t>> textureArray;

    int padding = 0;

public:
    typedef std::pair<uint32_t, uint32_t> ClassID;
    typedef uint32_t SuperClassID;

    struct ClassData
    {
        uint32_t dllIndex;
        ClassID classID;
        SuperClassID superClassID;
    };

public:
    struct Chunk : public std::vector<Chunk>
    {
        Chunk* parent = nullptr;

        std::vector<char> property;

        uint16_t type = 0;
        uint16_t padding = 0;
        ClassData classData = {};
        std::string classDllFile;
        std::string classDllName;
        std::string name;
    };
    Chunk* classData = nullptr;
    Chunk* classDirectory = nullptr;
    Chunk* config = nullptr;
    Chunk* dllDirectory = nullptr;
    Chunk* scene = nullptr;
    Chunk* videoPostQueue = nullptr;

    ~miMAXNode()
    {
        delete classData;
        delete classDirectory;
        delete config;
        delete dllDirectory;
        delete scene;
        delete videoPostQueue;
    }

public:
    typedef int(*Print)(char const*, ...);

    static float Bezier(BezierFloat const& left, BezierFloat const& right, float scale);
    static void EulerToQuaternion(float const euler[3], Quat& quaternion);

    static bool RegisterObject(ClassID, bool(*)(Print, Chunk const&, Chunk const&, Chunk const&, miMAXNode&));

    static miMAXNode* OpenFile(char const* name, Print log);
};
