#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#if HAVE_3DSMAX_SDK
#include "object/sdk/mods/deflect.h"
#endif

static bool deflector(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 2)
        return false;

    float bounce = std::get<float>(paramBlock[0]);
    float width = std::get<float>(paramBlock[1]);
    float height = std::get<float>(paramBlock[2]);

    node.text = node.text + format("%-24s : %s", "Deflector", "Planar") + '\n';
    node.text = node.text + format("%-24s : %g", "Bounce", bounce) + '\n';
    node.text = node.text + format("%-24s : %g", "Width", width) + '\n';
    node.text = node.text + format("%-24s : %g", "Height", height) + '\n';
#if HAVE_3DSMAX_SDK
    void* param[] = {
        &bounce,
        &width,
        &height,
    };

    Mesh mesh{node.vertex, node.vertexArray, node.texture, node.textureArray};
    IParamBlock block{param};

    BuildMesh(mesh, &block);
#endif
    return true;
}

static bool register_object = miMAXNode::RegisterObject(DEFLECTOBJECT_CLASS_ID, deflector);
