#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#if HAVE_3DSMAX_SDK
#include "object/sdk/suprprts/udeflect.h"
#endif

static bool deflector(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 5)
        return false;

    float bounce = std::get<float>(paramBlock[0]);
    float bounceVar = std::get<float>(paramBlock[1]);
    float chaos = std::get<float>(paramBlock[2]);
    float radius = std::get<float>(paramBlock[3]);
    float friction = std::get<float>(paramBlock[4]);
    float velocity = std::get<float>(paramBlock[5]);

    node.text = node.text + format("%-24s : %s", "Deflector", "UDeflector") + '\n';
    node.text = node.text + format("%-24s : %g", "Bounce", bounce) + '\n';
    node.text = node.text + format("%-24s : %g", "Bounce Variation", bounceVar) + '\n';
    node.text = node.text + format("%-24s : %g", "Chaos", chaos) + '\n';
    node.text = node.text + format("%-24s : %g", "Deflector Friction", friction) + '\n';
    node.text = node.text + format("%-24s : %g", "Radius", radius) + '\n';
    node.text = node.text + format("%-24s : %g", "Velocity Inheritance", velocity) + '\n';
#if HAVE_3DSMAX_SDK
    void* param[] = {
        &bounce,
        &bounceVar,
        &chaos,
        &radius,
        &friction,
        &velocity,
    };

    Mesh mesh{node.vertex, node.vertexArray, node.texture, node.textureArray};
    IParamBlock block{param};

    BuildMesh(mesh, &block);
#endif
    return true;
}

static bool register_object = miMAXNode::RegisterObject(UNIDEF_CLASS_ID, deflector);
