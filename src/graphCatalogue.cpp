#include "global.h"
#include "graphCatalogue.h"

void GraphCatalogue::insertGraph(Graph* graph)
{
    logger.log("GraphCatalogue::insertGraph");
    this->graphs[graph->graphName] = graph;
}

Graph* GraphCatalogue::getGraph(string graphName)
{
    logger.log("GraphCatalogue::getGraph");
    if (this->isGraph(graphName))
        return this->graphs[graphName];
    return nullptr;
}

bool GraphCatalogue::isGraph(string graphName)
{
    logger.log("GraphCatalogue::isGraph");
    if (this->graphs.count(graphName))
        return true;
    return false;
}

void GraphCatalogue::deleteGraph(string graphName)
{
    logger.log("GraphCatalogue::deleteGraph");
    // TODO: Also delete the temporary files associated with the graph
    this->graphs.erase(graphName);
}

void GraphCatalogue::print()
{
    logger.log("GraphCatalogue::print");
    cout << "Graphs in Catalogue: " << this->graphs.size() << endl;
    for (auto const& [key, val] : this->graphs)
    {
        cout << key << endl;
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
