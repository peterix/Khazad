#ifndef PATH_MANAGER__HEADER
#define PATH_MANAGER__HEADER

#include <stdafx.h>
#include <Singleton.h>
#include <Pool.h>

#include <Coordinates.h>

class MovementController;
class PathAlgorithm;
class KhazadGrid;
class AStar;
class Heuristic;
class Profile;
class MapPath;
class Timer;
class AStarNode;

class PathManager
{
    DECLARE_SINGLETON_CLASS(PathManager)

public:

    ~PathManager();
    bool Init();

    void CreateMapAbstraction();
    void EditMapAbstraction();
    void DeleteMapAbstraction();

    MovementController* getNewController(int AgentSize, int MovementType, MapCoordinates StartCoords); // Create a new controler

    MapPath* FindPath(int PathSystem, MapCoordinates StartCoords, MapCoordinates GoalCoords);
    MapPath* ProfilePath(int PathSystem, MapCoordinates StartCoords, MapCoordinates GoalCoords, Profile* TargetProfile);

    bool isPathPossible(int PathSystem, MapCoordinates StartCoords, MapCoordinates GoalCoords);
    float EstimatePathLength(int PathSystem, MapCoordinates StartCoords, MapCoordinates GoalCoords);

    uint32_t getDirectionFlags(MapCoordinates Coordinates) const;
    bool contains(MapCoordinates Coordinates) const;
    float getEdgeCost(const MapCoordinates &TestCoords, Direction DirectionType) const;


    uint32_t getZone(const MapCoordinates &TargetCoords) const;
    uint32_t getZoneEquivilency(const MapCoordinates &TargetCoords) const;

    CentralPool<AStarNode>* getCentralNodePool()         { return NodeCentralPool; }

protected:

    Timer* PathingTimer;

    PathAlgorithm* AstarImplementation;
    PathAlgorithm* HeriarchialAstarImplementation;

    std::vector<MovementController*> MoveControllers;

    KhazadGrid* MapGrid;

    Heuristic* ManhattenHeuristic;
    Heuristic* EuclideanHeuristic;
    Heuristic* MaxDimensionHeuristic;
    Heuristic* DijkstraHeuristic;
    Heuristic* DiagonalHeuristic;

    CentralPool<AStarNode>* NodeCentralPool;
};

#define PATH (PathManager::GetInstance())

#endif // PATH__HEADER
