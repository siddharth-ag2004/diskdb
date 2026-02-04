#pragma once

#include "graph.h"

// Custom hash function for std::pair<string, GraphType>
struct PairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

/**
 * @brief The GraphCatalogue acts like an index of graphs existing in the
 * system. Everytime a graph is added(removed) to(from) the system, it needs to
 * be added(removed) to(from) the graphCatalogue.
 *
 */
class GraphCatalogue
{

    unordered_map<pair<string, GraphType>, Graph*, PairHash> graphs;

public:
    GraphCatalogue() {}
    void insertGraph(Graph* graph);
    void deleteGraph(string graphName, GraphType graphType);
    Graph* getGraph(string graphName, GraphType graphType);
    bool isGraph(string graphName, GraphType graphType);
    void print();
    ~GraphCatalogue();
};
