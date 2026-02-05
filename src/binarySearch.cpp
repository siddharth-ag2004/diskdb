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
vector<int> getTableRow(string tableName, int rowId) {
    logger.log("binarySearch::getRow");
    Table* table = tableCatalogue.getTable(tableName);
    if (table == nullptr || rowId < 0 || rowId >= table->rowCount) {
        return {};
    }
    int pageIndex = rowId / table->maxRowsPerBlock;
    int rowIndexInPage = rowId % table->maxRowsPerBlock;
    
    // Check if the calculated pageIndex is valid
    if (pageIndex >= table->blockCount) {
        return {};
    }

    Page page = bufferManager.getPage(tableName, pageIndex);
    
    // Check if the rowIndexInPage is valid for the given page
    if (rowIndexInPage >= table->rowCount) {
        return {};
    }

    return page.getRow(rowIndexInPage);
}

/**
 * @brief Finds the first occurrence of a value in a sorted column of a table.
 *
 * @param tableName The name of the table to search in.
 * @param columnName The name of the sorted column to search.
 * @param value The value to search for.
 * @return pair<int, int> A pair containing the page index and row index in page. 
 *                        Returns {-1, -1} if the value is not found.
 */
pair<int, int> findFirstOccurrence(string tableName, string columnName, int value) {
    logger.log("binarySearch::findFirstOccurrence");
    Table* table = tableCatalogue.getTable(tableName);
    if (table == nullptr) {
        cout << "Table not found: " << tableName << endl;
        return {-1, -1};
    }

    int columnIndex = table->getColumnIndex(columnName);
    if (columnIndex == -1) {
        cout << "Column not found: " << columnName << endl;
        return {-1, -1};
    }

    long long int low = 0, high = table->rowCount - 1;
    long long int resultRowId = -1;

    while (low <= high) {
        long long int mid = low + (high - low) / 2;
        vector<int> row = getTableRow(tableName, mid);

        if (row.empty()) {
            // This might happen if the last page has fewer rows than maxRowsPerBlock.
            // If mid is out of bounds, getRow returns empty.
            // Let's try to search in the lower half.
            high = mid - 1;
            continue;
        }

        if (row[columnIndex] >= value) {
            if (row[columnIndex] == value) {
                resultRowId = mid;
            }
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    if (resultRowId != -1) {
        int pageIndex = resultRowId / table->maxRowsPerBlock;
        int rowIndexInPage = resultRowId % table->maxRowsPerBlock;
        return {pageIndex, rowIndexInPage};
    }

    return {-1, -1};
}
