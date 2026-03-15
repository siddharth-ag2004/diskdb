#include "global.h"

// Helper function to parse an aggregate string like "MAX(Salary)"
bool parseAggregate(string token, AggregateExpression& expr) {
    size_t openParen = token.find('(');
    size_t closeParen = token.find(')');
    if (openParen == string::npos || closeParen == string::npos || closeParen < openParen) {
        cout << "SYNTAX ERROR: Invalid aggregate format" << endl; 
        return false;
    }
    string funcStr = token.substr(0, openParen);
    string colStr = token.substr(openParen + 1, closeParen - openParen - 1);
    
    if (funcStr == "MAX") expr.func = MAX;
    else if (funcStr == "MIN") expr.func = MIN;
    else if (funcStr == "COUNT") expr.func = COUNT;
    else if (funcStr == "SUM") expr.func = SUM;
    else if (funcStr == "AVG") expr.func = AVG;
    else { 
        cout << "SYNTAX ERROR: Unknown aggregate function " << funcStr << endl; 
        return false; 
    }
    
    expr.colName = colStr;
    return true;
}

bool syntacticParseGROUP_BY(int arrowIdx) {
    logger.log("syntacticParseGROUP_BY");
    parsedQuery.queryType = GROUP_BY;
    
    // Parse result tables (everything before <-)
    for(int i = 0; i < arrowIdx; i++) {
        parsedQuery.groupByResultTables.push_back(tokenizedQuery[i]);
    }
    
    if (arrowIdx + 2 >= tokenizedQuery.size() || tokenizedQuery[arrowIdx + 2] != "BY") {
        cout << "SYNTAX ERROR: Expected 'BY' after 'GROUP'" << endl; return false;
    }
    
    int fromIdx = -1;
    for(int i = arrowIdx + 3; i < tokenizedQuery.size(); i++) {
        if(tokenizedQuery[i] == "FROM") { fromIdx = i; break; }
    }
    if (fromIdx == -1) { cout << "SYNTAX ERROR: Expected 'FROM' clause" << endl; return false; }
    
    // Parse grouping attributes
    for(int i = arrowIdx + 3; i < fromIdx; i++) {
        parsedQuery.groupByAttributes.push_back(tokenizedQuery[i]);
    }
    
    if (fromIdx + 1 >= tokenizedQuery.size()) { cout << "SYNTAX ERROR: Expected table name" << endl; return false; }
    parsedQuery.groupByRelationName = tokenizedQuery[fromIdx + 1];
    
    if (fromIdx + 2 >= tokenizedQuery.size() || tokenizedQuery[fromIdx + 2] != "HAVING") {
        cout << "SYNTAX ERROR: Expected 'HAVING' clause" << endl; return false;
    }
    
    // Parse HAVING condition
    if (fromIdx + 5 >= tokenizedQuery.size()) { cout << "SYNTAX ERROR: Incomplete HAVING clause" << endl; return false; }
    
    if (!parseAggregate(tokenizedQuery[fromIdx + 3], parsedQuery.havingCondition.left)) return false;
    
    string opStr = tokenizedQuery[fromIdx + 4];
    if (opStr == "<") parsedQuery.havingCondition.op = LESS_THAN;
    else if (opStr == ">") parsedQuery.havingCondition.op = GREATER_THAN;
    else if (opStr == "<=" || opStr == "=<") parsedQuery.havingCondition.op = LEQ;
    else if (opStr == ">=" || opStr == "=>") parsedQuery.havingCondition.op = GEQ;
    else if (opStr == "==") parsedQuery.havingCondition.op = EQUAL;
    else if (opStr == "!=") parsedQuery.havingCondition.op = NOT_EQUAL;
    else { cout << "SYNTAX ERROR: Invalid operator in HAVING" << endl; return false; }
    
    string rightStr = tokenizedQuery[fromIdx + 5];
    regex numeric("[-]?[0-9]+");
    if (regex_match(rightStr, numeric)) {
        parsedQuery.havingCondition.isRightNumber = true;
        parsedQuery.havingCondition.rightNumber = stoi(rightStr);
    } else {
        parsedQuery.havingCondition.isRightNumber = false;
        if (!parseAggregate(rightStr, parsedQuery.havingCondition.right)) return false;
    }
    
    int returnIdx = fromIdx + 6;
    if (returnIdx >= tokenizedQuery.size() || tokenizedQuery[returnIdx] != "RETURN") {
        cout << "SYNTAX ERROR: Expected 'RETURN' clause" << endl; return false;
    }
    
    // Parse RETURN aggregates
    for(int i = returnIdx + 1; i < tokenizedQuery.size(); i++) {
        AggregateExpression expr;
        if (!parseAggregate(tokenizedQuery[i], expr)) return false;
        parsedQuery.returnAggregates.push_back(expr);
    }
    
    return true;
}

bool semanticParseGROUP_BY() {
    logger.log("semanticParseGROUP_BY");
    
    if (!tableCatalogue.isTable(parsedQuery.groupByRelationName)) {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }
    
    Table* table = tableCatalogue.getTable(parsedQuery.groupByRelationName);
    
    for (string attr : parsedQuery.groupByAttributes) {
        cout<<"attr:"<<attr<<endl;
        if (!table->isColumn(attr)) {
            cout << "SEMANTIC ERROR: Group By column doesn't exist in relation" << endl;
            return false;
        }
    }
    
    auto checkAggCol = [&](const AggregateExpression& expr, string errMsg) {
        if (expr.func == COUNT && expr.colName == "*") return true;
        if (!table->isColumn(expr.colName)) {
            cout << errMsg << endl;
            return false;
        }
        return true;
    };
    
    if (!checkAggCol(parsedQuery.havingCondition.left, "SEMANTIC ERROR: Having column doesn't exist in relation")) return false;
    
    if (!parsedQuery.havingCondition.isRightNumber) {
        if (!checkAggCol(parsedQuery.havingCondition.right, "SEMANTIC ERROR: Having column doesn't exist in relation")) return false;
    }
    
    for (auto& expr : parsedQuery.returnAggregates) {
        if (!checkAggCol(expr, "SEMANTIC ERROR: Return column doesn't exist in relation")) return false;
    }
    
    if (parsedQuery.groupByResultTables.size() != parsedQuery.groupByAttributes.size() || 
        parsedQuery.groupByAttributes.size() != parsedQuery.returnAggregates.size()) {
        cout << "SEMANTIC ERROR: Number of result tables, grouping attributes, and return aggregates must match" << endl;
        return false;
    }
    
    for (string resTable : parsedQuery.groupByResultTables) {
        if (tableCatalogue.isTable(resTable)) {
            cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
            return false;
        }
    }
    
    return true;
}

// Struct to maintain the state of an aggregate function across a group
struct AggState {
    long long sum = 0;
    long long count = 0;
    int min_val = INT_MAX;
    int max_val = INT_MIN;
    
    void reset() {
        sum = 0;
        count = 0;
        min_val = INT_MAX;
        max_val = INT_MIN;
    }
    
    void update(int val) {
        sum += val;
        count++;
        min_val = min(min_val, val);
        max_val = max(max_val, val);
    }
    
    int getResult(AggregateFunction func) {
        if (func == SUM) return sum;
        if (func == COUNT) return count;
        if (func == MIN) return (min_val == INT_MAX ? 0 : min_val);
        if (func == MAX) return (max_val == INT_MIN ? 0 : max_val);
        if (func == AVG) return count == 0 ? 0 : sum / count;
        return 0;
    }
};

void executeGROUP_BY() {
    logger.log("executeGROUP_BY");
    Table* table = tableCatalogue.getTable(parsedQuery.groupByRelationName);
    
    // Process each grouping independently
    for (int i = 0; i < parsedQuery.groupByAttributes.size(); i++) {
        string groupAttr = parsedQuery.groupByAttributes[i];
        string resTable = parsedQuery.groupByResultTables[i];
        AggregateExpression retAgg = parsedQuery.returnAggregates[i];
        
        // 1. Create a temporary table sorted by the grouping attribute to group the rows
        string tempName = resTable + "_temp_sort";
        vector<int> sortCols = { table->getColumnIndex(groupAttr) };
        vector<SortingStrategy> sortStrats = { ASC };
        table->externalSortCreateNewTable(tempName, sortCols, sortStrats);
        Table* sortedTable = tableCatalogue.getTable(tempName);
        
        // 2. Prepare the resulting table
        string retAggName;
        if (retAgg.func == COUNT && retAgg.colName == "*") {
            retAggName = "COUNT";
        } else {
            string funcStr[] = {"MAX", "MIN", "COUNT", "SUM", "AVG"};
            retAggName = funcStr[retAgg.func] + retAgg.colName;
        }
        vector<string> resCols = { groupAttr, retAggName };
        Table* resultTable = new Table(resTable, resCols);
        bool hasRows = false;
        
        // 3. Iterate through the sorted temporary table to compute aggregates
        Cursor cursor = sortedTable->getCursor();
        vector<int> row = cursor.getNext();
        
        if (!row.empty()) {
            int groupColIdx = sortedTable->getColumnIndex(groupAttr);
            int havLeftColIdx = parsedQuery.havingCondition.left.colName == "*" ? -1 : sortedTable->getColumnIndex(parsedQuery.havingCondition.left.colName);
            int havRightColIdx = (!parsedQuery.havingCondition.isRightNumber && parsedQuery.havingCondition.right.colName != "*") ? sortedTable->getColumnIndex(parsedQuery.havingCondition.right.colName) : -1;
            int retColIdx = retAgg.colName == "*" ? -1 : sortedTable->getColumnIndex(retAgg.colName);
            
            int currentGroupVal = row[groupColIdx];
            AggState havLeftState, havRightState, retState;
            
            while (!row.empty()) {
                if (row[groupColIdx] != currentGroupVal) {
                    // Group completely read, evaluate HAVING condition
                    int valL = havLeftState.getResult(parsedQuery.havingCondition.left.func);
                    int valR = parsedQuery.havingCondition.isRightNumber ? parsedQuery.havingCondition.rightNumber : havRightState.getResult(parsedQuery.havingCondition.right.func);
                    
                    if (evaluateBinOp(valL, valR, parsedQuery.havingCondition.op)) {
                        resultTable->writeRow<int>({currentGroupVal, retState.getResult(retAgg.func)});
                        hasRows = true;
                    }
                    
                    // Reset stats for the new group
                    havLeftState.reset();
                    havRightState.reset();
                    retState.reset();
                    currentGroupVal = row[groupColIdx];
                }
                
                // Accumulate states for current row
                havLeftState.update(havLeftColIdx == -1 ? 1 : row[havLeftColIdx]);
                if (!parsedQuery.havingCondition.isRightNumber) {
                    havRightState.update(havRightColIdx == -1 ? 1 : row[havRightColIdx]);
                }
                retState.update(retColIdx == -1 ? 1 : row[retColIdx]);
                
                row = cursor.getNext();
            }
            
            // Don't forget to evaluate the very last group read!
            int valL = havLeftState.getResult(parsedQuery.havingCondition.left.func);
            int valR = parsedQuery.havingCondition.isRightNumber ? parsedQuery.havingCondition.rightNumber : havRightState.getResult(parsedQuery.havingCondition.right.func);
            
            if (evaluateBinOp(valL, valR, parsedQuery.havingCondition.op)) {
                resultTable->writeRow<int>({currentGroupVal, retState.getResult(retAgg.func)});
                hasRows = true;
            }
        }
        
        // 4. Save resultant table or discard if empty
        if (hasRows) {
            resultTable->blockify();
            tableCatalogue.insertTable(resultTable);
        } else {
            resultTable->unload();
            delete resultTable;
        }
        
        // 5. Cleanup temporary sorting table
        tableCatalogue.deleteTable(tempName);
    }
}