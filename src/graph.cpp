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

    string nodeRelationName = this->graphName + "_Nodes";
    bool nodesLoaded = this->blockify(this->nodeFileName, nodeRelationName, this->nodeBlockCount, this->nodeCount, this->nodeColumnCount, this->nodesPerBlock);

    if (!nodesLoaded) {
        return false;
    }

    string edgeRelationName = this->graphName + "_Edges";
    bool edgesLoaded = this->blockify(this->edgeFileName, edgeRelationName, this->edgeBlockCount, this->edgeCount, this->edgeColumnCount, this->edgesPerBlock);

    if (!edgesLoaded) {
        // If blockify failed, check if it was because the edge file is valid but empty (only a header).
        ifstream edgeFile(this->edgeFileName);
        if (!edgeFile.is_open()) {
            return false; // File truly doesn't exist.
        }
        string line;
        if (getline(edgeFile, line) && !edgeFile.eof()) { // Header exists.
            if (edgeFile.peek() == EOF) { // No more content after header.
                 // This is a valid empty edge file. We need to set its metadata.
                stringstream s_header(line);
                string word;
                this->edgeColumnCount = 0;
                while (getline(s_header, word, ',')) {
                    this->edgeColumnCount++;
                }
                if (this->edgeColumnCount > 0) {
                    this->edgesPerBlock = (uint)((BLOCK_SIZE * 1000) / (sizeof(int) * this->edgeColumnCount));
                }
                this->edgeCount = 0;
                this->edgeBlockCount = 0;
                return true; // Success.
            }
        }
        return false; // Some other error occurred.
    }

    return true;
}

bool Graph::blockify(string inputFileName, string relationName, int &blockCount, long long int &rowCount, int &columnCount, uint &rowsPerBlock)
{
    logger.log("Graph::blockify");
    ifstream fin(inputFileName, ios::in);
    if (!fin.is_open()) {
        return false;
    }

    string line, word;
    if (!getline(fin, line)) {
        fin.close();
        return false;
    }

    stringstream s_header(line);
    columnCount = 0;
    while (getline(s_header, word, ',')) {
        columnCount++;
    }

    if (columnCount == 0) {
        fin.close();
        return false;
    }
    rowsPerBlock = (uint)((BLOCK_SIZE * 1000) / (sizeof(int) * columnCount));
    if (rowsPerBlock == 0) {
        fin.close();
        return false;
    }

    vector<int> row(columnCount, 0);
    vector<vector<int>> rowsInPage(rowsPerBlock, row);
    int pageCounter = 0;
    blockCount = 0;
    rowCount = 0;

    while (getline(fin, line)) {
        stringstream s_row(line);
        for (int i = 0; i < columnCount; i++) {
            if (!getline(s_row, word, ',')) {
                fin.close();
                return false;
            }
            try {
                row[i] = stoi(word);
            } catch (const std::exception& e) {
                fin.close();
                return false;
            }
        }
        rowsInPage[pageCounter] = row;
        pageCounter++;
        rowCount++;
        if (pageCounter == rowsPerBlock) {
            bufferManager.writePage(relationName, blockCount, rowsInPage, pageCounter);
            blockCount++;
            pageCounter = 0;
        }
    }

    if (pageCounter > 0) {
        bufferManager.writePage(relationName, blockCount, rowsInPage, pageCounter);
        blockCount++;
    }

    fin.close();

    if (rowCount == 0) {
        return false;
    }

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
