#include "global.h"
#include "graphCatalogue.h"

void GraphCatalogue::insertGraph(Graph* graph)
{
    logger.log("GraphCatalogue::insertGraph");
    this->graphs[{graph->graphName, graph->graphType}] = graph;
}

Graph* GraphCatalogue::getGraph(string graphName, GraphType graphType)
{
    logger.log("GraphCatalogue::getGraph");
    if (this->isGraph(graphName, graphType))
        return this->graphs.at({graphName, graphType});
    return nullptr;
}

bool GraphCatalogue::isGraph(string graphName, GraphType graphType)
{
    logger.log("GraphCatalogue::isGraph");
    if (this->graphs.count({graphName, graphType}))
        return true;
    return false;
}

void GraphCatalogue::deleteGraph(string graphName, GraphType graphType)
{
    logger.log("GraphCatalogue::deleteGraph");
    // TODO: Also delete the temporary files associated with the graph
    this->graphs.erase({graphName, graphType});
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
        delete val;
    }
}
