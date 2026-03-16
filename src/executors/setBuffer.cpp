#include "global.h"

/**
 * @brief 
 * SYNTAX: SETBUFFER <K>
 */
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
    // Update the global BLOCK_COUNT
    BLOCK_COUNT = parsedQuery.setBufferSize;
    
    // As per TA doubt document, no pages are dropped immediately from main memory.
    // The BufferManager will handle the resizing during subsequent reads.
    return;
}