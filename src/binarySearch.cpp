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

    long long low = 0, high = table->rowCount - 1;

    while (low <= high) {
        long long mid = low + (high - low) / 2;
        vector<int> row = getRow(tableName, mid);

        if (row.empty()) {
            high = mid - 1;
            continue;
        }

        if (row[columnIndex] >= value) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    if (low > table->rowCount) {
        return {-1, -1};
    }

    // Convert global row ID → page, offset
    int pageIndex = low / table->maxRowsPerBlock;
    int rowIndexInPage = low % table->maxRowsPerBlock;

    // Case: position is exactly after last row
    if (low == table->rowCount) {
        int lastPage = table->blockCount - 1;
        int lastRowCount = table->rowsPerBlockCount[lastPage];

        return {lastPage, lastRowCount};
    }

    return {pageIndex, rowIndexInPage};
}
