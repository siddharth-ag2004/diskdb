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
        cout << "SEMANTIC ERROR: Graph not found" << endl;
        return false;
    }
    return true;
}

void executeDEGREE()
{
    logger.log("executeDEGREE");
    Graph* graph = graphCatalogue.getGraph(parsedQuery.degreeGraphName, UNKOWN);

    int degree = graph->getDegree(parsedQuery.degreeNodeId);
    cout << "Degree of node " << parsedQuery.degreeNodeId << " in graph " << parsedQuery.degreeGraphName << " is: " << degree << endl;
    return;
}
