#pragma once

#include "table.h"
#include "sortingStrategy.h"

enum GraphType {
    UNDIRECTED,
    DIRECTED,
    UNKOWN,
};
struct PathCondition {
            string attribute; 
            char type;        
            int value;        
            bool isExplicit;  
    };
struct PathResult {
    bool found;
    int totalWeight;
    vector<int> pathNodes;         
    vector<vector<int>> pathEdges; 
};

class Graph {

private:
   // Helpers
    int getAttributeValue(const vector<int>& row, const vector<string>& columns, string colName);
    bool checkConditions(const vector<int>& row, const vector<string>& columns, 
                         const vector<PathCondition>& conditions, char type);
    PathResult runDijkstra(int src, int dest, const vector<PathCondition>& conditions);

public:
    string graphName = "";
    GraphType graphType;
    long long int nodeCount = 0;
    long long int edgeCount = 0;
    
    Table* nodeTable = nullptr;            // Raw loaded nodes
    Table* edgeTable = nullptr;            // Raw loaded edges
    Table* sortedNodeTable = nullptr;      // Nodes sorted by ID (Col 0)
    Table* sortedEdgeTable = nullptr;      // Edges sorted by Src (Col 0)
    Table* sortedReverseEdgeTable = nullptr; // Edges sorted by Dest (Col 1)

    Graph(string graphName, GraphType type);

    bool load();
    bool loadResult();
    bool unload();
    void print();
    void makePermanent();
    bool isPermanent();
    
    // Returns cursor on sortedNodeTable positioned at nodeId (or end)
    Cursor cursorToNode(int nodeId); 
    // Returns cursor on sortedEdgeTable (or reverse) positioned at start of neighbors
    Cursor cursorToNeighbors(int nodeId, bool searchReverse = false); 

    long long countRowsBetween(
        pair<int,int> start,
        pair<int,int> end,
        Table* table
    );

    // bool findPath(int srcNodeId, int destNodeId, vector<string> conditions, string newGraphName);
    
    int getDegree(int nodeId);
    bool findPath(string resultGraphName, int srcNodeId, int destNodeId, 
                  vector<PathCondition> conditions);
};
