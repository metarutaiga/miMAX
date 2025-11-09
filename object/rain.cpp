#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static char const* getDisplay(int display)
{
    switch (display) {
    default:
    case 0: return "Falkes";
    case 1: return "Dots";
    case 2: return "Ticks";
    }
    return nullptr;
}

static char const* getRender(int render)
{
    switch (render) {
    default:
    case 0: return "Six Point";
    case 1: return "Triangle";
    case 2: return "Facing";
    }
    return nullptr;
}

static bool particle(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 9)
        return false;

    int vptCount = std::get<int>(paramBlock[0]);
    int rndCount = 1;
    float size = std::get<float>(paramBlock[1]);
    float speed = std::get<float>(paramBlock[2]);
    float variation = std::get<float>(paramBlock[3]);
    int viewportType = std::get<int>(paramBlock[4]);
    int start = std::get<int>(paramBlock[5]);
    int life = std::get<int>(paramBlock[6]);
    float width = std::get<float>(paramBlock[7]);
    float height = std::get<float>(paramBlock[8]);
    int hideEmitter = std::get<int>(paramBlock[9]);
    float birthRate = 0.1f;
    int constant = 0;
    int renderType = 0;
    float tumble = 0.005f;
    float tumbleScale = 0.01f;

    if (paramBlock.size() > 11) {
        birthRate = std::get<float>(paramBlock[10]);
        constant = std::get<int>(paramBlock[11]);
    }

    if (chunk.classData.classID == RAIN_CLASS_ID) {
        if (paramBlock.size() > 12) {
            renderType = std::get<int>(paramBlock[12]);
        }
        if (paramBlock.size() > 13) {
            rndCount = std::get<int>(paramBlock[1]);
            size = std::get<float>(paramBlock[2]);
            speed = std::get<float>(paramBlock[3]);
            variation = std::get<float>(paramBlock[4]);
            viewportType = std::get<int>(paramBlock[5]);
            start = std::get<int>(paramBlock[6]);
            life = std::get<int>(paramBlock[7]);
            width = std::get<float>(paramBlock[8]);
            height = std::get<float>(paramBlock[9]);
            hideEmitter = std::get<int>(paramBlock[10]);
            birthRate = std::get<float>(paramBlock[11]);
            constant = std::get<int>(paramBlock[12]);
            renderType = std::get<int>(paramBlock[13]);
        }
    }

    if (chunk.classData.classID == SNOW_CLASS_ID) {
        if (paramBlock.size() > 14) {
            tumble = std::get<float>(paramBlock[4]);
            tumbleScale = std::get<float>(paramBlock[5]);
            viewportType = std::get<int>(paramBlock[6]);
            start = std::get<int>(paramBlock[7]);
            life = std::get<int>(paramBlock[8]);
            width = std::get<float>(paramBlock[9]);
            height = std::get<float>(paramBlock[10]);
            hideEmitter = std::get<int>(paramBlock[11]);
            birthRate = std::get<float>(paramBlock[12]);
            constant = std::get<int>(paramBlock[13]);
            renderType = std::get<int>(paramBlock[14]);
        }
        if (paramBlock.size() > 15) {
            rndCount = std::get<int>(paramBlock[1]);
            size = std::get<float>(paramBlock[2]);
            speed = std::get<float>(paramBlock[3]);
            variation = std::get<float>(paramBlock[4]);
            viewportType = std::get<int>(paramBlock[5]);
            start = std::get<int>(paramBlock[6]);
            life = std::get<int>(paramBlock[7]);
            width = std::get<float>(paramBlock[8]);
            height = std::get<float>(paramBlock[9]);
            hideEmitter = std::get<int>(paramBlock[10]);
            birthRate = std::get<float>(paramBlock[11]);
            constant = std::get<int>(paramBlock[12]);
            renderType = std::get<int>(paramBlock[13]);
            tumble = std::get<float>(paramBlock[14]);
            tumbleScale = std::get<float>(paramBlock[15]);
        }
    }

    if (chunk.classData.classID == RAIN_CLASS_ID) {
        node.text = node.text + format("%-24s : %s", "Particle", "Spray") + '\n';
    }
    if (chunk.classData.classID == SNOW_CLASS_ID) {
        node.text = node.text + format("%-24s : %s", "Particle", "Snow") + '\n';
    }
    node.text = node.text + format("%-24s : %d", "Viewport Particles", vptCount) + '\n';
    node.text = node.text + format("%-24s : %d", "Render Particles", rndCount) + '\n';
    node.text = node.text + format("%-24s : %g", "Particle Size", size) + '\n';
    node.text = node.text + format("%-24s : %g", "Particle Speed", speed) + '\n';
    node.text = node.text + format("%-24s : %g", "Particle Variation", variation) + '\n';
    node.text = node.text + format("%-24s : %s", "Display Type", getDisplay(viewportType)) + '\n';
    node.text = node.text + format("%-24s : %d", "Start Time", start) + '\n';
    node.text = node.text + format("%-24s : %d", "Life Time", life) + '\n';
    node.text = node.text + format("%-24s : %g", "Emitter Width", width) + '\n';
    node.text = node.text + format("%-24s : %g", "Emitter Height", height) + '\n';
    node.text = node.text + format("%-24s : %s", "Hide Emitter", getBoolean(hideEmitter)) + '\n';
    node.text = node.text + format("%-24s : %g", "Birth Rate", birthRate) + '\n';
    node.text = node.text + format("%-24s : %s", "Constant Birth Rate", getBoolean(constant)) + '\n';
    node.text = node.text + format("%-24s : %s", "Render Type", getRender(renderType)) + '\n';
    if (chunk.classData.classID == SNOW_CLASS_ID) {
        node.text = node.text + format("%-24s : %g", "Tumble", tumble) + '\n';
        node.text = node.text + format("%-24s : %g", "Tumble Scale", tumbleScale) + '\n';
    }
    return true;
}

static bool register_object = miMAXNode::RegisterObject(RAIN_CLASS_ID, particle) | miMAXNode::RegisterObject(SNOW_CLASS_ID, particle);
