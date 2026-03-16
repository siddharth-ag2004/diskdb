#include "global.h"
/**
 * @brief 
 * SYNTAX: LOAD relation_name
 */

bool syntacticParseLOADTable()
{
    logger.log("syntacticParseLOADTable");

    if (tokenizedQuery.size() != 2)
    {
        cout << "SYNTAX ERROR: Invalid number of tokens for table load" << endl;
        return false;
    }

    parsedQuery.queryType = LOAD;
    parsedQuery.entityType = TABLE;
    parsedQuery.loadRelationName = tokenizedQuery[1];
    return true;
}

bool syntacticParseLOADGraph()
{
    logger.log("syntacticParseLOADGraph");

    if (tokenizedQuery.size() != 4)
    {
        cout << "SYNTAX ERROR: Invalid number of tokens for graph load" << endl;
        return false;
    }

    if (tokenizedQuery[3] != "U" && tokenizedQuery[3] != "D")
    {
        cout << "SYNTAX ERROR: Invalid graph type (must be 'U' or 'D')" << endl;
        return false;
    }

    parsedQuery.queryType = LOAD_GRAPH;
    parsedQuery.entityType = GRAPH;
    parsedQuery.loadGraphRelationName = tokenizedQuery[2];
    parsedQuery.graphType = (tokenizedQuery[3] == "U") ? UNDIRECTED : DIRECTED;
    return true;
}

bool syntacticParseLOAD()
{
    logger.log("syntacticParseLOAD");

    if (tokenizedQuery.size() == 2)
    {
        return syntacticParseLOADTable();
    }
    else if (tokenizedQuery.size() == 4 && tokenizedQuery[1] == "GRAPH")
    {
        return syntacticParseLOADGraph();
    }
    else
    {
        cout << "SYNTAX ERROR: Invalid LOAD syntax" << endl;
        return false;
    }
}

bool semanticParseLOADTable()
{
    logger.log("semanticParseLOADTable");
    if (tableCatalogue.isTable(parsedQuery.loadRelationName))
    {
        cout << "SEMANTIC ERROR: Relation already exists" << endl;
        return false;
    }

    if (!isFileExists(parsedQuery.loadRelationName))
    {
        cout << "SEMANTIC ERROR: Data file doesn't exist" << endl;
        return false;
    }
    return true;
}

bool semanticParseLOADGraph()
{
    logger.log("semanticParseLOADGraph");

    if (graphCatalogue.isGraph(parsedQuery.loadGraphRelationName, parsedQuery.graphType))
    {
        cout << "SEMANTIC ERROR: Graph already exists" << endl;
        return false;
    }

    if (!isFileExists(parsedQuery.loadGraphRelationName))
    {
        cout << "SEMANTIC ERROR: Data file doesn't exist" << endl;
        return false;
    }

    return true;
}

void executeLOADTable()
{
    logger.log("executeLOAD");

    Table *table = new Table(parsedQuery.loadRelationName);
    if (table->load())
    {
        tableCatalogue.insertTable(table);
        cout << "Loaded Table. Column Count: " << table->columnCount << " Row Count: " << table->rowCount << endl;
    }
    return;
}

void executeLOADGraph()
{
    logger.log("executeLOADGraph");

    Graph *graph = new Graph(parsedQuery.loadGraphRelationName, parsedQuery.graphType);
    if (graph->load())
    {
        graphCatalogue.insertGraph(graph);
        cout << "Loaded Graph.Node Count:" << graph->nodeCount << ",Edge Count:" << graph->edgeCount << endl;
    }
    return;
}