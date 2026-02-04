#pragma once

#include "cursor.h"

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
    string nodeFileName = "";
    string edgeFileName = "";

    Graph();
    Graph(string graphName, GraphType type);

    bool load();
    bool unload();
    void print();
    void makePermanent();
    bool isPermanent();
    
    bool findPath(int srcNodeId, int destNodeId, vector<string> conditions, string newGraphName);
    
    int getDegree(int nodeId);
};
