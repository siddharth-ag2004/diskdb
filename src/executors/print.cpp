#include "global.h"
/**
 * @brief 
 * SYNTAX: PRINT relation_name
 */
bool syntacticParsePRINT()
{
    logger.log("syntacticParsePRINT");
    if (tokenizedQuery.size() == 2)
    {
        parsedQuery.queryType = PRINT;
        parsedQuery.printRelationName = tokenizedQuery[1];
        parsedQuery.entityType = TABLE;
        return true;
    }
    else if (tokenizedQuery.size() == 3 && tokenizedQuery[1] == "GRAPH")
    {
        parsedQuery.queryType = PRINT;
        parsedQuery.printRelationName = tokenizedQuery[2];
        parsedQuery.entityType = GRAPH;
        return true;
    }
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
}

bool semanticParsePRINT()
{
    logger.log("semanticParsePRINT");
    if (parsedQuery.entityType == TABLE)
    {
        if (!tableCatalogue.isTable(parsedQuery.printRelationName))
        {
            cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
            return false;
        }
    }
    else if (parsedQuery.entityType == GRAPH)
    {
        if (graphCatalogue.isGraph(parsedQuery.printRelationName, DIRECTED))
        {
            parsedQuery.graphType = DIRECTED;
        }
        else if (graphCatalogue.isGraph(parsedQuery.printRelationName, UNDIRECTED))
        {
            parsedQuery.graphType = UNDIRECTED;
        }
        else
        {
            cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
            return false;
        }
    }
    return true;
}

void executePRINT()
{
    logger.log("executePRINT");
    if (parsedQuery.entityType == TABLE)
    {
        Table* table = tableCatalogue.getTable(parsedQuery.printRelationName);
        table->print();
    }
    else if (parsedQuery.entityType == GRAPH)
    {
        Graph* graph = graphCatalogue.getGraph(parsedQuery.printRelationName, parsedQuery.graphType);
        graph->print();
    }
    return;
}
