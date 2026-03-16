#pragma once

#include"semanticParser.h"

void executeCommand();

void executeCLEAR();
void executeCROSS();
void executeDISTINCT();
void executeEXPORT();
void executeEXPORTGraph();
void executeINDEX();
void executeJOIN();
void executeLIST();
void executeLOADTable();
void executeLOADGraph();
void executePATH();
void executePRINT();
void executePROJECTION();
void executeRENAME();
void executeSELECTION();
void executeSET_BUFFER();
void executeSORT();
void executeSOURCE();
void executeDEGREE();
void executeGROUP_BY();

bool evaluateBinOp(int value1, int value2, BinaryOperator binaryOperator);
void printRowCount(int rowCount);