#include "global.h"
#include "graphCatalogue.h"

void GraphCatalogue::insertGraph(Graph* graph)
{
    logger.log("GraphCatalogue::insertGraph");
    this->graphs[{graph->graphName, graph->graphType}] = graph;
}

Graph* GraphCatalogue::getGraph(string graphName, GraphType graphType)
{
    if (graphType == UNKOWN) {
        auto itD = graphs.find({graphName, DIRECTED});
        if (itD != graphs.end())
            return itD->second;

        auto itU = graphs.find({graphName, UNDIRECTED});
        if (itU != graphs.end())
            return itU->second;

        return nullptr;
    }

    auto it = graphs.find({graphName, graphType});
    if (it == graphs.end())
        return nullptr;

    return it->second;
}

bool GraphCatalogue::isGraph(string graphName, GraphType graphType)
{
    if (graphType == UNKOWN) {
        return graphs.find({graphName, DIRECTED}) != graphs.end()
            || graphs.find({graphName, UNDIRECTED}) != graphs.end();
    }

    return graphs.find({graphName, graphType}) != graphs.end();
}


void GraphCatalogue::deleteGraph(string graphName, GraphType graphType)
{
    logger.log("GraphCatalogue::deleteGraph");
    if (this->isGraph(graphName, graphType)) {
        Graph* g = this->graphs.at({graphName, graphType});
        g->unload(); 
        delete g;
        this->graphs.erase({graphName, graphType});
    }
}

void GraphCatalogue::print()
{
    logger.log("GraphCatalogue::print");
    cout << "Graphs in Catalogue: " << this->graphs.size() << endl;
    for (auto const& [key, val] : this->graphs)
    {
        cout << key.first << " (" << (key.second == DIRECTED ? "Directed" : "Undirected") << ")" << endl;
    }
}

GraphCatalogue::~GraphCatalogue()
{
    logger.log("GraphCatalogue::~GraphCatalogue");
    for (auto const& [key, val] : this->graphs)
    {
        val->unload();
        delete val;
    }
}
