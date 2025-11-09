#include "miMAX.h"
#include "chunk.h"
#include "format.h"

static char const* getBirthMethod(int birthMethod)
{
    switch (birthMethod) {
    default:
    case 0: return "Use Rate";
    case 1: return "Use Total";
    }
    return nullptr;
}

static char const* getParticleClass(int particleClass)
{
    switch (particleClass) {
    default:
    case 0: return "Standard Particles";
    case 1: return "MetaParticles";
    case 2: return "Instanced Geometry";
    }
    return nullptr;
}

static char const* getParticleType(int particleType)
{
    switch (particleType) {
    default:
    case 0: return "Triangle";
    case 1: return "Cube";
    case 2: return "Special";
    case 3: return "Facing";
    case 4: return "Constant";
    case 5: return "Tetra";
    case 6: return "SixPoint";
    case 7: return "Sphere";
    }
    return nullptr;
}

static char const* getAnimationOffset(int animationOffset)
{
    switch (animationOffset) {
    default:
    case 0: return "None";
    case 1: return "Birth";
    case 2: return "Random";
    }
    return nullptr;
}

static char const* getViewportDisplay(int viewportDisplay)
{
    switch (viewportDisplay) {
    default:
    case 0: return "Dots";
    case 1: return "Ticks";
    case 2: return "Mesh";
    case 3: return "BBox";
    }
    return nullptr;
}

static char const* getMapping(int mapping)
{
    switch (mapping) {
    default:
    case 0: return "Time";
    case 1: return "Distance";
    }
    return nullptr;
}

static char const* getCustomMaterial(int customMaterial)
{
    switch (customMaterial) {
    default:
    case 0: return "Emitter";
    case 1: return "Instanced Geometry";
    }
    return nullptr;
}

static char const* getSpinDirection(int spinDirection)
{
    switch (spinDirection) {
    default:
    case 0: return "Random";
    case 1: return "Direction of Travel/Mblur";
    case 2: return "User Defined";
    }
    return nullptr;
}

static char const* getSpawnType(int spawnType)
{
    switch (spawnType) {
    default:
    case 0: return "None";
    case 1: return "Die After Collision";
    case 2: return "Spawn on Collision";
    case 3: return "Spawn on Death";
    case 4: return "Spawn Trails";
    }
    return nullptr;
}

static char const* getSpeedChaosSign(int speedChaos)
{
    switch (speedChaos) {
    default:
    case 0: return "Slow";
    case 1: return "Fast";
    case 2: return "Both";
    }
    return nullptr;
}

static char const* getScaleChaosSign(int scaleChaos)
{
    switch (scaleChaos) {
    default:
    case 0: return "Down";
    case 1: return "Up";
    case 2: return "Both";
    }
    return nullptr;
}

static bool particle(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    auto* pParamBlock = getLinkChunk(scene, chunk, 0);
    if (pParamBlock == nullptr)
        return false;
    auto paramBlock = getParamBlock(*pParamBlock);
    if (paramBlock.size() <= 68)
        return false;

    float offAxis = std::get<float>(paramBlock[0]);
    float axisSpread = std::get<float>(paramBlock[1]);
    float offPlane = std::get<float>(paramBlock[2]);
    float planeSpread = std::get<float>(paramBlock[3]);
    float speed = std::get<float>(paramBlock[4]);
    float speedVar = std::get<float>(paramBlock[5]);
    int birthMethod = std::get<int>(paramBlock[6]);
    int birthRate = std::get<int>(paramBlock[7]);
    int totalNumber = std::get<int>(paramBlock[8]);
    int emitStart = std::get<int>(paramBlock[9]);
    int emitStop = std::get<int>(paramBlock[10]);
    int displayUntil = std::get<int>(paramBlock[11]);
    int life = std::get<int>(paramBlock[12]);
    int lifeVar = std::get<int>(paramBlock[13]);
    int subframeMove = std::get<int>(paramBlock[14]);
    int subframeTime = std::get<int>(paramBlock[15]);
    float size = std::get<float>(paramBlock[16]);
    float sizeVar = std::get<float>(paramBlock[17]);
    int growTime = std::get<int>(paramBlock[18]);
    int fadeTime = std::get<int>(paramBlock[19]);
    int rndSeed = std::get<int>(paramBlock[20]);
    float emitterWidth = std::get<float>(paramBlock[21]);
    int emitterHidden = std::get<int>(paramBlock[22]);
    int particleClass = std::get<int>(paramBlock[23]);
    int particleType = std::get<int>(paramBlock[24]);
    float metaTension = std::get<float>(paramBlock[25]);
    float metaTensionVar = std::get<float>(paramBlock[26]);
    float metaCourse = std::get<float>(paramBlock[27]);
    int autoCoarseness = std::get<int>(paramBlock[28]);
    int useSubtree = std::get<int>(paramBlock[29]);
    int animationOffset = std::get<int>(paramBlock[30]);
    int offsetAmount = std::get<int>(paramBlock[31]);
    int viewportShows = std::get<int>(paramBlock[32]);
    float displayPortion = std::get<float>(paramBlock[33]);
    int mappingType = std::get<int>(paramBlock[34]);
    int mappingTime = std::get<int>(paramBlock[35]);
    float mappingDist = std::get<float>(paramBlock[36]);
    int spinTime = std::get<int>(paramBlock[37]);
    float spinTimeVar = std::get<float>(paramBlock[38]);
    float spinPhase = std::get<float>(paramBlock[39]);
    float spinPhaseVar = std::get<float>(paramBlock[40]);
    int spinAxisChoice = std::get<int>(paramBlock[41]);
    float spinAxisX = std::get<float>(paramBlock[42]);
    float spinAxisY = std::get<float>(paramBlock[43]);
    float spinAxisZ = std::get<float>(paramBlock[44]);
    float spinAxisVar = std::get<float>(paramBlock[45]);
    float emitInfluence = std::get<float>(paramBlock[46]);
    float emitMultiplier = std::get<float>(paramBlock[47]);
    float emitMultVar = std::get<float>(paramBlock[48]);
    int spawnType = std::get<int>(paramBlock[49]);
    int numberOfGens = std::get<int>(paramBlock[50]);
    int numberOfSpawns = std::get<int>(paramBlock[51]);
    float directionChaos = std::get<float>(paramBlock[52]);
    float speedChaos = std::get<float>(paramBlock[53]);
    int speedChaosSign = std::get<int>(paramBlock[54]);
    int inheritOldParticleVelocity = std::get<int>(paramBlock[55]);
    float scaleChaos = std::get<float>(paramBlock[56]);
    int scaleChaosSign = std::get<int>(paramBlock[57]);
    int lifespanEntryField = std::get<int>(paramBlock[58]);
    int constantSpawnSpeed = std::get<int>(paramBlock[59]);
    int constantSpawnScale = std::get<int>(paramBlock[60]);
    float bubbleAmp = std::get<float>(paramBlock[61]);
    float bubbleAmpVar = std::get<float>(paramBlock[62]);
    int bubblePeriod = std::get<int>(paramBlock[63]);
    float bubblePeriodVar = std::get<float>(paramBlock[64]);
    float bubblePhase = std::get<float>(paramBlock[65]);
    float bubblePhaseVar = std::get<float>(paramBlock[66]);
    int stretch = std::get<int>(paramBlock[67]);
    int customMtl = std::get<int>(paramBlock[68]);
    float viewportCourseness = 0.0f;
    int subframeRot = 0;
    int spawnPercent = 1;
    float spawnMultVar = 0.1f;
    int surfaceTracking = 0;
    int diesAfterX = 10;
    float diesAfterXVar = 0.01f;
    int ipcEnable = false;
    int ipcSteps = 1;
    float ipcBounce = 1.0f;
    float ipcBounceVar = 1.0f;

    if (paramBlock.size() > 69) {
        viewportCourseness = std::get<float>(paramBlock[69]);
    }

    if (paramBlock.size() > 70) {
        subframeRot = std::get<int>(paramBlock[70]);
    }

    if (paramBlock.size() > 72) {
        spawnPercent = std::get<int>(paramBlock[71]);
        spawnMultVar = std::get<float>(paramBlock[72]);
    }

    if (paramBlock.size() > 73) {
        surfaceTracking = std::get<int>(paramBlock[73]);
    }

    if (paramBlock.size() > 75) {
        diesAfterX = std::get<int>(paramBlock[74]);
        diesAfterXVar = std::get<float>(paramBlock[75]);
    }

    if (paramBlock.size() > 79) {
        ipcEnable = std::get<int>(paramBlock[76]);
        ipcSteps = std::get<int>(paramBlock[77]);
        ipcBounce = std::get<float>(paramBlock[78]);
        ipcBounceVar = std::get<float>(paramBlock[79]);
    }

    node.text = node.text + format("%-40s : %s", "Particle", "Super Spray") + '\n';
    node.text = node.text + format("%-40s : %g", "Off Axis", offAxis) + '\n';
    node.text = node.text + format("%-40s : %g", "Axis Spread", axisSpread) + '\n';
    node.text = node.text + format("%-40s : %g", "Off Plane", offPlane) + '\n';
    node.text = node.text + format("%-40s : %g", "Plane Spread", planeSpread) + '\n';
    node.text = node.text + format("%-40s : %g", "Speed", speed) + '\n';
    node.text = node.text + format("%-40s : %g", "Speed Variation", speedVar) + '\n';
    node.text = node.text + format("%-40s : %s", "Birth Method", getBirthMethod(birthMethod)) + '\n';
    node.text = node.text + format("%-40s : %d", "Birth Rate", birthRate) + '\n';
    node.text = node.text + format("%-40s : %d", "Total Number", totalNumber) + '\n';
    node.text = node.text + format("%-40s : %d", "Emitter Start", emitStart) + '\n';
    node.text = node.text + format("%-40s : %d", "Emitter Stop", emitStop) + '\n';
    node.text = node.text + format("%-40s : %d", "Display Until", displayUntil) + '\n';
    node.text = node.text + format("%-40s : %d", "Life", life) + '\n';
    node.text = node.text + format("%-40s : %d", "Life Variation", lifeVar) + '\n';
    node.text = node.text + format("%-40s : %s", "Subframe Sampling Emitter Translation", getBoolean(subframeMove)) + '\n';
    node.text = node.text + format("%-40s : %s", "Subframe Sampling Createion Time", getBoolean(subframeTime)) + '\n';
    node.text = node.text + format("%-40s : %g", "Size", size) + '\n';
    node.text = node.text + format("%-40s : %g", "Size Variation", sizeVar) + '\n';
    node.text = node.text + format("%-40s : %d", "Growth Time", growTime) + '\n';
    node.text = node.text + format("%-40s : %d", "Fade Time", fadeTime) + '\n';
    node.text = node.text + format("%-40s : %d", "Random Seed", rndSeed) + '\n';
    node.text = node.text + format("%-40s : %g", "Emitter Width", emitterWidth) + '\n';
    node.text = node.text + format("%-40s : %s", "Emitter Hidden", getBoolean(emitterHidden)) + '\n';
    node.text = node.text + format("%-40s : %s", "Particle Class", getParticleClass(particleClass)) + '\n';
    node.text = node.text + format("%-40s : %s", "Particle Type", getParticleType(particleType)) + '\n';
    node.text = node.text + format("%-40s : %g", "Metaparticle Tension", metaTension) + '\n';
    node.text = node.text + format("%-40s : %g", "Metaparticle Tension Variation", metaTensionVar) + '\n';
    node.text = node.text + format("%-40s : %g", "Metaparticle Coarseness", metaCourse) + '\n';
    node.text = node.text + format("%-40s : %s", "Automatic Coarseness", getBoolean(autoCoarseness)) + '\n';
    node.text = node.text + format("%-40s : %s", "Use Subtree", getBoolean(useSubtree)) + '\n';
    node.text = node.text + format("%-40s : %s", "Animation Offset Method", getAnimationOffset(animationOffset)) + '\n';
    node.text = node.text + format("%-40s : %d", "Animation Offset Amount", offsetAmount) + '\n';
    node.text = node.text + format("%-40s : %s", "Viewport Display", getViewportDisplay(viewportShows)) + '\n';
    node.text = node.text + format("%-40s : %g", "Percentage Displayed", displayPortion) + '\n';
    node.text = node.text + format("%-40s : %s", "Mapping Type", getMapping(mappingType)) + '\n';
    node.text = node.text + format("%-40s : %d", "Mapping Time Base", mappingTime) + '\n';
    node.text = node.text + format("%-40s : %g", "Mapping Distance Base", mappingDist) + '\n';
    node.text = node.text + format("%-40s : %d", "Spin Time", spinTime) + '\n';
    node.text = node.text + format("%-40s : %g", "Spin Time Variation", spinTimeVar) + '\n';
    node.text = node.text + format("%-40s : %g", "Spin Phase", spinPhase) + '\n';
    node.text = node.text + format("%-40s : %g", "Spin Phase Variation", spinPhaseVar) + '\n';
    node.text = node.text + format("%-40s : %s", "Spin Axis Type", getSpinDirection(spinAxisChoice)) + '\n';
    node.text = node.text + format("%-40s : %g", "X Spin Vector", spinAxisX) + '\n';
    node.text = node.text + format("%-40s : %g", "Y Spin Vector", spinAxisY) + '\n';
    node.text = node.text + format("%-40s : %g", "Z Spin Vector", spinAxisZ) + '\n';
    node.text = node.text + format("%-40s : %g", "Spin Axis Variation", spinAxisVar) + '\n';
    node.text = node.text + format("%-40s : %g", "Object Motion Inheritance", emitInfluence) + '\n';
    node.text = node.text + format("%-40s : %g", "Object Motion Multiplier", emitMultiplier) + '\n';
    node.text = node.text + format("%-40s : %g", "Object Motion Multiplier Variation", emitMultVar) + '\n';
    node.text = node.text + format("%-40s : %s", "Spawn Type", getSpawnType(spawnType)) + '\n';
    node.text = node.text + format("%-40s : %d", "Spawn Generations", numberOfGens) + '\n';
    node.text = node.text + format("%-40s : %d", "Spawn Count", numberOfSpawns) + '\n';
    node.text = node.text + format("%-40s : %g", "Spawn Direction Chaos", directionChaos) + '\n';
    node.text = node.text + format("%-40s : %g", "Spawn Speed Chaos", speedChaos) + '\n';
    node.text = node.text + format("%-40s : %s", "Spawn Speed Chaos Sign", getSpeedChaosSign(speedChaosSign)) + '\n';
    node.text = node.text + format("%-40s : %s", "Inherit Parent Velocity", getBoolean(inheritOldParticleVelocity)) + '\n';
    node.text = node.text + format("%-40s : %g", "Spawn Scale Chaos", scaleChaos) + '\n';
    node.text = node.text + format("%-40s : %s", "Spawn Scale Chaos Sign", getScaleChaosSign(scaleChaosSign)) + '\n';
    node.text = node.text + format("%-40s : %d", "Spawn Lifespan", lifespanEntryField) + '\n';
    node.text = node.text + format("%-40s : %s", "Spawn Speed Use Fixed Value", getBoolean(constantSpawnSpeed)) + '\n';
    node.text = node.text + format("%-40s : %s", "Spawn Scale Use Fixed Value", getBoolean(constantSpawnScale)) + '\n';
    node.text = node.text + format("%-40s : %g", "Bubble Amplitude", bubbleAmp) + '\n';
    node.text = node.text + format("%-40s : %g", "Bubble Amplitude Variation", bubbleAmpVar) + '\n';
    node.text = node.text + format("%-40s : %d", "Bubble Period", bubblePeriod) + '\n';
    node.text = node.text + format("%-40s : %g", "Bubble Period Variation", bubblePeriodVar) + '\n';
    node.text = node.text + format("%-40s : %g", "Bubble Phase", bubblePhase) + '\n';
    node.text = node.text + format("%-40s : %g", "Bubble Phase Variation", bubblePhaseVar) + '\n';
    node.text = node.text + format("%-40s : %d", "Blur Stretch", stretch) + '\n';
    node.text = node.text + format("%-40s : %s", "Custom Material", getCustomMaterial(customMtl)) + '\n';
    node.text = node.text + format("%-40s : %g", "Metaparticle Coarseness Viewport", viewportCourseness) + '\n';
    node.text = node.text + format("%-40s : %s", "Subframe Sampling Emitter Rotation", getBoolean(subframeRot)) + '\n';
    node.text = node.text + format("%-40s : %d", "Spawn Affects", spawnPercent) + '\n';
    node.text = node.text + format("%-40s : %g", "Spawn Multiplier Variation", spawnMultVar) + '\n';
    node.text = node.text + format("%-40s : %s", "One Connected Blob", getBoolean(surfaceTracking)) + '\n';
    node.text = node.text + format("%-40s : %d", "Die X frames after collision", diesAfterX) + '\n';
    node.text = node.text + format("%-40s : %g", "Die X frames after collision variation", diesAfterXVar) + '\n';
    node.text = node.text + format("%-40s : %s", "Interparticle Collisions On", getBoolean(ipcEnable)) + '\n';
    node.text = node.text + format("%-40s : %d", "Interparticle Collision Steps", ipcSteps) + '\n';
    node.text = node.text + format("%-40s : %g", "Interparticle Collision Bounce", ipcBounce) + '\n';
    node.text = node.text + format("%-40s : %g", "Interparticle Collision Bounce Variation", ipcBounceVar) + '\n';
    return true;
}

static bool register_object = miMAXNode::RegisterObject(SUPRSPRAY_CLASS_ID, particle);
