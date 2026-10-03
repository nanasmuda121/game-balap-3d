#pragma once
#include "common.h"
#include <vector>

struct BuildingInstance {
    Vector3 pos;
    float yaw;
    float scale;
    int type; // 0 = Villa, 1 = Apartment
};

struct StuntRamp {
    Vector3 pos;
    float yaw;
    Vector3 size; // width, height, length
};

class City {
public:
    City();
    ~City();

    void Init();
    void Draw3D();
    bool CheckCollision(Vector3 carPos, float carRadius, Vector3& pushOut);
    float GetSurfaceHeight(Vector3 carPos);

private:
    Model m_modelVilla;
    Model m_modelApt;
    bool m_villaLoaded;
    bool m_aptLoaded;

    std::vector<BuildingInstance> m_buildings;
    std::vector<StuntRamp> m_ramps;
    std::vector<Vector3> m_trees;
    std::vector<Vector3> m_streetLights;
};
