/*
 *   I pledge my
 *   Honor that I have not cheated, 
 *   and will not cheat, on this assignment.
 *   Raymond Gillies
 */

#include <stdarg.h>
#include <string.h>

#include "codegen.h"
#include "ast.h"
#include "symtbl.h"

#define MAX_DISM_ADDR 65535
/* Global for the DISM output file */
FILE *fout;
/* Global to remember the next unique label number to use */
unsigned int labelNumber = 0;
unsigned int SP = 0;
unsigned int FP = 0;
unsigned int HP = 0;

/* Declare mutually recursive functions (defs and docs appear below) */
void codeGenExpr(ASTree *t, int classNumber, int methodNumber);
void codeGenExprs(ASTree *expList, int classNumber, int methodNumber);

void writeLine(char* line, ...)
{
    va_list lst;
    va_start(lst, line);

    if(vfprintf(fout, line, lst) < 0 || fprintf(fout, "\n") < 0)
    {
        printf("Compiler error: failed to write to target file");
        exit(-1);
    }
}

void printAndExit(char* msg, ...)
{
    va_list lst;
    va_start(lst, msg);
    vprintf(msg, lst);
    exit(-1);
}

/* Using the global classesST, calculate the total number of fields,
 including inherited fields, in an object of the given type */
int getNumObjectFields(int type)
{
    int ret = 0;
    ret += classesST[type].numVars;
    
    while(classesST[type].superclass > 0)
    {
        type = classesST[type].superclass;
        ret += classesST[type].numVars;
    }
    return ret;
}
/* Generate code that increments the stack pointer */
void incSP()
{
    int jumpVal = labelNumber++;
    writeLine("    mov 1 1  ;  SP++");
    writeLine("    add 6 6 1");
    writeLine("    bgt 6 5 #%d", jumpVal);
    writeLine("    mov 1 77  ;  error code 77 => out of stack memory");
    writeLine("    hlt 1  ;  out of stack memory! (HP >= SP)");
    writeLine("#%d: mov 0 0  ;  SP++ success", jumpVal);
    SP += 1;
}
/* Generate code that decrements the stack pointer */
void decSP()
{
    int jumpVal = labelNumber++;
    writeLine("    mov 1 1  ;  SP--");
    writeLine("    sub 6 6 1");
    writeLine("    bgt 6 5 #%d", jumpVal);
    writeLine("    mov 1 77  ;  error code 77 => out of stack memory");
    writeLine("    hlt 1  ;  out of stack memory! (HP >= SP)");
    writeLine("#%d: mov 0 0  ;  SP-- success", jumpVal);
    SP -= 1;
}
/* Output code to check for a null value at the top of the stack.
 If the top stack value (at M[SP+1]) is null (0), the DISM code
 output will halt. */
void checkNullDereference()
{
    int jumpVal = labelNumber++;
    writeLine("    lod 1 6 0");
    writeLine("    bgt 1 0 #%s", jumpVal);
    writeLine("    mov 1 99  ;  error code 99 => null dereference");
    writeLine("    hlt 1  ;  null dereference! (M(SP + 1) == 0)");
    writeLine("#%d: mov 0 0  ;  dereference success", jumpVal);
}
/* Generate DISM code for the given single expression, which appears
 in the given class and method (or main block).
 If classNumber < 0 then methodNumber may be anything and we assume
 we are generating code for the program's main block. */
void codeGenExpr(ASTree *t, int classNumber, int methodNumber)
{
    if(t == NULL)
    {
        printAndExit("Error in code gen: null expression node");
    }

    else if(t->typ == DOT_METHOD_CALL_EXPR)
    {
        writeLine("    mov 0 0  ;  DOT METHOD CALL");
        writeLine("    mov 0 0  ;  END DOT METHOD CALL");
    }
    else if(t->typ == METHOD_CALL_EXPR)
    {
        int retLabel = labelNumber++;
        writeLine("    mov 0 0  ;  METHOD CALL");
        writeLine("    mov 1 %d", retLabel);
        writeLine("    str 6 0 1  ;  push return label to stack");
        incSP();
        writeLine("    mov 1 %d", classNumber);
        writeLine("    str 6 0 1  ;  push class to stack");
        checkNullDereference();
        writeLine("    mov 1 %d", classNumber);
        writeLine("    str 6 0 1  ;  push class to stack");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    mov 0 0  ;  END METHOD CALL");
    }
    else if(t->typ == DOT_ID_EXPR)
    {
        writeLine("    mov 0 0  ;  DOT ID");
        writeLine("    mov 0 0  ;  END DOT ID");
    }
    else if(t->typ == ID_EXPR)
    {
        writeLine("    mov 0 0  ;  ID(%s)", t->children->data->idVal);
        // if we are accessing a non-dot id from the main, then we are in the main frame
        // (i.e. it is on the stack)
        if(classNumber < 0)
        {
            int i = 0;
            for(i = 0; i < numMainBlockLocals; i++)
            {
                if(strcmp(mainBlockST[i].varName, t->children->data->idVal) == 0)
                {
                    if(mainBlockST[i].type >= 0)
                    {
                        // Address is FP - i
                        writeLine("    mov 1 %d  ;  i", i);
                        writeLine("    sub 1 7 1  ;  FP - i");
                        writeLine("    str 6 0 1  ;  write address to stack");
                    }
                    else if(mainBlockST[i].type == -1)
                    {
                        // Address is FP - i
                        writeLine("    mov 1 %d  ;  i", i);
                        writeLine("    sub 1 7 1  ;  FP - i");
                        writeLine("    lod 1 1 0  ;  R[1] = M[R[1]]");
                        writeLine("    str 6 0 1  ;  write value to stack");
                    }
                }
            }
        }
        writeLine("    mov 0 0  ;  END ID(%s)", t->children->data->idVal);
    }
    else if(t->typ == DOT_ASSIGN_EXPR)
    {
        writeLine("    mov 0 0  ;  DOT ASSIGN EXPR");
        writeLine("    mov 0 0  ;  END DOT ASSIGN EXPR");
    }
    else if(t->typ == ASSIGN_EXPR)
    {
        // non-dot assign = var is local or param, should be on stack
        writeLine("    mov 0 0  ;  ASSIGN EXPR (%s)", t->children->data->idVal);
        codeGenExpr(t->childrenTail->data, classNumber, methodNumber);
        if(classNumber < 0)
        {
            int i = 0;
            for(i = 0; i < numMainBlockLocals; i++)
            {
                if(strcmp(mainBlockST[i].varName, t->children->data->idVal) == 0)
                {
                    // Address is FP - i
                    writeLine("    mov 1 %d  ;  i", i);
                    writeLine("    sub 1 7 1  ;  FP - i");
                    writeLine("    add 3 0 1  ;  write address to R[3]");
                }
            }
        }
        writeLine("    lod 2 6 0  ;  load second value into R[2]");
        writeLine("    str 3 0 2  ;  %s = R[2]", t->children->data->idVal);
        writeLine("    str 6 0 2  ;  store value on stack");
        writeLine("    mov 0 0  ;  END ASSIGN EXPR (%s)", t->children->data->idVal);
    }
    else if(t->typ == PLUS_EXPR)
    {
        writeLine("    mov 0 0  ;  PLUS EXPR");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 3 6 0  ;  load first value into R[3]");
        codeGenExpr(t->childrenTail->data, classNumber, methodNumber);
        writeLine("    lod 2 6 0  ;  load second value into R[2]");
        writeLine("    add 1 3 2");
        writeLine("    str 6 0 1  ;  write result to stack");
        writeLine("    mov 0 0  ;  END PLUS EXPR");
    }
    else if(t->typ == MINUS_EXPR)
    {
        writeLine("    mov 0 0  ;  MINUS EXPR");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 3 6 0  ;  load first value into R[3]");
        codeGenExpr(t->childrenTail->data, classNumber, methodNumber);
        writeLine("    lod 2 6 0  ;  load second value into R[2]");
        writeLine("    sub 1 3 2");
        writeLine("    str 6 0 1  ;  write result to stack");
        writeLine("    mov 0 0  ;  END MINUS EXPR");
    }
    else if(t->typ == TIMES_EXPR)
    {
        writeLine("    mov 0 0  ;  TIMES EXPR");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 3 6 0  ;  load first value into R[3]");
        codeGenExpr(t->childrenTail->data, classNumber, methodNumber);
        writeLine("    lod 2 6 0  ;  load second value into R[2]");
        writeLine("    mul 1 3 2");
        writeLine("    str 6 0 1  ;  write result to stack");
        writeLine("    mov 0 0  ;  END TIMES EXPR");
    }
    else if(t->typ == EQUALITY_EXPR)
    {
        int jump = labelNumber++;
        writeLine("    mov 0 0  ;  EQUALITY EXPR");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 3 6 0  ;  load first value into R[3]");
        codeGenExpr(t->childrenTail->data, classNumber, methodNumber);
        writeLine("    lod 2 6 0  ;  load second value into R[2]");
        writeLine("    mov 1 1");
        writeLine("    beq 2 3 #%d", jump);
        writeLine("    mov 1 0");
        writeLine("#%d: str 6 0 1  ;  write result to stack", jump);
         writeLine("    mov 0 0  ;  END EQUALITY EXPR");
    }
    else if(t->typ == GREATER_THAN_EXPR)
    {
        int jump = labelNumber++;
        writeLine("    mov 0 0  ;  GREATER THAN EXPR");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 3 6 0  ;  load first value into R[3]");
        codeGenExpr(t->childrenTail->data, classNumber, methodNumber);
        writeLine("    lod 2 6 0  ;  load second value into R[2]");
        writeLine("    mov 1 1");
        writeLine("    bgt 3 2 #%d", jump);
        writeLine("    mov 1 0");
        writeLine("#%d: str 6 0 1  ;  write result to stack", jump);
        writeLine("    mov 0 0  ;  END THAN EXPR");
    }
    else if(t->typ == NOT_EXPR)
    {
        int tru = labelNumber++;
        int end = labelNumber++;
        writeLine("    mov 0 0  ;  NOT EXPR");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 1 6 0  ;  load passed value into R[1]");
        writeLine("    bgt 1 0 #%d", tru);
        writeLine("    mov 1 1");
        writeLine("    str 6 0 1");
        writeLine("    jmp 0 #%d", end);
        writeLine("#%d: mov 1 0", tru);
        writeLine("    str 6 0 1");
        writeLine("#%d: mov 0 0", end);
        writeLine("    mov 0 0  ;  END NOT EXPR");
    }
    else if(t->typ == AND_EXPR)
    {
        int jump = labelNumber++;
        writeLine("    mov 0 0  ;  AND EXPR");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 4 6 0  ;  load first value into R[3]");
        codeGenExpr(t->childrenTail->data, classNumber, methodNumber);
        writeLine("    lod 2 6 0  ;  load second value into R[2]");
        writeLine("    mov 1 0");
        writeLine("    beq 4 0 #%d", jump);
        writeLine("    beq 2 0 #%d", jump);
        writeLine("    mov 1 1");
        writeLine("#%d: str 6 0 1", jump);
        writeLine("    mov 0 0  ;  END AND EXPR");
    }
    else if(t->typ == IF_THEN_ELSE_EXPR)
    {
        int elselabel = labelNumber++;
        int endLabel = labelNumber++;

        writeLine("    mov 0 0  ;  IF-ELSE-THEN");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 3 6 0  ;  load test result into R[1]");
        writeLine("    beq 0 3 #%d", elselabel);
        codeGenExprs(t->children->next->data, classNumber, methodNumber);
        writeLine("    jmp 0 #%d  ;  skip else branch", endLabel);
        writeLine("#%d: mov 0 0  ;  begin else branch", elselabel);
        codeGenExprs(t->childrenTail->data, classNumber, methodNumber);
        writeLine("#%d: mov 0 0  ;  end if-else", endLabel);
        writeLine("    mov 0 0  ;  END IF-ELSE-THEN");
    }
    else if(t->typ == WHILE_EXPR)
    {
        int loop = labelNumber++;
        int end = labelNumber++;

        writeLine("#%d: mov 0 0  ;  WHILE", loop);
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 1 6 0  ;  load test result into R[1]");
        writeLine("    beq 0 1 #%d", end);
        codeGenExprs(t->childrenTail->data, classNumber, methodNumber);
        writeLine("    jmp 0 #%d", loop);
        writeLine("#%d: mov 0 0  ;  END WHILE", end);
    }
    else if(t->typ == PRINT_EXPR)
    {
        writeLine("    mov 0 0  ;  PRINT NAT");
        codeGenExpr(t->children->data, classNumber, methodNumber);
        writeLine("    lod 1 6 0  ;  load passed value into R[1]");
        writeLine("    ptn 1");
        writeLine("    mov 0 0  ;  END PRINT NAT");
    }
    else if(t->typ == READ_EXPR)
    {
        writeLine("    rdn 1  ;  READ NAT");
        writeLine("    str 6 0 1  ;  save read value into stack");
         writeLine("    mov 0 0  ;  END READ NAT");
    }
    else if(t->typ == THIS_EXPR)
    {
        writeLine("    mov 0 0  ;  THIS EXPR");
        writeLine("    mov 1 %d  ;  value of this class", classNumber);
        writeLine("    str 6 0 1");
        writeLine("    mov 0 0  ;  END THIS EXPR");
    }
    else if(t->typ == NEW_EXPR)
    {
        int jump = labelNumber++;
        int jump2 = labelNumber++;
        int n = getNumObjectFields(t->staticClassNum);
        writeLine("    mov 1 1  ;  NEW EXPR");
        writeLine("    add 2 5 1  ;  R[2] = HP + 1");
        writeLine("    mov 1 %d  ;", n);
        writeLine("    add 2 5 1  ;  R[2] = HP + n + 1");
        writeLine("    mov 1 %d", MAX_DISM_ADDR);
        writeLine("    bgt 1 2 #%d ;  HP + n + 1 < MAX_DISM_ADDRESS", jump);
        writeLine("    mov 1 66  ;  error code 66 => insufficient heap space");
        writeLine("    hlt 1  ;  insufficient heap space!");
        writeLine("#%d: mov 0 0  ;  sufficient heap space", jump);
        writeLine("    mov 1 1");
        writeLine("    mov 2 0");
        writeLine("    mov 3 %d", n);
        writeLine("#%d: str 5 0 0  ;  allocate a field", jump2);
        writeLine("    add 5 5 1  ;  HP++");
        writeLine("    add 2 2 1");
        writeLine("    bgt 3 2 #%d  ;  end of loop", jump2);
        writeLine("    mov 2 %d  ;  R[2] stores type", t->staticClassNum);
        writeLine("    str 5 0 2");
        writeLine("    str 6 0 5");
        writeLine("    add 5 5 1");
        decSP();
        writeLine("    mov 0 0  ;  NEW EXPR");
    }
    else if(t->typ == NULL_EXPR)
    {
        writeLine("    str 6 0 0  ;  NULL EXPR");
    }
    else if(t->typ == NAT_LITERAL_EXPR)
    {
        writeLine("    mov 1 %d  ;  NAT LITERAL EXPR", t->natVal);
        writeLine("    str 6 0 1  ;  Write value to stack");
    }
    else if(t->typ == ARG_LIST)
    {

    }
}
/* Generate DISM code for an expression list, which appears in
 the given class and method (or main block).
 If classNumber < 0 then methodNumber may be anything and we assume
 we are generating code for the program's main block. */
void codeGenExprs(ASTree *expList, int classNumber, int methodNumber)
{
    ASTList* it = expList->children;
    while(it && it->data)
    {
        codeGenExpr(it->data, classNumber, methodNumber);
        it = it->next;
    }
}
/* Generate DISM code as the prologue to the given method or main
 block. If classNumber < 0 then methodNumber may be anything and we
 assume we are generating code for the program's main block. */
void genPrologue(int classNumber, int methodNumber)
{
    // Main
    if(classNumber < 0)
    {
        writeLine("    mov 7 %d  ;  initialize FP", MAX_DISM_ADDR);
        writeLine("    mov 6 %d  ;  initialize SP", MAX_DISM_ADDR);
        writeLine("    mov 5 1  ;  initialize HP");
    }
}
/* Generate DISM code as the epilogue to the given method or main
 block. If classNumber < 0 then methodNumber may be anything and we
 assume we are generating code for the program's main block. */
void genEpilogue(int classNumber, int methodNumber)
{
    if(classNumber < 0)
    {
        writeLine("#%d: mov 0 0", labelNumber++);
        writeLine("    hlt 0  ;  NORMAL TERMINATION AT END OF MAIN BLOCK");
    }
}
/* Generate DISM code for the given method or main block.
 If classNumber < 0 then methodNumber may be anything and we assume
 we are generating code for the program's main block. */
void genBody(int classNumber, int methodNumber)
{
    if(classNumber < 0)
    {
        writeLine("    mov 0 0  ;  WRITE MAIN LOCALS");
        int i = 0;
        for(i = 0; i < numMainBlockLocals; i++)
        {
            if(mainBlockST[i].type == -1)
            {
                writeLine("    str 6 0 0  ;  NAT %s", mainBlockST[i].varName);
                decSP();
            }
            else
            {

            }
        }
        codeGenExprs(mainExprs, -1, 0);
    }
    else
    {
        codeGenExprs(classesST[classNumber].methodList[methodNumber].bodyExprs,
        classNumber, methodNumber);
    }
}
/* Map a given (1) static class number, (2) a method number defined
 in that class, and (3) a dynamic object's type to:
 (a) the dynamic class number and (b) the dynamic method number that
 actually get called when an object of type (3) dynamically invokes
 method (2).
 This method assumes that dynamicType is a subtype of staticClass. */
void getDynamicMethodInfo(int staticClass, int staticMethod,
 int dynamicType, int *dynamicClassToCall, int *dynamicMethodToCall)
{

}
/* Emit code for the program's vtable, beginning at label #VTABLE.
 The vtable jumps (i.e., dispatches) to code based on
 (1) the number of arguments on the stack (at M[SP+1]),
 (2) the dynamic calling object's address (at M[SP+4+numArguments]),
 (3) the calling object's static type (at M[SP+3+numArguments]), and
 (4) the static method number (at M[SP+2+numArguments]). */
void genVTable()
{
    writeLine("#VTABLE: mov 0 0");
    
}

void generateDISM(FILE *outputFile)
{
    fout = outputFile;
    FP = MAX_DISM_ADDR;
    SP = MAX_DISM_ADDR;
    HP = 0;
    // Generate main prologue
    genPrologue(-1, 0);
    // Code gen main block
    genBody(-1, 0);
    // Gen main epilogue
    genEpilogue(-1, 0);
    // Method code gen

    writeLine("    mov 0 0  ;  METHODS");
    int i = 0;
    int totalMethods = 0;
    for(i = 0; i < numClasses; i++)
    {
        int j = 0;
        for(j = 0; j < classesST[i].numMethods; j++)
        {
            writeLine("#%d: mov 0 0  ;  %s.%s", 
            labelNumber++,
            classesST[i].className, 
            classesST[i].methodList[j].methodName);
            genPrologue(i, j);
            genBody(i, j);
            genEpilogue(i, j);
        }
    }
    // Gen Vtable
    genVTable();
}

