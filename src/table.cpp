#include "global.h"

/**
 * @brief Construct a new Table:: Table object
 *
 */
Table::Table()
{
    logger.log("Table::Table");
}

/**
 * @brief Construct a new Table:: Table object used in the case where the data
 * file is available and LOAD command has been called. This command should be
 * followed by calling the load function;
 *
 * @param tableName 
 */
Table::Table(string tableName)
{
    logger.log("Table::Table");
    this->sourceFileName = "../data/" + tableName + ".csv";
    this->tableName = tableName;
}

/**
 * @brief Construct a new Table:: Table object used when an assignment command
 * is encountered. To create the table object both the table name and the
 * columns the table holds should be specified.
 *
 * @param tableName 
 * @param columns 
 */
Table::Table(string tableName, vector<string> columns)
{
    logger.log("Table::Table");
    this->sourceFileName = "../data/temp/" + tableName + ".csv";
    this->tableName = tableName;
    this->columns = columns;
    this->columnCount = columns.size();
    this->maxRowsPerBlock = (uint)((BLOCK_SIZE * 1000) / (sizeof(int) * columnCount));
    this->writeRow<string>(columns);
}

/**
 * @brief The load function is used when the LOAD command is encountered. It
 * reads data from the source file, splits it into blocks and updates table
 * statistics.
 *
 * @return true if the table has been successfully loaded 
 * @return false if an error occurred 
 */
bool Table::load()
{
    logger.log("Table::load");
    fstream fin(this->sourceFileName, ios::in);
    string line;
    if (getline(fin, line))
    {
        fin.close();
        if (this->extractColumnNames(line))
            if (this->blockify())
                return true;
    }
    fin.close();
    return false;
}

/**
 * @brief Function extracts column names from the header line of the .csv data
 * file. 
 *
 * @param line 
 * @return true if column names successfully extracted (i.e. no column name
 * repeats)
 * @return false otherwise
 */
bool Table::extractColumnNames(string firstLine)
{
    logger.log("Table::extractColumnNames");
    unordered_set<string> columnNames;
    string word;
    stringstream s(firstLine);
    while (getline(s, word, ','))
    {
        word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
        if (columnNames.count(word))
            return false;
        columnNames.insert(word);
        this->columns.emplace_back(word);
    }
    this->columnCount = this->columns.size();
    this->maxRowsPerBlock = (uint)((BLOCK_SIZE * 1000) / (sizeof(int) * this->columnCount));
    return true;
}

/**
 * @brief This function splits all the rows and stores them in multiple files of
 * one block size. 
 *
 * @return true if successfully blockified
 * @return false otherwise
 */
bool Table::blockify()
{
    logger.log("Table::blockify");
    ifstream fin(this->sourceFileName, ios::in);
    string line, word;
    vector<int> row(this->columnCount, 0);
    vector<vector<int>> rowsInPage(this->maxRowsPerBlock, row);
    int pageCounter = 0;
    unordered_set<int> dummy;
    dummy.clear();
    this->distinctValuesInColumns.assign(this->columnCount, dummy);
    this->distinctValuesPerColumnCount.assign(this->columnCount, 0);
    getline(fin, line);
    while (getline(fin, line))
    {
        stringstream s(line);
        for (int columnCounter = 0; columnCounter < this->columnCount; columnCounter++)
        {
            if (!getline(s, word, ','))
                return false;
            row[columnCounter] = stoi(word);
            rowsInPage[pageCounter][columnCounter] = row[columnCounter];
        }
        pageCounter++;
        this->updateStatistics(row);
        if (pageCounter == this->maxRowsPerBlock)
        {
            bufferManager.writePage(this->tableName, this->blockCount, rowsInPage, pageCounter);
            this->blockCount++;
            this->rowsPerBlockCount.emplace_back(pageCounter);
            pageCounter = 0;
        }
    }
    if (pageCounter)
    {
        bufferManager.writePage(this->tableName, this->blockCount, rowsInPage, pageCounter);
        this->blockCount++;
        this->rowsPerBlockCount.emplace_back(pageCounter);
        pageCounter = 0;
    }

    if (this->rowCount == 0)
        return false;
    this->distinctValuesInColumns.clear();
    return true;
}

/**
 * @brief Given a row of values, this function will update the statistics it
 * stores i.e. it updates the number of rows that are present in the column and
 * the number of distinct values present in each column. These statistics are to
 * be used during optimisation.
 *
 * @param row 
 */
void Table::updateStatistics(vector<int> row)
{
    this->rowCount++;
    for (int columnCounter = 0; columnCounter < this->columnCount; columnCounter++)
    {
        if (!this->distinctValuesInColumns[columnCounter].count(row[columnCounter]))
        {
            this->distinctValuesInColumns[columnCounter].insert(row[columnCounter]);
            this->distinctValuesPerColumnCount[columnCounter]++;
        }
    }
}

/**
 * @brief Checks if the given column is present in this table.
 *
 * @param columnName 
 * @return true 
 * @return false 
 */
bool Table::isColumn(string columnName)
{
    logger.log("Table::isColumn");
    for (auto col : this->columns)
    {
        if (col == columnName)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Renames the column indicated by fromColumnName to toColumnName. It is
 * assumed that checks such as the existence of fromColumnName and the non prior
 * existence of toColumnName are done.
 *
 * @param fromColumnName 
 * @param toColumnName 
 */
void Table::renameColumn(string fromColumnName, string toColumnName)
{
    logger.log("Table::renameColumn");
    for (int columnCounter = 0; columnCounter < this->columnCount; columnCounter++)
    {
        if (columns[columnCounter] == fromColumnName)
        {
            columns[columnCounter] = toColumnName;
            break;
        }
    }
    return;
}

/**
 * @brief Function prints the first few rows of the table. If the table contains
 * more rows than PRINT_COUNT, exactly PRINT_COUNT rows are printed, else all
 * the rows are printed.
 *
 */
void Table::print()
{
    logger.log("Table::print");
    uint count = min((long long)PRINT_COUNT, this->rowCount);

    //print headings
    this->writeRow(this->columns, cout);

    Cursor cursor(this->tableName, 0);
    vector<int> row;
    for (int rowCounter = 0; rowCounter < count; rowCounter++)
    {
        row = cursor.getNext();
        this->writeRow(row, cout);
    }
    printRowCount(this->rowCount);
}



/**
 * @brief This function returns one row of the table using the cursor object. It
 * returns an empty row is all rows have been read.
 *
 * @param cursor 
 * @return vector<int> 
 */
void Table::getNextPage(Cursor *cursor)
{
    logger.log("Table::getNext");

        if (cursor->pageIndex < this->blockCount - 1)
        {
            cursor->nextPage(cursor->pageIndex+1);
        }
}



/**
 * @brief called when EXPORT command is invoked to move source file to "data"
 * folder.
 *
 */
void Table::makePermanent()
{
    logger.log("Table::makePermanent");
    if(!this->isPermanent())
        bufferManager.deleteFile(this->sourceFileName);
    string newSourceFile = "../data/" + this->tableName + ".csv";
    ofstream fout(newSourceFile, ios::out);

    //print headings
    this->writeRow(this->columns, fout);

    Cursor cursor(this->tableName, 0);
    vector<int> row;
    for (int rowCounter = 0; rowCounter < this->rowCount; rowCounter++)
    {
        row = cursor.getNext();
        this->writeRow(row, fout);
    }
    fout.close();
}

/**
 * @brief Function to check if table is already exported
 *
 * @return true if exported
 * @return false otherwise
 */
bool Table::isPermanent()
{
    logger.log("Table::isPermanent");
    if (this->sourceFileName == "../data/" + this->tableName + ".csv")
    return true;
    return false;
}

/**
 * @brief The unload function removes the table from the database by deleting
 * all temporary files created as part of this table
 *
 */
void Table::unload(){
    logger.log("Table::~unload");
    for (int pageCounter = 0; pageCounter < this->blockCount; pageCounter++)
        bufferManager.deleteFile(this->tableName, pageCounter);
    if (!isPermanent())
        bufferManager.deleteFile(this->sourceFileName);
}

/**
 * @brief Function that returns a cursor that reads rows from this table
 * 
 * @return Cursor 
 */
Cursor Table::getCursor()
{
    logger.log("Table::getCursor");
    Cursor cursor(this->tableName, 0);
    return cursor;
}
/**
 * @brief Function that returns the index of column indicated by columnName
 * 
 * @param columnName 
 * @return int 
 */
int Table::getColumnIndex(string columnName)
{
    logger.log("Table::getColumnIndex");
    for (int columnCounter = 0; columnCounter < this->columnCount; columnCounter++)
    {
        if (this->columns[columnCounter] == columnName)
            return columnCounter;
    }
    return -1;
}

void Table::externalSortCreateNewTable(
    const string& resultTableName,
    const vector<int> columnIndices,
    const vector<SortingStrategy> sortOrders,
    int topCount,
    int bottomCount)
{
    logger.log("Table::externalSortCreateNewTable");

    int nB = BLOCK_COUNT;
    int mergeDegree = nB - 1;

    vector<string> runNames;

    // PHASE 1: CREATE INITIAL RUNS

    int currentPage = 0;
    int totalPages = this->blockCount;
    int runCounter = 0;

    while (currentPage < totalPages)
    {
        vector<vector<int>> rows;
        int pagesLoaded = 0;

        while (pagesLoaded < nB && currentPage < totalPages)
        {
            Page page = bufferManager.getPage(this->tableName, currentPage);

            for (int r = 0; r < this->rowsPerBlockCount[currentPage]; r++)
                rows.push_back(page.getRow(r));

            currentPage++;
            pagesLoaded++;
        }

        if (!rows.empty())
            mergeSortRows(rows, 0, rows.size() - 1, columnIndices, sortOrders);

        string runName = resultTableName + "_run_" + to_string(runCounter++);
        Table* runTable = new Table(runName, this->columns);

        int outputPageIndex = 0;
        int rowPtr = 0;

        while (rowPtr < rows.size())
        {
            vector<vector<int>> pageRows;

            for (int i = 0; i < this->maxRowsPerBlock && rowPtr < rows.size(); i++)
                pageRows.push_back(rows[rowPtr++]);

            bufferManager.writePage(runName, outputPageIndex, pageRows, pageRows.size());

            runTable->rowsPerBlockCount.push_back(pageRows.size());
            runTable->blockCount++;
            outputPageIndex++;
        }

        runTable->rowCount = rows.size();
        tableCatalogue.insertTable(runTable);
        runNames.push_back(runName);
    }

    // PHASE 2: MULTI PASS MERGE

    int pass = 0;

    while (runNames.size() > 1)
    {
        bool finalMergePass = (runNames.size() <= mergeDegree);

        vector<string> newRunNames;
        int i = 0;

        while (i < runNames.size())
        {
            int runsToMerge = min(mergeDegree, (int)(runNames.size() - i));

            vector<string> mergeGroup;
            for (int j = 0; j < runsToMerge; j++)
                mergeGroup.push_back(runNames[i + j]);

            string mergedName;

            if (finalMergePass)
                mergedName = resultTableName;
            else
                mergedName = resultTableName +
                             "_pass_" + to_string(pass) +
                             "_run_" + to_string(i);

            Table* mergedTable = new Table(mergedName, this->columns);

            vector<Cursor> cursors;
            for (auto &name : mergeGroup)
                cursors.emplace_back(name, 0);

            priority_queue<
                HeapNode,
                vector<HeapNode>,
                HeapCompare
            > minHeap(HeapCompare(columnIndices, sortOrders));

            // initialize heap
            for (int j = 0; j < cursors.size(); j++)
            {
                vector<int> row = cursors[j].getNext();
                if (!row.empty())
                    minHeap.push({row, j});
            }

            vector<vector<int>> outputRows;
            int outputPageIndex = 0;

            while (!minHeap.empty())
            {
                HeapNode node = minHeap.top();
                minHeap.pop();

                outputRows.push_back(node.row);

                vector<int> nextRow = cursors[node.runIndex].getNext();
                if (!nextRow.empty())
                    minHeap.push({nextRow, node.runIndex});

                if (outputRows.size() == this->maxRowsPerBlock)
                {
                    bufferManager.writePage(
                        mergedName,
                        outputPageIndex,
                        outputRows,
                        outputRows.size()
                    );

                    mergedTable->rowsPerBlockCount.push_back(outputRows.size());
                    mergedTable->blockCount++;

                    outputRows.clear();
                    outputPageIndex++;
                }
            }

            if (!outputRows.empty())
            {
                bufferManager.writePage(
                    mergedName,
                    outputPageIndex,
                    outputRows,
                    outputRows.size()
                );

                mergedTable->rowsPerBlockCount.push_back(outputRows.size());
                mergedTable->blockCount++;
            }

            mergedTable->rowCount = 0;
            for (int x : mergedTable->rowsPerBlockCount)
                mergedTable->rowCount += x;

            tableCatalogue.insertTable(mergedTable);

            if (!finalMergePass)
                newRunNames.push_back(mergedName);

            i += runsToMerge;
        }

        for (auto &name : runNames)
        {
            if (tableCatalogue.isTable(name))
                tableCatalogue.deleteTable(name);
        }

        if (finalMergePass)
            return;

        runNames = newRunNames;
        pass++;
    }
}