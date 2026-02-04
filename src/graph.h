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
    int nodeColumnCount = 0;
    int edgeColumnCount = 0;
    int nodeBlockCount = 0;
    int edgeBlockCount = 0;
    uint nodesPerBlock = 0;
    uint edgesPerBlock = 0;
    string nodeFileName = "";
    string edgeFileName = "";

private:
    bool blockify(string inputFileName, string relationName, int &blockCount, long long int &rowCount, int &columnCount, uint &rowsPerBlock);

public:
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
