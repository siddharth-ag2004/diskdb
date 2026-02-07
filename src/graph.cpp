#include "global.h"
#include "graph.h"
#include "binarySearch.h"
#include <queue>
#include <map>
#include <algorithm>
#include <climits>
#include <set>

Graph::Graph(string graphName, GraphType type)
{
    logger.log("Graph::Graph");
    this->graphName = graphName;
    this->graphType = type;
}

bool Graph::load()
{
    logger.log("Graph::load");

    string suffix = (this->graphType == DIRECTED) ? "_D" : "_U";

    string nodeTableName = this->graphName + "_Nodes" + suffix;
    string nodeSourceFile = "../data/" + nodeTableName + ".csv";

    this->nodeTable = new Table(nodeTableName);

    this->nodeTable->sourceFileName = nodeSourceFile;

    if (!this->nodeTable->load())
    {
        cout << "i am here 1" << endl;
        delete this->nodeTable;
        return false;
    }
    tableCatalogue.insertTable(this->nodeTable);

    string edgeTableName = this->graphName + "_Edges" + suffix;
    string edgeSourceFile = "../data/" + edgeTableName + ".csv";

    this->edgeTable = new Table(edgeTableName);
    this->edgeTable->sourceFileName = edgeSourceFile;

    if (!this->edgeTable->load())
    {
        tableCatalogue.deleteTable(nodeTableName);
        delete this->edgeTable;
        cout << "i am here 2" << endl;
        return false;
    }
    tableCatalogue.insertTable(this->edgeTable);

    this->nodeCount = this->nodeTable->rowCount;
    this->edgeCount = this->edgeTable->rowCount;

    string sortedNodeName = this->graphName + "_Nodes_Sorted";
    if (tableCatalogue.isTable(sortedNodeName))
        tableCatalogue.deleteTable(sortedNodeName);

    this->nodeTable->externalSortCreateNewTable(sortedNodeName, 0);
    this->sortedNodeTable = tableCatalogue.getTable(sortedNodeName);

    string sortedEdgeName = this->graphName + "_Edges_Sorted_Src";
    if (tableCatalogue.isTable(sortedEdgeName))
        tableCatalogue.deleteTable(sortedEdgeName);

    this->edgeTable->externalSortCreateNewTable(sortedEdgeName, 0);
    this->sortedEdgeTable = tableCatalogue.getTable(sortedEdgeName);

    string sortedRevEdgeName = this->graphName + "_Edges_Sorted_Dest";
    if (tableCatalogue.isTable(sortedRevEdgeName))
        tableCatalogue.deleteTable(sortedRevEdgeName);

    this->edgeTable->externalSortCreateNewTable(sortedRevEdgeName, 1);
    this->sortedReverseEdgeTable = tableCatalogue.getTable(sortedRevEdgeName);

    return true;
}

bool Graph::unload()
{
    logger.log("Graph::unload");

    string suffix = (this->graphType == DIRECTED) ? "_D" : "_U";

    string names[] = {
        this->graphName + "_Nodes" + suffix,
        this->graphName + "_Edges" + suffix,
        this->graphName + "_Nodes_Sorted",
        this->graphName + "_Edges_Sorted_Src",
        this->graphName + "_Edges_Sorted_Dest"};

    for (const string &name : names)
    {
        if (tableCatalogue.isTable(name))
            tableCatalogue.deleteTable(name);
    }
    return true;
}

void Graph::print()
{
    logger.log("Graph::print");

    cout << this->nodeCount << endl;
    cout << this->edgeCount << endl;
    cout << (this->graphType == DIRECTED ? "D" : "U") << endl;
    cout << endl;

    if (this->nodeTable)
    {
        Cursor cursor(this->nodeTable->tableName, 0);
        vector<int> row;
        for (long long i = 0; i < this->nodeCount; i++)
        {
            row = cursor.getNext();
            if (row.empty())
                break;
            this->nodeTable->writeRow(row, cout);
        }
    }
    cout << endl;
    if (this->edgeTable)
    {
        Cursor cursor(this->edgeTable->tableName, 0);
        vector<int> row;
        for (long long i = 0; i < this->edgeCount; i++)
        {
            row = cursor.getNext();
            if (row.empty())
                break;
            this->edgeTable->writeRow(row, cout);
        }
    }
}

void Graph::makePermanent()
{
    logger.log("Graph::makePermanent");
    if (this->nodeTable)
        this->nodeTable->makePermanent();
    if (this->edgeTable)
        this->edgeTable->makePermanent();
}

bool Graph::isPermanent()
{
    logger.log("Graph::isPermanent");
    if (this->nodeTable && this->edgeTable)
        return this->nodeTable->isPermanent() && this->edgeTable->isPermanent();
    return false;
}

int Graph::getDegree(int nodeId)
{
    logger.log("Graph::getDegree");

    Cursor cursor = cursorToNode(nodeId);
    vector<int> row = cursor.getNext();
    
    if (row.size() == 0 || row[0] != nodeId) {
        cout << "Node does not exist" << endl;
        return -1;
    }

    long long degree = 0;

    // OUT DEGREE
    {
        Table *table = this->sortedEdgeTable; // sorted by Src
        pair<int, int> start = findFirstOccurrence(
            table->tableName,
            table->columns[0], // SrcNodeID
            nodeId);

        pair<int, int> end = findFirstOccurrence(
            table->tableName,
            table->columns[0],
            nodeId + 1);

        degree += countRowsBetween(start, end, table);
    }

    //  IN DEGREE
    {
        Table *table = this->sortedReverseEdgeTable; // sorted by Dest
        pair<int, int> start = findFirstOccurrence(
            table->tableName,
            table->columns[1], // DestNodeID
            nodeId);

        pair<int, int> end = findFirstOccurrence(
            table->tableName,
            table->columns[1],
            nodeId + 1);

        degree += countRowsBetween(start, end, table);
    }

    return (int)degree;
}

Cursor Graph::cursorToNode(int nodeId)
{
    logger.log("Graph::cursorToNode");

    Table *table = this->sortedNodeTable;
    string tableName = table->tableName;

    pair<int, int> loc = findFirstOccurrence(
        tableName,
        table->columns[0], // NodeID column
        nodeId);

    // If table is empty or something went wrong
    if (loc.first == -1)
    {
        logger.log("Table is empty or error in binary search");
        Cursor endCursor(tableName, 0);
        endCursor.pagePointer = 0;
        return endCursor;
    }

    Cursor cursor(tableName, loc.first);
    cursor.pagePointer = loc.second;
    return cursor;
}

Cursor Graph::cursorToNeighbors(int nodeId, bool searchReverse)
{
    logger.log("Graph::cursorToNeighbors");

    Table *table = searchReverse ? this->sortedReverseEdgeTable : this->sortedEdgeTable;
    // Col 0 is Src, Col 1 is Dest.
    // Forward Table sorted by Src (0). Reverse Table sorted by Dest (1).
    string columnName = table->columns[searchReverse ? 1 : 0];

    pair<int, int> location = findFirstOccurrence(table->tableName, columnName, nodeId);

    if (location.first != -1)
    {
        Cursor cursor(table->tableName, location.first);
        cursor.pagePointer = location.second;
        return cursor;
    }

    int lastPageIndex = table->blockCount > 0 ? table->blockCount - 1 : 0;
    Cursor endCursor(table->tableName, lastPageIndex);
    endCursor.pagePointer = (table->blockCount > 0) ? table->rowsPerBlockCount.back() : 0;
    return endCursor;
}

long long Graph::countRowsBetween(
    pair<int, int> start,
    pair<int, int> end,
    Table *table)
{
    logger.log("Graph::countRowsBetween");

    if (table == nullptr)
        return 0;

    // Convert start to global row ID
    long long startRowId =
        (long long)start.first * table->maxRowsPerBlock + start.second;

    // Convert end to global row ID
    long long endRowId =
        (long long)end.first * table->maxRowsPerBlock + end.second;

    // Clamp to valid bounds
    if (startRowId < 0)
        startRowId = 0;

    if (endRowId > table->rowCount)
        endRowId = table->rowCount;

    if (endRowId < startRowId)
        return 0;

    return endRowId - startRowId;
}

int Graph::getAttributeValue(const vector<int> &row, const vector<string> &columns, string colName)
{
    for (size_t i = 0; i < columns.size(); i++)
    {
        if (columns[i] == colName)
            return row[i];
    }
    return -1;
}

bool Graph::checkConditions(const vector<int> &row, const vector<string> &columns,
                            const vector<PathCondition> &conditions, char type)
{
    if (row.empty())
        return false;
    for (const auto &cond : conditions)
    {
        if (cond.type != type)
            continue;
        if (cond.attribute == "ANY")
            continue;
        int val = getAttributeValue(row, columns, cond.attribute);
        if (cond.isExplicit)
        {
            if (val != cond.value)
                return false;
        }
    }
    return true;
}

struct PQItem
{
    int u, cost;
    bool operator>(const PQItem &other) const { return cost > other.cost; }
};

PathResult Graph::runDijkstra(int src, int dest, const vector<PathCondition> &conditions)
{
    priority_queue<PQItem, vector<PQItem>, greater<PQItem>> pq;
    map<int, int> dist;
    map<int, pair<int, vector<int>>> parent;

    pq.push({src, 0});
    dist[src] = 0;

    // Check Source Node
    Cursor srcC = cursorToNode(src);
    vector<int> srcRow = srcC.getNext();
    if (srcRow.empty() || srcRow[0] != src || !checkConditions(srcRow, nodeTable->columns, conditions, 'N'))
    {
        return {false, 0, {}, {}};
    }

    PathResult result;
    result.found = false;

    while (!pq.empty())
    {
        int u = pq.top().u;
        int d = pq.top().cost;
        pq.pop();

        if (d > dist[u])
            continue;

        if (u == dest)
        {
            // Check Dest Conditions
            Cursor destC = cursorToNode(dest);
            vector<int> destRow = destC.getNext();
            if (destRow.empty() || destRow[0] != dest || !checkConditions(destRow, nodeTable->columns, conditions, 'N'))
            {
                continue;
            }
            result.found = true;
            result.totalWeight = d;
            break;
        }

        int iterations = (graphType == DIRECTED) ? 1 : 2;
        for (int k = 0; k < iterations; k++)
        {
            bool searchReverse = (k == 1);
            Cursor edgeCursor = cursorToNeighbors(u, searchReverse);

            while (true)
            {
                vector<int> edgeRow = edgeCursor.getNext();
                // if (edgeRow.empty())
                if (edgeRow.empty() || (searchReverse == 0 && edgeRow[0] != u ) || (searchReverse == 1 && edgeRow[1] != u))
                    break;

                int rowSrc = edgeRow[0];
                int rowDest = edgeRow[1];
                int weight = edgeRow[2];
                int v = -1;

                if (!searchReverse)
                {
                    if (rowSrc != u)
                        break;
                    v = rowDest;
                }
                else
                {
                    if (rowDest != u)
                        break;
                    v = rowSrc;
                }

                if (!checkConditions(edgeRow, edgeTable->columns, conditions, 'E'))
                    continue;

                if (dist.find(v) == dist.end() || d + weight < dist[v])
                {
                    if (v != dest)
                    {
                        Cursor nodeC = cursorToNode(v);
                        vector<int> nodeRow = nodeC.getNext();
                        if (nodeRow.empty() || nodeRow[0] != v || !checkConditions(nodeRow, nodeTable->columns, conditions, 'N'))
                            continue;
                    }
                    dist[v] = d + weight;
                    parent[v] = {u, edgeRow};
                    pq.push({v, dist[v]});
                }
            }
        }
    }

    if (result.found)
    {
        int curr = dest;
        while (curr != src)
        {
            result.pathNodes.push_back(curr);
            auto p = parent[curr];
            result.pathEdges.push_back(p.second);
            curr = p.first;
        }
        result.pathNodes.push_back(src);
        reverse(result.pathNodes.begin(), result.pathNodes.end());
        reverse(result.pathEdges.begin(), result.pathEdges.end());
    }
    return result;
}

bool Graph::findPath(string resultGraphName, int srcNodeId, int destNodeId, vector<PathCondition> conditions)
{
    logger.log("Graph::findPath");

    Cursor srcCursor = cursorToNode(srcNodeId);
    vector<int> srcRow = srcCursor.getNext();
    Cursor destCursor = cursorToNode(destNodeId);
    vector<int> destRow = destCursor.getNext();

    if (srcRow.empty() || srcRow[0] != srcNodeId || destRow.empty() || destRow[0] != destNodeId)
    {
        cout << "Node does not exist" << endl;
        return false;
    }

    // resolving ANY(N)
    vector<vector<PathCondition>> firstConditions;
    vector<string> node_columns = this->nodeTable->columns;
    node_columns.assign(node_columns.begin()+1, node_columns.end());
    vector<string> edge_columns = this->edgeTable->columns;
    edge_columns.assign(edge_columns.begin()+3,edge_columns.end());
    bool any_n_0 = false;
    bool any_n_1 = false;
    bool any_e_0 = false;
    bool any_e_1 = false;

    vector<PathCondition> conditions_without_any;

    for (auto &cond : conditions)
    {
        if (cond.type == 'N' && !cond.isExplicit && cond.attribute == "ANY")
        {
            any_n_0 = true;
            any_n_1 = true;
        }
        else if (cond.type == 'N' && cond.attribute == "ANY" && cond.isExplicit)
        {
            if (cond.value == 0)
            {
                any_n_0 = true;
            }
            else if (cond.value == 1)
            {
                any_n_1 = true;
            }
            else
            {
                cout << "SEMANTIC ERROR: Invalid value for ANY(N) condition" << endl;
                return false;
            }
        }
        else if (cond.type == 'E' && !cond.isExplicit && cond.attribute == "ANY")
        {
            any_e_0 = true;
            any_e_1 = true;
        }
        else if (cond.type == 'E' && cond.attribute == "ANY" && cond.isExplicit)
        {
            if (cond.value == 0)
            {
                any_e_0 = true;
            }
            else if (cond.value == 1)
            {
                any_e_1 = true;
            }
            else
            {
                cout << "SEMANTIC ERROR: Invalid value for ANY(E) condition" << endl;
                return false;
            }
        }
        else if (cond.attribute != "ANY")
        {
            conditions_without_any.push_back(cond);
        }
    }

    firstConditions.push_back(conditions_without_any);
    vector<vector<PathCondition>> secondConditions;
    for (auto &plan : firstConditions)
    {
        for (auto &c_name : node_columns)
        {
            int val = getAttributeValue(srcRow, nodeTable->columns, c_name);
            if (val == -1)
            {
                cout << "SEMANTIC ERROR: Attribute " << c_name << " missing" << endl;
                return false;
            }

            vector<PathCondition> temp_plan = plan;
            if (val == 0 && any_n_0)
            {
                PathCondition fixed_n_att;
                fixed_n_att.attribute = c_name;
                fixed_n_att.value = 0;
                fixed_n_att.isExplicit = true;
                fixed_n_att.type = 'N';
                temp_plan.push_back(fixed_n_att);
                secondConditions.push_back(temp_plan);
            }
            else if (val == 1 && any_n_1)
            {
                PathCondition fixed_n_att;
                fixed_n_att.attribute = c_name;
                fixed_n_att.value = 1;
                fixed_n_att.isExplicit = true;
                fixed_n_att.type = 'N';
                temp_plan.push_back(fixed_n_att);
                secondConditions.push_back(temp_plan);
            }

        }
    }

    // if there is any update due to ANY(N)
    if (secondConditions.size() > 0){
     firstConditions = secondConditions;
    }

    
    // handle ANY(E)
    secondConditions.clear();

    for (auto& plan: firstConditions)
    {
        for(auto& c_name: edge_columns){
            vector<PathCondition> temp_plan = plan;
            if (any_e_0){
                PathCondition fixed_e_att;
                fixed_e_att.attribute = c_name;
                fixed_e_att.value = 0;
                fixed_e_att.isExplicit = true;
                fixed_e_att.type = 'E';
                temp_plan.push_back(fixed_e_att);
                secondConditions.push_back(temp_plan);
            }
            temp_plan = plan;
            if (any_e_1){
                PathCondition fixed_e_att;
                fixed_e_att.attribute = c_name;
                fixed_e_att.value = 1;
                fixed_e_att.isExplicit = true;
                fixed_e_att.type = 'E';
                temp_plan.push_back(fixed_e_att);
                secondConditions.push_back(temp_plan);
            }
        }
    }
    // if there is any update due to ANY(E)
    if (secondConditions.size()>0)
    {
        firstConditions = secondConditions;
    }

    

    // 1. Resolve Implicit Node Conditions
    secondConditions.clear();
    for (auto& plan: firstConditions){
        vector<PathCondition> baseConditions;
        for (auto &cond : plan)
        {
            if (cond.type == 'N' && !cond.isExplicit && cond.attribute != "ANY")
            {
                int val = getAttributeValue(srcRow, nodeTable->columns, cond.attribute);
                if (val == -1)
                {
                    cout << "SEMANTIC ERROR: Attribute " << cond.attribute << " missing" << endl;
                    return false;
                }
                PathCondition fixed = cond;
                fixed.value = val;
                fixed.isExplicit = true;
                baseConditions.push_back(fixed);
            }
            else
            {
                baseConditions.push_back(cond);
            }
        }
        secondConditions.push_back(baseConditions);
    }

    firstConditions = secondConditions;



    // 2. Generate Plans (Implicit Edge 0/1 branching)
    vector<vector<PathCondition>> executionPlans;
    // executionPlans.push_back(baseConditions);
    executionPlans = firstConditions;
    vector<PathCondition> baseConditions = firstConditions[0];
    for (int i = 0; i < (int)baseConditions.size(); i++)
    {
        if (baseConditions[i].type == 'E' && !baseConditions[i].isExplicit && baseConditions[i].attribute != "ANY")
        {
            vector<vector<PathCondition>> newPlans;
            for (auto &plan : executionPlans)
            {
                auto p0 = plan;
                p0[i].value = 0;
                p0[i].isExplicit = true;
                newPlans.push_back(p0);
                auto p1 = plan;
                p1[i].value = 1;
                p1[i].isExplicit = true;
                newPlans.push_back(p1);
            }
            executionPlans = newPlans;
        }
    }

    // Once print all the conditions
    // for (const auto &plan : executionPlans)
    // {
    //     for (const auto &cond : plan)
    //     {
    //         cout << cond.type << "(" << cond.attribute << ") = " << cond.value << (cond.isExplicit ? " [Explicit]" : " [Implicit]") << " ";
    //     }
    //     cout << endl;
    // }



    // 3. Run Dijkstra
    PathResult bestResult;
    bestResult.found = false;
    bestResult.totalWeight = INT_MAX;

    for (const auto &plan : executionPlans)
    {
        PathResult res = runDijkstra(srcNodeId, destNodeId, plan);
        if (res.found)
        {
            if (!bestResult.found || res.totalWeight < bestResult.totalWeight)
            {
                bestResult = res;
            }
        }
    }

    if (!bestResult.found)
    {
        cout << "FALSE" << endl;
        return false;
    }

    cout << "TRUE " << bestResult.totalWeight << endl;

    // 4. Save Result (Write to temporary files)
    string suffix = (this->graphType == DIRECTED) ? "_D" : "_U";
    string resNodeName = resultGraphName + "_Nodes" + suffix;
    string resEdgeName = resultGraphName + "_Edges" + suffix;

    // Clean up previous existence of result files
    if (tableCatalogue.isTable(resNodeName))
        tableCatalogue.deleteTable(resNodeName);
    if (tableCatalogue.isTable(resEdgeName))
        tableCatalogue.deleteTable(resEdgeName);

    Table *resNodes = new Table(resNodeName, nodeTable->columns);
    Table *resEdges = new Table(resEdgeName, edgeTable->columns);

    for (int nodeId : bestResult.pathNodes)
    {
        Cursor c = cursorToNode(nodeId);
        resNodes->writeRow(c.getNext());
    }
    for (const auto &eRow : bestResult.pathEdges)
    {
        resEdges->writeRow(eRow);
    }

    // Blockify and insert into catalogue (as temporary tables)
    if (resNodes->blockify())
    {
        tableCatalogue.insertTable(resNodes);
    }
    else
    {
        tableCatalogue.insertTable(resNodes);
    }

    if (resEdges->blockify())
    {
        tableCatalogue.insertTable(resEdges);
    }
    else
    {
        tableCatalogue.insertTable(resEdges);
    }

    // 5. Create and Register Graph Object directly

    if (graphCatalogue.isGraph(resultGraphName, this->graphType))
    {
        graphCatalogue.deleteGraph(resultGraphName, this->graphType);
    }

    Graph *resGraph = new Graph(resultGraphName, this->graphType);

    // Manually set tables to the ones we just created (which are now in catalogue)
    resGraph->nodeTable = resNodes;
    resGraph->edgeTable = resEdges;
    resGraph->nodeCount = resNodes->rowCount;
    resGraph->edgeCount = resEdges->rowCount;

    string sortedNodeName = resGraph->graphName + "_Nodes_Sorted";
    if (tableCatalogue.isTable(sortedNodeName))
        tableCatalogue.deleteTable(sortedNodeName);
    resNodes->externalSortCreateNewTable(sortedNodeName, 0);
    resGraph->sortedNodeTable = tableCatalogue.getTable(sortedNodeName);

    string sortedEdgeName = resGraph->graphName + "_Edges_Sorted_Src";
    if (tableCatalogue.isTable(sortedEdgeName))
        tableCatalogue.deleteTable(sortedEdgeName);
    resEdges->externalSortCreateNewTable(sortedEdgeName, 0);
    resGraph->sortedEdgeTable = tableCatalogue.getTable(sortedEdgeName);

    string sortedRevEdgeName = resGraph->graphName + "_Edges_Sorted_Dest";
    if (tableCatalogue.isTable(sortedRevEdgeName))
        tableCatalogue.deleteTable(sortedRevEdgeName);
    resEdges->externalSortCreateNewTable(sortedRevEdgeName, 1);
    resGraph->sortedReverseEdgeTable = tableCatalogue.getTable(sortedRevEdgeName);

    graphCatalogue.insertGraph(resGraph);

    return true;
}