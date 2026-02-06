#include "global.h"

/**
 * @brief 
 * SYNTAX: EXPORT <relation_name> 
 */

bool syntacticParseEXPORT()
{
    logger.log("syntacticParseEXPORT");
    if (tokenizedQuery.size() < 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    if (tokenizedQuery[1] == "GRAPH")
    {
        if (tokenizedQuery.size() != 3)
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.queryType = EXPORT_GRAPH;
        parsedQuery.exportRelationName = tokenizedQuery[2];
        return true;
    }
    else
    {
        if (tokenizedQuery.size() != 2)
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.queryType = EXPORT;
        parsedQuery.exportRelationName = tokenizedQuery[1];
        return true;
    }
}

bool semanticParseEXPORT()
{
    logger.log("semanticParseEXPORT");
    //Table should exist
    if (tableCatalogue.isTable(parsedQuery.exportRelationName))
        return true;
    cout << "SEMANTIC ERROR: No such relation exists" << endl;
    return false;
}

bool semanticParseEXPORTGraph()
{
    logger.log("semanticParseEXPORTGraph");
    if (graphCatalogue.isGraph(parsedQuery.exportRelationName, UNKOWN))
        return true;
    cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
    return false;
}

void executeEXPORT()
{
    logger.log("executeEXPORT");
    Table* table = tableCatalogue.getTable(parsedQuery.exportRelationName);
    table->makePermanent();
    return;
}

void executeEXPORTGraph()
{
    logger.log("executeEXPORTGraph");
    Graph* graph = graphCatalogue.getGraph(parsedQuery.exportRelationName, UNKOWN);
    graph->makePermanent();
    return;
}