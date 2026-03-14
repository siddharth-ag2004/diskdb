#pragma once

#include "tableCatalogue.h"
#include "graphCatalogue.h"
#include "sortingStrategy.h"

using namespace std;

enum QueryType
{
    CLEAR,
    CROSS,
    DISTINCT,
    EXPORT,
    EXPORT_GRAPH,
    INDEX,
    JOIN,
    LIST,
    LOAD,
    PRINT,
    PROJECTION,
    RENAME,
    SELECTION,
    SORT,
    SOURCE,
    LOAD_GRAPH,
    PATH,
    DEGREE,
    UNDETERMINED,
};

enum BinaryOperator
{
    LESS_THAN,
    GREATER_THAN,
    LEQ,
    GEQ,
    EQUAL,
    NOT_EQUAL,
    NO_BINOP_CLAUSE
};

enum ArithmeticOperator
{
    ADDITION,
    SUBTRACTION,
    NO_ARITH_CLAUSE
};


enum SelectType
{
    COLUMN,
    INT_LITERAL,
    NO_SELECT_CLAUSE
};

enum EntityType
{
    TABLE,
    GRAPH,
    UNDEFINED_ENTITY
};



class ParsedQuery
{

public:
    
    QueryType queryType = UNDETERMINED;
    
    EntityType entityType = UNDEFINED_ENTITY;

    string clearRelationName = "";

    string crossResultRelationName = "";
    string crossFirstRelationName = "";
    string crossSecondRelationName = "";

    string distinctResultRelationName = "";
    string distinctRelationName = "";

    string exportRelationName = "";

    IndexingStrategy indexingStrategy = NOTHING;
    string indexColumnName = "";
    string indexRelationName = "";

    BinaryOperator joinBinaryOperator = NO_BINOP_CLAUSE;
    string joinResultRelationName = "";
    string joinFirstRelationName = "";
    string joinSecondRelationName = "";
    string joinFirstColumnName = "";
    string joinSecondColumnName = "";

    ArithmeticOperator joinArithmeticOperator = NO_ARITH_CLAUSE;
    int joinNumber = 0;
    bool hasWhereClause = false;
    string whereColumnName = "";
    BinaryOperator whereOperator = NO_BINOP_CLAUSE;
    int whereNumber = 0;
    bool hasProjectClause = false;
    vector<string> projectColumnNames;

    string loadRelationName = "";

    string printRelationName = "";

    string projectionResultRelationName = "";
    vector<string> projectionColumnList;
    string projectionRelationName = "";

    string renameFromColumnName = "";
    string renameToColumnName = "";
    string renameRelationName = "";

    SelectType selectType = NO_SELECT_CLAUSE;
    BinaryOperator selectionBinaryOperator = NO_BINOP_CLAUSE;
    string selectionResultRelationName = "";
    string selectionRelationName = "";
    string selectionFirstColumnName = "";
    string selectionSecondColumnName = "";
    int selectionIntLiteral = 0;

    string sortRelationName = "";
    vector<string> sortColumnNames;
    vector<SortingStrategy> sortStrategies;

    string sourceFileName = "";

    // For GRAPH
    string loadGraphRelationName = "";
    GraphType graphType;

    // For PATH command of GRAPH
    string pathResultGraphName = "";
    string pathGraphName = "";
    int pathSrcNodeId = -1;
    int pathDestNodeId = -1;
    vector<PathCondition> pathConditions;

    // For DEGREE command of GRAPH
    string degreeGraphName = "";
    int degreeNodeId = -1;

    ParsedQuery();
    void clear();
};

bool syntacticParse();
bool syntacticParseCLEAR();
bool syntacticParseCROSS();
bool syntacticParseDISTINCT();
bool syntacticParseEXPORT();
bool syntacticParseINDEX();
bool syntacticParseJOIN();
bool syntacticParseLIST();
bool syntacticParseLOAD();
bool syntacticParsePATH();
bool syntacticParsePRINT();
bool syntacticParsePROJECTION();
bool syntacticParseRENAME();
bool syntacticParseSELECTION();
bool syntacticParseSORT();
bool syntacticParseSOURCE();
bool syntacticParseDEGREE();

bool isFileExists(string tableName);
bool isQueryFile(string fileName);
