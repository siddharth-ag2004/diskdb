#pragma once

#include "graph.h"

/**
 * @brief The GraphCatalogue acts like an index of graphs existing in the
 * system. Everytime a graph is added(removed) to(from) the system, it needs to
 * be added(removed) to(from) the graphCatalogue.
 *
 */
class GraphCatalogue
{

    unordered_map<string, Graph*> graphs;

public:
    GraphCatalogue() {}
    void insertGraph(Graph* graph);
    void deleteGraph(string graphName);
    Graph* getGraph(string graphName);
    bool isGraph(string graphName);
    void print();
    ~GraphCatalogue();
};
