#pragma once

#include "table.h"

enum GraphType {
    UNDIRECTED,
    DIRECTED
};

class Graph {
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
    bool unload();
    void print();
    void makePermanent();
    bool isPermanent();
    
    // Returns cursor on sortedNodeTable positioned at nodeId (or end)
    Cursor cursorToNode(int nodeId); 
    // Returns cursor on sortedEdgeTable (or reverse) positioned at start of neighbors
    Cursor cursorToNeighbors(int nodeId, bool searchReverse = false); 

    bool findPath(int srcNodeId, int destNodeId, vector<string> conditions, string newGraphName);
    
    int getDegree(int nodeId);
};
