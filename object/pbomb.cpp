#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static char const* getSymmetry(int symmetry)
{
    switch (symmetry) {
    default:
    case 0: return "Spherical";
    case 1: return "Cylindrical";
    case 2: return "Planar";
    }
    return nullptr;
}

static char const* getDecay(int decay)
{
    switch (decay) {
    default:
    case 0: return "Unlimited Range";
    case 1: return "Linear";
    case 2: return "Exponential";
    }
    return nullptr;
}

static bool particle(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 6)
        return false;

    int blastSymmetry = std::get<int>(paramBlock[0]);
    float directionChaos = std::get<float>(paramBlock[1]);
    int startTime = std::get<int>(paramBlock[2]);
    int lastTime = std::get<int>(paramBlock[3]);
    float deltaV = std::get<float>(paramBlock[4]);
    float decayRange = std::get<float>(paramBlock[5]);
    int decayType = std::get<int>(paramBlock[6]);
    float iconSize = std::get<float>(paramBlock[7]);
    int rangeIndicator = 0;

    if (paramBlock.size() > 8) {
        rangeIndicator = std::get<int>(paramBlock[8]);
    }

    node.text = node.text + format("%-24s : %s", "Particle", "PBomb") + '\n';
    node.text = node.text + format("%-24s : %s", "Blast Symmetry", getSymmetry(blastSymmetry)) + '\n';
    node.text = node.text + format("%-24s : %g", "Direction Chaos", directionChaos) + '\n';
    node.text = node.text + format("%-24s : %d", "Start Time", startTime) + '\n';
    node.text = node.text + format("%-24s : %d", "Last Time", lastTime) + '\n';
    node.text = node.text + format("%-24s : %g", "Delta V", deltaV) + '\n';
    node.text = node.text + format("%-24s : %g", "Decay Range", decayRange) + '\n';
    node.text = node.text + format("%-24s : %s", "Decay Type", getDecay(decayType)) + '\n';
    node.text = node.text + format("%-24s : %g", "Icon Size", iconSize) + '\n';
    node.text = node.text + format("%-24s : %s", "Range Indicator", getBoolean(rangeIndicator)) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(PBOMB_CLASS_ID, particle);
