#include "global.h"
#include <unordered_map>

bool syntacticParseJOIN()
{
    logger.log("syntacticParseJOIN");

    if (!tokenizedQuery.empty())
    {
        string &lastToken = tokenizedQuery.back();
        if (!lastToken.empty() && lastToken.back() == ';')
        {
            lastToken.pop_back();
            if (lastToken.empty())
                tokenizedQuery.pop_back();
        }
    }

    int onIdx = -1, whereIdx = -1, projectIdx = -1;
    for (int i = 0; i < tokenizedQuery.size(); i++)
    {
        if (tokenizedQuery[i] == "ON")
            onIdx = i;
        if (tokenizedQuery[i] == "WHERE")
            whereIdx = i;
        if (tokenizedQuery[i] == "PROJECT")
            projectIdx = i;
    }

    if (onIdx == -1 || tokenizedQuery.size() < 6)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.queryType = JOIN;
    parsedQuery.joinResultRelationName = tokenizedQuery[0];
    parsedQuery.joinFirstRelationName = tokenizedQuery[3];
    parsedQuery.joinSecondRelationName = tokenizedQuery[4];

    int onEndIdx = tokenizedQuery.size();
    if (whereIdx != -1)
        onEndIdx = whereIdx;
    else if (projectIdx != -1)
        onEndIdx = projectIdx;

    int onLen = onEndIdx - (onIdx + 1);
    auto extractColumn = [](string token)
    {
        size_t dot = token.find('.');
        if (dot != string::npos)
            return token.substr(dot + 1);
        return token;
    };

    if (onLen == 3)
    {
        parsedQuery.joinFirstColumnName = extractColumn(tokenizedQuery[onIdx + 1]);
        parsedQuery.joinSecondColumnName = extractColumn(tokenizedQuery[onIdx + 3]);
        if (tokenizedQuery[onIdx + 2] != "==")
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.joinArithmeticOperator = NO_ARITH_CLAUSE;
    }
    else if (onLen == 5)
    {
        parsedQuery.joinFirstColumnName = extractColumn(tokenizedQuery[onIdx + 1]);
        parsedQuery.joinSecondColumnName = extractColumn(tokenizedQuery[onIdx + 3]);
        string arith = tokenizedQuery[onIdx + 2];
        if (arith == "+")
            parsedQuery.joinArithmeticOperator = ADDITION;
        else if (arith == "-")
            parsedQuery.joinArithmeticOperator = SUBTRACTION;
        else
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }

        if (tokenizedQuery[onIdx + 4] != "==")
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.joinNumber = stoi(tokenizedQuery[onIdx + 5]);
    }
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    if (whereIdx != -1)
    {
        parsedQuery.hasWhereClause = true;
        int whereEndIdx = (projectIdx != -1) ? projectIdx : tokenizedQuery.size();
        if (whereEndIdx - (whereIdx + 1) != 3)
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.whereColumnName = extractColumn(tokenizedQuery[whereIdx + 1]);
        string op = tokenizedQuery[whereIdx + 2];
        if (op == "==")
            parsedQuery.whereOperator = EQUAL;
        else if (op == "!=")
            parsedQuery.whereOperator = NOT_EQUAL;
        else if (op == "<")
            parsedQuery.whereOperator = LESS_THAN;
        else if (op == "<=")
            parsedQuery.whereOperator = LEQ;
        else if (op == ">")
            parsedQuery.whereOperator = GREATER_THAN;
        else if (op == ">=")
            parsedQuery.whereOperator = GEQ;
        else
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.whereNumber = stoi(tokenizedQuery[whereIdx + 3]);
    }

    if (projectIdx != -1)
    {
        parsedQuery.hasProjectClause = true;
        for (int i = projectIdx + 1; i < tokenizedQuery.size(); i++)
        {
            parsedQuery.projectColumnNames.push_back(extractColumn(tokenizedQuery[i]));
        }
        if (parsedQuery.projectColumnNames.empty())
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
    }

    return true;
}

bool semanticParseJOIN()
{
    logger.log("semanticParseJOIN");

    if (tableCatalogue.isTable(parsedQuery.joinResultRelationName))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
        return false;
    }

    if (!tableCatalogue.isTable(parsedQuery.joinFirstRelationName) ||
        !tableCatalogue.isTable(parsedQuery.joinSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }

    if (!tableCatalogue.isColumnFromTable(parsedQuery.joinFirstColumnName, parsedQuery.joinFirstRelationName))
    {
        cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
        return false;
    }

    if (!tableCatalogue.isColumnFromTable(parsedQuery.joinSecondColumnName, parsedQuery.joinSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
        return false;
    }

    if (parsedQuery.hasWhereClause)
    {
        if (!tableCatalogue.isColumnFromTable(parsedQuery.whereColumnName, parsedQuery.joinFirstRelationName) &&
            !tableCatalogue.isColumnFromTable(parsedQuery.whereColumnName, parsedQuery.joinSecondRelationName))
        {
            cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
            return false;
        }
    }

    if (parsedQuery.hasProjectClause)
    {
        for (string col : parsedQuery.projectColumnNames)
        {
            if (!tableCatalogue.isColumnFromTable(col, parsedQuery.joinFirstRelationName) &&
                !tableCatalogue.isColumnFromTable(col, parsedQuery.joinSecondRelationName))
            {
                cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
                return false;
            }
        }
    }

    return true;
}

void executeJOIN()
{
    logger.log("executeJOIN");

    Table *table1 = tableCatalogue.getTable(parsedQuery.joinFirstRelationName);
    Table *table2 = tableCatalogue.getTable(parsedQuery.joinSecondRelationName);

    vector<string> outCols;
    if (parsedQuery.hasProjectClause)
    {
        outCols = parsedQuery.projectColumnNames;
    }
    else
    {
        outCols = table1->columns;
        outCols.insert(outCols.end(), table2->columns.begin(), table2->columns.end());
    }

    Table *resultTable = new Table(parsedQuery.joinResultRelationName, outCols);
    int outPageIndex = 0;
    vector<vector<int>> outBuffer;

    int B = BLOCK_COUNT;
    int col1Idx = table1->getColumnIndex(parsedQuery.joinFirstColumnName);
    int col2Idx = table2->getColumnIndex(parsedQuery.joinSecondColumnName);

    auto getJoinKey = [&](vector<int> &row, int colIdx, bool isTable2)
    {
        if (!isTable2)
            return row[colIdx];
        int val = row[colIdx];
        if (parsedQuery.joinArithmeticOperator == ADDITION)
            return parsedQuery.joinNumber - val;
        if (parsedQuery.joinArithmeticOperator == SUBTRACTION)
            return val + parsedQuery.joinNumber;
        return val;
    };

    // Generalized probing function that handles both full in-memory and chunked fallback
    auto performProbing = [&](Table *leftTable, Table *rightTable, int leftColIdx, int rightColIdx)
    {
        bool leftIsSmaller = leftTable->blockCount <= rightTable->blockCount;
        Table *smallT = leftIsSmaller ? leftTable : rightTable;
        Table *largeT = leftIsSmaller ? rightTable : leftTable;
        int smallColIdx = leftIsSmaller ? leftColIdx : rightColIdx;
        int largeColIdx = leftIsSmaller ? rightColIdx : leftColIdx;

        int maxInMemoryBlocks = max(1, B - 2);

        for (int chunkStart = 0; chunkStart < smallT->blockCount; chunkStart += maxInMemoryBlocks)
        {
            unordered_multimap<int, vector<int>> hashTable;
            int chunkEnd = min((int)smallT->blockCount, chunkStart + maxInMemoryBlocks);

            // Load chunk of smallT into memory
            for (int b = chunkStart; b < chunkEnd; b++)
            {
                Page p = bufferManager.getPage(smallT->tableName, b);
                for (int r = 0; r < smallT->rowsPerBlockCount[b]; r++)
                {
                    vector<int> row = p.getRow(r);
                    int key = getJoinKey(row, smallColIdx, !leftIsSmaller);
                    hashTable.insert({key, row});
                }
            }

            // Stream largeT and probe
            for (int b = 0; b < largeT->blockCount; b++)
            {
                Page p = bufferManager.getPage(largeT->tableName, b);
                for (int r = 0; r < largeT->rowsPerBlockCount[b]; r++)
                {
                    vector<int> row = p.getRow(r);
                    int searchKey = getJoinKey(row, largeColIdx, leftIsSmaller);

                    auto range = hashTable.equal_range(searchKey);
                    for (auto it = range.first; it != range.second; ++it)
                    {
                        vector<int> smallRow = it->second;
                        vector<int> &largeRow = row;

                        vector<int> &leftRow = leftIsSmaller ? smallRow : largeRow;
                        vector<int> &rightRow = leftIsSmaller ? largeRow : smallRow;

                        bool passWhere = true;
                        if (parsedQuery.hasWhereClause)
                        {
                            int whereVal;
                            if (table1->isColumn(parsedQuery.whereColumnName))
                            {
                                whereVal = leftRow[table1->getColumnIndex(parsedQuery.whereColumnName)];
                            }
                            else
                            {
                                whereVal = rightRow[table2->getColumnIndex(parsedQuery.whereColumnName)];
                            }
                            passWhere = evaluateBinOp(whereVal, parsedQuery.whereNumber, parsedQuery.whereOperator);
                        }

                        if (passWhere)
                        {
                            vector<int> outRow;
                            for (const string &col : outCols)
                            {
                                if (table1->isColumn(col))
                                {
                                    outRow.push_back(leftRow[table1->getColumnIndex(col)]);
                                }
                                else
                                {
                                    outRow.push_back(rightRow[table2->getColumnIndex(col)]);
                                }
                            }

                            outBuffer.push_back(outRow);
                            if (outBuffer.size() == resultTable->maxRowsPerBlock)
                            {
                                bufferManager.writePage(resultTable->tableName, outPageIndex, outBuffer, outBuffer.size());
                                resultTable->blockCount++;
                                resultTable->rowsPerBlockCount.push_back(outBuffer.size());
                                resultTable->rowCount += outBuffer.size();
                                outPageIndex++;
                                outBuffer.clear();
                            }
                        }
                    }
                }
            }
        }
    };

    // Check In-Memory Join Optimization (if smaller table fits entirely in memory)
    if (min(table1->blockCount, table2->blockCount) <= max(1, B - 2))
    {
        performProbing(table1, table2, col1Idx, col2Idx);
    }
    else
    {
        // Partitioning Phase
        int M = max(1, B - 1);
        vector<Table *> partitions1(M);
        vector<Table *> partitions2(M);

        for (int i = 0; i < M; i++)
        {
            partitions1[i] = new Table(parsedQuery.joinFirstRelationName + "_part_" + to_string(i), table1->columns);
            partitions2[i] = new Table(parsedQuery.joinSecondRelationName + "_part_" + to_string(i), table2->columns);
            tableCatalogue.insertTable(partitions1[i]);
            tableCatalogue.insertTable(partitions2[i]);
        }

        auto partitionTable = [&](Table *table, int colIdx, vector<Table *> &partitions, bool isTable2)
        {
            vector<vector<vector<int>>> buffers(M);
            int maxRows = table->maxRowsPerBlock;

            for (int b = 0; b < table->blockCount; b++)
            {
                Page p = bufferManager.getPage(table->tableName, b);
                for (int r = 0; r < table->rowsPerBlockCount[b]; r++)
                {
                    vector<int> row = p.getRow(r);
                    int key = getJoinKey(row, colIdx, isTable2);
                    int hash = ((key % M) + M) % M;

                    buffers[hash].push_back(row);
                    if (buffers[hash].size() == maxRows)
                    {
                        bufferManager.writePage(partitions[hash]->tableName, partitions[hash]->blockCount, buffers[hash], maxRows);
                        partitions[hash]->blockCount++;
                        partitions[hash]->rowsPerBlockCount.push_back(maxRows);
                        partitions[hash]->rowCount += maxRows;
                        buffers[hash].clear();
                    }
                }
            }

            for (int h = 0; h < M; h++)
            {
                if (!buffers[h].empty())
                {
                    bufferManager.writePage(partitions[h]->tableName, partitions[h]->blockCount, buffers[h], buffers[h].size());
                    partitions[h]->blockCount++;
                    partitions[h]->rowsPerBlockCount.push_back(buffers[h].size());
                    partitions[h]->rowCount += buffers[h].size();
                    buffers[h].clear();
                }
            }
        };

        partitionTable(table1, col1Idx, partitions1, false);
        partitionTable(table2, col2Idx, partitions2, true);

        // Probing Phase
        for (int i = 0; i < M; i++)
        {
            if (partitions1[i]->rowCount > 0 && partitions2[i]->rowCount > 0)
            {
                performProbing(partitions1[i], partitions2[i], col1Idx, col2Idx);
            }
        }

        // Cleanup partitions
        for (int i = 0; i < M; i++)
        {
            tableCatalogue.deleteTable(partitions1[i]->tableName);
            tableCatalogue.deleteTable(partitions2[i]->tableName);
        }
    }

    // Flush remaining output buffer
    if (!outBuffer.empty())
    {
        bufferManager.writePage(resultTable->tableName, outPageIndex, outBuffer, outBuffer.size());
        resultTable->blockCount++;
        resultTable->rowsPerBlockCount.push_back(outBuffer.size());
        resultTable->rowCount += outBuffer.size();
        outBuffer.clear();
    }

    // Finalize Result Table
    if (resultTable->rowCount > 0)
    {
        tableCatalogue.insertTable(resultTable);
    }
    else
    {
        resultTable->unload();
        delete resultTable;
    }
}