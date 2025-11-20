#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static bool deflector(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 4)
        return false;

    float bounce = std::get<float>(paramBlock[0]);
    float bounceVar = std::get<float>(paramBlock[1]);
    float chaos = std::get<float>(paramBlock[2]);
    float radius = std::get<float>(paramBlock[3]);
    float velocityInheritance = std::get<float>(paramBlock[4]);
    float friction = 0.0f;

    if (paramBlock.size() > 5) {
        friction = std::get<float>(paramBlock[5]);
    }

    node.text = node.text + format("%-24s : %s", "Deflector", "Sphere") + '\n';
    node.text = node.text + format("%-24s : %g", "Bounce", bounce) + '\n';
    node.text = node.text + format("%-24s : %g", "Bounce Variation", bounceVar) + '\n';
    node.text = node.text + format("%-24s : %g", "Chaos", chaos) + '\n';
    node.text = node.text + format("%-24s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-24s : %g", "Velocity Inheritance", velocityInheritance) + '\n';
    node.text = node.text + format("%-24s : %g", "Friction", friction) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(SPHEREDEF_CLASS_ID, deflector);
