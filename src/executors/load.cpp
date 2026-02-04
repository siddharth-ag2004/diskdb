#include "global.h"
/**
 * @brief 
 * SYNTAX: LOAD relation_name
 */
bool syntacticParseLOAD()
{
    logger.log("syntacticParseLOAD");
    if (tokenizedQuery.size() != 2 && tokenizedQuery.size() != 4)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    
    if(tokenizedQuery.size() == 2) { // For Table
        parsedQuery.queryType = LOAD;
        parsedQuery.loadRelationName = tokenizedQuery[1];
    }
    else if(tokenizedQuery.size() == 4) { // For Graph
        if(tokenizedQuery[3] != "U" && tokenizedQuery[3] != "D") {
            cout << "SYNTAX ERROR: Invalid graph type (must be 'U' or 'D')" << endl;
            return false;
        }
        parsedQuery.queryType = LOAD_GRAPH;
        parsedQuery.loadGraphRelationName = tokenizedQuery[2];
        parsedQuery.graphType = (tokenizedQuery[3] == "U") ?  UNDIRECTED : DIRECTED;
    }
    return true;
}

bool semanticParseLOAD()
{
    logger.log("semanticParseLOAD");
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

void executeLOAD()
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