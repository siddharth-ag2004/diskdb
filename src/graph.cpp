#include "global.h"
#include "graph.h"
#include "binarySearch.h"

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

    if (this->nodeTable) {
        Cursor cursor(this->nodeTable->tableName, 0);
        vector<int> row;
        for (long long i = 0; i < this->nodeCount; i++) {
            row = cursor.getNext();
            if (row.empty()) break; 
            this->nodeTable->writeRow(row, cout);
        }
    }
    cout << endl; 
    if (this->edgeTable) {
        Cursor cursor(this->edgeTable->tableName, 0);
        vector<int> row;
        for (long long i = 0; i < this->edgeCount; i++) {
            row = cursor.getNext();
            if (row.empty()) break;
            this->edgeTable->writeRow(row, cout);
        }
    }
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

int Graph::getDegree(int nodeId)
{
    logger.log("Graph::getDegree");

    long long degree = 0;

    // OUT DEGREE
    {
        Table* table = this->sortedEdgeTable; // sorted by Src
        pair<int,int> start = findFirstOccurrence(
            table->tableName,
            table->columns[0],  // SrcNodeID
            nodeId
        );

        pair<int,int> end = findFirstOccurrence(
            table->tableName,
            table->columns[0],
            nodeId + 1
        );

        degree += countRowsBetween(start, end, table);
    }

    //  IN DEGREE
    {
        Table* table = this->sortedReverseEdgeTable; // sorted by Dest
        pair<int,int> start = findFirstOccurrence(
            table->tableName,
            table->columns[1],  // DestNodeID
            nodeId
        );

        pair<int,int> end = findFirstOccurrence(
            table->tableName,
            table->columns[1],
            nodeId + 1
        );

        degree += countRowsBetween(start, end, table);
    }

    return (int)degree;
}

Cursor Graph::cursorToNode(int nodeId)
{
    logger.log("Graph::cursorToNode");

    Table* table = this->sortedNodeTable;
    string tableName = table->tableName;

    pair<int, int> loc = findFirstOccurrence(
        tableName,
        table->columns[0],  // NodeID column
        nodeId
    );

    // If table is empty or something went wrong
    if (loc.first == -1) {
        logger.log("Table is empty or error in binary search");
        Cursor endCursor(tableName, 0);
        endCursor.pagePointer = 0;
        return endCursor;
    }

    Cursor cursor(tableName, loc.first);
    cursor.pagePointer = loc.second;
    return cursor;
}


Cursor Graph::cursorToNeighbors(int nodeId, bool searchReverse) {
    // TODO: Implement binary search to find specific edge range
    if (searchReverse)
        return this->sortedReverseEdgeTable->getCursor();
    return this->sortedEdgeTable->getCursor();
}

long long Graph::countRowsBetween(
    pair<int,int> start,
    pair<int,int> end,
    Table* table
) {
    logger.log("Graph::countRowsBetween");

    if (table == nullptr)
        return 0;

    // Convert start to global row ID
    long long startRowId =
        (long long)start.first * table->maxRowsPerBlock
        + start.second;

    // Convert end to global row ID
    long long endRowId =
        (long long)end.first * table->maxRowsPerBlock
        + end.second;

    // Clamp to valid bounds
    if (startRowId < 0)
        startRowId = 0;

    if (endRowId > table->rowCount)
        endRowId = table->rowCount;

    if (endRowId < startRowId)
        return 0;

    return endRowId - startRowId;
}
