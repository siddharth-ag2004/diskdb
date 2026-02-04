#include "global.h"
#include "graph.h"

Graph::Graph() {
    logger.log("Graph::Graph");
}

Graph::Graph(string graphName, GraphType type) {
    logger.log("Graph::Graph");
    this->graphName = graphName;
    this->graphType = type;
    if (type == DIRECTED) {
        this->nodeFileName = "../data/" + graphName + "_Nodes_D.csv";
        this->edgeFileName = "../data/" + graphName + "_Edges_D.csv";
    } else {
        this->nodeFileName = "../data/" + graphName + "_Nodes_U.csv";
        this->edgeFileName = "../data/" + graphName + "_Edges_U.csv";
    }
}

bool Graph::load() {
    logger.log("Graph::load");
    // TODO: Implement loading of graph from CSV files into blocked format
    // For now, just a placeholder
    return true;
}

bool Graph::unload() {
    // TODO: Implement unloading of graph and deletion of temporary files
    logger.log("Graph::unload");
    return true;
}   

void Graph::print() {
    logger.log("Graph::print");
    // TODO: Implement printing of graph
}

void Graph::makePermanent() {
    logger.log("Graph::makePermanent");
    // TODO: Implement exporting of graph
}

bool Graph::isPermanent() {
    logger.log("Graph::isPermanent");
    // TODO: Implement check if graph is permanent
    return false;
}

bool Graph::findPath(int srcNodeId, int destNodeId, vector<string> conditions, string newGraphName) {
    logger.log("Graph::findPath");
    // TODO: Implement path finding algorithm
    return false;
}

int Graph::getDegree(int nodeId) {
    logger.log("Graph::getDegree");
    // TODO: Implement degree calculation
    return 0;
}
