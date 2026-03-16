#include "global.h"

bool syntacticParseSET_BUFFER()
{
    logger.log("syntacticParseSET_BUFFER");
    if (tokenizedQuery.size() != 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    
    parsedQuery.queryType = SET_BUFFER;
    
    try {
        parsedQuery.setBufferSize = stoi(tokenizedQuery[1]);
    } catch (...) {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    
    return true;
}

bool semanticParseSET_BUFFER()
{
    logger.log("semanticParseSET_BUFFER");
    if (parsedQuery.setBufferSize < 2 || parsedQuery.setBufferSize > 10)
    {
        cout << "SEMANTIC ERROR: Buffer size must be between 2 and 10" << endl;
        return false;
    }
    return true;
}

void executeSET_BUFFER()
{
    logger.log("executeSET_BUFFER");
    BLOCK_COUNT = parsedQuery.setBufferSize;
    return;
}