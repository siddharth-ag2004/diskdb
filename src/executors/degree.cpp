#include "global.h"

bool syntacticParseDEGREE()
{
    logger.log("syntacticParseDEGREE");
    if (tokenizedQuery.size() != 3)
    {
        cout << "SYNTAX ERROR: Incorrect number of arguments for DEGREE" << endl;
        return false;
    }
    parsedQuery.queryType = DEGREE;
    parsedQuery.degreeGraphName = tokenizedQuery[1];
    parsedQuery.degreeNodeId = stoi(tokenizedQuery[2]);
    return true;
}

bool semanticParseDEGREE()
{
    logger.log("semanticParseDEGREE");
    if (!graphCatalogue.isGraph(parsedQuery.degreeGraphName, UNKOWN))
    {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return false;
    }
    return true;
}

void executeDEGREE()
{
    logger.log("executeDEGREE");
    Graph* graph = graphCatalogue.getGraph(parsedQuery.degreeGraphName, UNKOWN);

    int degree = graph->getDegree(parsedQuery.degreeNodeId);
    if (degree != -1)
        cout << degree << endl;
    return;
}
