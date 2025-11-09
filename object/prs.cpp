#include "miMAX.h"
#include "chunk.h"
#include "format.h"

#define FLOAT_TYPE          0x2501
#define POINT3_TYPE         0x2503
#define QUAT_TYPE           0x2504
#define SCALEVALUE_TYPE     0x2505

static bool control(Print log, Chunk const& scene, Chunk const& chunk, Chunk const& child, miMAXNode& node)
{
    // FFFFFFFF-00002005-00000000-00009008 Position/Rotation/Scale  PRS_CONTROL_CLASS_ID + MATRIX3_SUPERCLASS_ID
    if (checkClass(log, chunk, PRS_CONTROL_CLASS_ID, MATRIX3_SUPERCLASS_ID) == false)
        return false;

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
    return true;
}

static bool register_object = miMAXNode::RegisterObject(PRS_CONTROL_CLASS_ID, control);
