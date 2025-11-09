#pragma once

typedef miMAXNode::ClassID ClassID;
typedef miMAXNode::SuperClassID SuperClassID;
typedef miMAXNode::ClassData ClassData;
typedef miMAXNode::Point3 Point3;
typedef miMAXNode::Quat Quat;
typedef miMAXNode::BezierFloat BezierFloat;
typedef miMAXNode::Chunk Chunk;
typedef miMAXNode::Print Print;

#define BASENODE_SUPERCLASS_ID          0x00000001
#define PARAMETER_BLOCK_SUPERCLASS_ID   0x00000008
#define PARAMETER_BLOCK2_SUPERCLASS_ID  0x00000082
#define GEOMOBJECT_SUPERCLASS_ID        0x00000010
#define OSM_SUPERCLASS_ID               0x00000810
#define FLOAT_SUPERCLASS_ID             0x00009003
#define MATRIX3_SUPERCLASS_ID           0x00009008
#define POSITION_SUPERCLASS_ID          0x0000900b
#define ROTATION_SUPERCLASS_ID          0x0000900c
#define SCALE_SUPERCLASS_ID             0x0000900d

#define LININTERP_POSITION_CLASS_ID     ClassID{0x00002002, 0x00000000}
#define LININTERP_ROTATION_CLASS_ID     ClassID{0x00002003, 0x00000000}
#define LININTERP_SCALE_CLASS_ID        ClassID{0x00002004, 0x00000000}
#define PRS_CONTROL_CLASS_ID            ClassID{0x00002005, 0x00000000}
#define HYBRIDINTERP_FLOAT_CLASS_ID     ClassID{0x00002007, 0x00000000}
#define HYBRIDINTERP_POSITION_CLASS_ID  ClassID{0x00002008, 0x00000000}
#define HYBRIDINTERP_SCALE_CLASS_ID     ClassID{0x00002010, 0x00000000}
#define HYBRIDINTERP_POINT4_CLASS_ID    ClassID{0x00002012, 0x00000000}
#define TCBINTERP_POSITION_CLASS_ID     ClassID{0x00442312, 0x00000000}
#define TCBINTERP_ROTATION_CLASS_ID     ClassID{0x00442313, 0x00000000}
#define TCBINTERP_SCALE_CLASS_ID        ClassID{0x00442315, 0x00000000}
#define IPOS_CONTROL_CLASS_ID           ClassID{0x118f7e02, 0xffee238a}

#define BOXOBJ_CLASS_ID                 ClassID{0x00000010, 0x00000000}
#define SPHERE_CLASS_ID                 ClassID{0x00000011, 0x00000000}
#define CYLINDER_CLASS_ID               ClassID{0x00000012, 0x00000000}
#define TORUS_CLASS_ID                  ClassID{0x00000020, 0x00000000}
#define CONE_CLASS_ID                   ClassID{0xa86c23dd, 0x00000000}
#define GSPHERE_CLASS_ID                ClassID{0x00000000, 0x00007f9e}
#define TUBE_CLASS_ID                   ClassID{0x00007b21, 0x00000000}
#define PYRAMID_CLASS_ID                ClassID{0x76bf318a, 0x4bf37b10}
#define GRID_CLASS_ID                   ClassID{0x081f1dfc, 0x77566f65}

#define EDITTRIOBJ_CLASS_ID             ClassID{0xe44f10b3, 0x00000000}
#define EPOLYOBJ_CLASS_ID               ClassID{0x1bf8338d, 0x192f6098}
#define PATCHOBJ_CLASS_ID               ClassID{0x00001030, 0x00000000}

#define EDIT_NORMALS_CLASS_ID           ClassID{0x4aa52ae3, 0x35ca1cde}
#define PAINTLAYERMOD_CLASS_ID          ClassID{0x7ebb4645, 0x7be2044b}

#define BLIZZARD_CLASS_ID               ClassID{0x5835054d, 0x564b40ed}
#define PARRAY_CLASS_ID                 ClassID{0x0e3c25b5, 0x109d1659}
#define PBOMB_CLASS_ID                  ClassID{0x4c200df3, 0x1a347a77}
#define PBOMBMOD_CLASS_ID               ClassID{0x0c0609ea, 0x01300b3d}
#define PCLOUD_CLASS_ID                 ClassID{0x1c0f3d2f, 0x30310af9}
#define PFOLLOW_CLASS_ID                ClassID{0x7ab83ab5, 0x5e1d34bd}
#define PFOLLOWMOD_CLASS_ID             ClassID{0x263e723d, 0x132724e5}
#define RAIN_CLASS_ID                   ClassID{0x9bd61aa0, 0x00000000}
#define SNOW_CLASS_ID                   ClassID{0x9bd61aa1, 0x00000000}
#define SPHEREDEF_CLASS_ID              ClassID{0x6cbd289d, 0x3fef6656}
#define SPHEREDEFMOD_CLASS_ID           ClassID{0x5cdf4181, 0x4c5b42f9}
#define SUPRSPRAY_CLASS_ID              ClassID{0x74f811e3, 0x21fb7b57}
#define UNIDEF_CLASS_ID                 ClassID{0x28497b68, 0x00000000}
#define UNIDEFMOD_CLASS_ID              ClassID{0x4d456b2d, 0x00000000}

#define PLANAR_COLLISION_CLASS_ID       ClassID{0x14585111, 0x444a7dcf}
#define SPHERICAL_COLLISION_CLASS_ID    ClassID{0x14585222, 0x555a7dcf}
#define MESH_COLLISION_CLASS_ID         ClassID{0x14585333, 0x666a7dcf}

#define FORCE_CLASS_ID                  ClassID{0x3d4d3ca5, 0x3b22358f}
#define FORCEMOD_CLASS_ID               ClassID{0x44975ae9, 0x65f53e3a}
#define MOTOR_CLASS_ID                  ClassID{0x63081cea, 0x1fc549db}
#define MOTORMOD_CLASS_ID               ClassID{0x26712b4d, 0x02213417}
#define PIN_CLASS_ID                    ClassID{0x41d14a7d, 0x793d56e4}
#define PINMOD_CLASS_ID                 ClassID{0x1f756c96, 0x26180a50}

#define BASICFLECTOR_CLASS_ID           ClassID{0x5e5259be, 0x47263c57}
#define BASICFLECTORMOD_CLASS_ID        ClassID{0x149a4a28, 0x0ffc17f5}
#define MONOFLECTOR_CLASS_ID            ClassID{0x5eea6cb7, 0x29027b3e}
#define PDYNADEF_CLASS_ID               ClassID{0x00b46c87, 0x3eee2ac4}
#define PDYNADEFMOD_CLASS_ID            ClassID{0x783f281d, 0x0ade1abe}
#define PLANARSPAWNDEF_CLASS_ID         ClassID{0x4e94628d, 0x4e437774}
#define PLANARSPAWNDEFMOD_CLASS_ID      ClassID{0x7d257b98, 0x439e09de}
#define SDYNADEFL_CLASS_ID              ClassID{0x44692f1c, 0x13ca051a}
#define SDYNADEFLMOD_CLASS_ID           ClassID{0x70051444, 0x2bf5270a}
#define SSPAWNDEFL_CLASS_ID             ClassID{0x656107ca, 0x1f284a6f}
#define SSPAWNDEFLMOD_CLASS_ID          ClassID{0x72a61178, 0x21b407d9}
#define UDYNADEFL_CLASS_ID              ClassID{0x685771aa, 0x678144bd}
#define UDYNADEFLMOD_CLASS_ID           ClassID{0x0cc400b8, 0x08732e2f}
#define USPAWNDEFL_CLASS_ID             ClassID{0x19fd4916, 0x557f71d9}
#define USPAWNDEFLMOD_CLASS_ID          ClassID{0x36350a51, 0x5073041f}
