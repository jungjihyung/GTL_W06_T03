#pragma once


enum OBJECTS : uint8
{
    OBJ_SPHERE,
    OBJ_CUBE,
    OBJ_DIRLIGHT,
    OBJ_SpotLight,
    OBJ_PointLight,
    OBJ_PARTICLE,
    OBJ_Text,
    OBJ_Fireball,
    OBJ_TRIANGLE,
    OBJ_CAMERA,
    OBJ_PLAYER,
    OBJ_Fog,
    OBJ_END
};

enum ARROW_DIR : uint8
{
    AD_X,
    AD_Y,
    AD_Z,
    AD_END
};

enum ControlMode : uint8
{
    CM_TRANSLATION,
    CM_ROTATION,
    CM_SCALE,
    CM_END
};

enum CoordiMode : uint8
{
    CDM_WORLD,
    CDM_LOCAL,
    CDM_END
};

enum EPrimitiveColor : uint8
{
    RED_X,
    GREEN_Y,
    BLUE_Z,
    NONE,
    RED_X_ROT,
    GREEN_Y_ROT,
    BLUE_Z_ROT
};
