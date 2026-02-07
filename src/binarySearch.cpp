#include "binarySearch.h"
#include "global.h"
#include "logger.h"
#include "tableCatalogue.h"
#include "bufferManager.h"
#include "page.h"

/**
 * @brief Gets a specific row from a table by its global row ID.
 *
 * @param tableName The name of the table.
 * @param rowId The global ID of the row to retrieve.
 * @return vector<int> The row data. Returns an empty vector if not found.
 */
vector<int> getRow(string tableName, int rowId) {
    logger.log("binarySearch::getRow");
    Table* table = tableCatalogue.getTable(tableName);
    if (table == nullptr || rowId < 0 || rowId >= table->rowCount) {
        return {};
    }
    int pageIndex = rowId / table->maxRowsPerBlock;
    int rowIndexInPage = rowId % table->maxRowsPerBlock;
    
    if (pageIndex >= table->blockCount) {
        return {};
    }

    // Use table->rowsPerBlockCount to check the number of rows
    if (rowIndexInPage >= table->rowsPerBlockCount[pageIndex]) {
        return {};
    }

    Page page = bufferManager.getPage(tableName, pageIndex);
    
    return page.getRow(rowIndexInPage);
}

/**
 * @brief Finds the first occurrence of a value in a sorted column of a table,
 *        or the first occurrence of the next higher value if the value is not found.
 *
 * @param tableName The name of the table to search in.
 * @param columnName The name of the sorted column to search.
 * @param value The value to search for.
 * @return pair<int, int> A pair containing the page index and row index in page. 
 *                        Returns {-1, -1} if no value >= the search value is found.
 */
pair<int, int> findFirstOccurrence(string tableName, string columnName, int value)
{
    logger.log("binarySearch::findFirstOccurrence");

    Table* table = tableCatalogue.getTable(tableName);
    if (table == nullptr) {
        return {-1, -1};
    }

    int columnIndex = table->getColumnIndex(columnName);
    if (columnIndex == -1) {
        return {-1, -1};
    }

    // binary search on pages
    int lowPage = 0;
    int highPage = table->blockCount - 1;
    int targetPage = -1;

    while (lowPage <= highPage) {
        int midPage = lowPage + (highPage - lowPage) / 2;

        if (table->rowsPerBlockCount[midPage] == 0) {
            lowPage = midPage + 1;
            continue;
        }

        Page page = bufferManager.getPage(tableName, midPage);
        int lastRowIndex = table->rowsPerBlockCount[midPage] - 1;
        vector<int> lastRow = page.getRow(lastRowIndex);

        if (lastRow[columnIndex] >= value) {
            targetPage = midPage;
            highPage = midPage - 1;
        } else {
            lowPage = midPage + 1;
        }
    }

    // targetPage = -1 means value is greater than all values in the table
    if (targetPage == -1) {
        if (table->blockCount == 0) return {-1, -1}; 
        int lastPage = table->blockCount - 1;
        int lastPageRowCount = table->rowsPerBlockCount[lastPage];
        return {lastPage, lastPageRowCount};
    }

    // bin Search in identified page
    Page page = bufferManager.getPage(tableName, targetPage);
    int rowCount = table->rowsPerBlockCount[targetPage];
    int low = 0, high = rowCount - 1;
    int ansRow = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        vector<int> row = page.getRow(mid);
        if (row[columnIndex] >= value) {
            ansRow = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    if (ansRow != -1) {
        return {targetPage, ansRow};
    }

    return {targetPage, rowCount};
}
