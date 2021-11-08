#ifndef _SCENE_HIT_POINT_HLSLI
#define _SCENE_HIT_POINT_HLSLI

struct SceneHitPoint
{
    unsigned int modelOffset;
    float t;
    unsigned int sceneNodeIndex;
};

SceneHitPoint EmptySceneHitPoint()
{
    SceneHitPoint shp;
    
    shp.t = 0.0f;
    shp.modelOffset = 0;
    shp.sceneNodeIndex = 0;
    
    return shp;
}

#endif