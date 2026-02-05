#include "global.h"
#include "graph.h"

Graph::Graph(string graphName, GraphType type) {
    logger.log("Graph::Graph");
    this->graphName = graphName;
    this->graphType = type;
}

bool Graph::load() {
    logger.log("Graph::load");

    string suffix = (this->graphType == DIRECTED) ? "_D" : "_U";
    
    string nodeTableName = this->graphName + "_Nodes" + suffix;
    string nodeSourceFile = "../data/" + nodeTableName + ".csv";
    
    this->nodeTable = new Table(nodeTableName);

    this->nodeTable->sourceFileName = nodeSourceFile;
    
    if (!this->nodeTable->load()) {
        delete this->nodeTable;
        return false;
    }
    tableCatalogue.insertTable(this->nodeTable);

    string edgeTableName = this->graphName + "_Edges" + suffix;
    string edgeSourceFile = "../data/" + edgeTableName + ".csv";

    this->edgeTable = new Table(edgeTableName);
    this->edgeTable->sourceFileName = edgeSourceFile;

    if (!this->edgeTable->load()) {
        tableCatalogue.deleteTable(nodeTableName);
        delete this->edgeTable; 
        return false;
    }
    tableCatalogue.insertTable(this->edgeTable);

    this->nodeCount = this->nodeTable->rowCount;
    this->edgeCount = this->edgeTable->rowCount;

    string sortedNodeName = this->graphName + "_Nodes_Sorted";
    if (tableCatalogue.isTable(sortedNodeName)) tableCatalogue.deleteTable(sortedNodeName);
    
    this->nodeTable->externalSortCreateNewTable(sortedNodeName, 0);
    this->sortedNodeTable = tableCatalogue.getTable(sortedNodeName);

    string sortedEdgeName = this->graphName + "_Edges_Sorted_Src";
    if (tableCatalogue.isTable(sortedEdgeName)) tableCatalogue.deleteTable(sortedEdgeName);

    this->edgeTable->externalSortCreateNewTable(sortedEdgeName, 0);
    this->sortedEdgeTable = tableCatalogue.getTable(sortedEdgeName);

    string sortedRevEdgeName = this->graphName + "_Edges_Sorted_Dest";
    if (tableCatalogue.isTable(sortedRevEdgeName)) tableCatalogue.deleteTable(sortedRevEdgeName);

    this->edgeTable->externalSortCreateNewTable(sortedRevEdgeName, 1);
    this->sortedReverseEdgeTable = tableCatalogue.getTable(sortedRevEdgeName);

    return true;
}


bool Graph::unload() {
    logger.log("Graph::unload");
    
    string suffix = (this->graphType == DIRECTED) ? "_D" : "_U";
    
    string names[] = {
        this->graphName + "_Nodes" + suffix,
        this->graphName + "_Edges" + suffix,
        this->graphName + "_Nodes_Sorted",
        this->graphName + "_Edges_Sorted_Src",
        this->graphName + "_Edges_Sorted_Dest"
    };

    for (const string& name : names) {
        if (tableCatalogue.isTable(name))
            tableCatalogue.deleteTable(name);
    }
    
    return true;
}   

void Graph::print() {
    logger.log("Graph::print");
    
    cout << this->nodeCount << endl;
    cout << this->edgeCount << endl;
    cout << (this->graphType == DIRECTED ? "D" : "U") << endl;
    cout << endl; 

    if (this->nodeTable) this->nodeTable->print();
    cout << endl; 
    if (this->edgeTable) this->edgeTable->print();
}

void Graph::makePermanent() {
    logger.log("Graph::makePermanent");
    if (this->nodeTable) this->nodeTable->makePermanent();
    if (this->edgeTable) this->edgeTable->makePermanent();
}

bool Graph::isPermanent() {
    logger.log("Graph::isPermanent");
    if (this->nodeTable && this->edgeTable)
        return this->nodeTable->isPermanent() && this->edgeTable->isPermanent();
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

Cursor Graph::cursorToNode(int nodeId) {
    // TODO: Implement binary search to find specific node
    return this->sortedNodeTable->getCursor();
}

Cursor Graph::cursorToNeighbors(int nodeId, bool searchReverse) {
    // TODO: Implement binary search to find specific edge range
    if (searchReverse)
        return this->sortedReverseEdgeTable->getCursor();
    return this->sortedEdgeTable->getCursor();
}