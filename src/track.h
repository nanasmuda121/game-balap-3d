#pragma once
#include "common.h"
#include <vector>

class Track {
public:
    Track();
    ~Track();

    void Init();
    void Draw3D();
    void DrawMiniMap(Rectangle bounds, Vector3 playerPos, Vector3 botPos, float playerYaw, float botYaw);

    // Queries
    int GetClosestIndex(Vector3 pos) const;
    float GetProgressAlongTrack(Vector3 pos) const;
    bool CheckBarrierCollision(Vector3 carPos, float carRadius, Vector3& pushOutForce);
    
    // Waypoints access for Bot AI
    const std::vector<TrackPoint>& GetPoints() const { return m_points; }
    float GetTotalLength() const { return m_totalLength; }
    Vector3 GetPointAhead(int currentIndex, float lookAheadDistance) const;

private:
    void GenerateSpline();
    void GenerateDecorations();

    std::vector<Vector3> m_baseWaypoints;
    std::vector<TrackPoint> m_points;
    float m_totalLength;

    // Scenery positions
    struct Tree { Vector3 pos; float scale; };
    struct Billboard { Vector3 pos; float yaw; const char* text; Color color; };
    struct LightPole { Vector3 pos; float yaw; };

    std::vector<Tree> m_trees;
    std::vector<Billboard> m_billboards;
    std::vector<LightPole> m_lightPoles;
};
