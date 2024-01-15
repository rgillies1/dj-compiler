/* I pledge my Honor that I have not cheated, and will not cheat, on this assignment. */
/* Raymond Gillies */

#include "typecheck.h"
#include <stdio.h>
#include <string.h>


int join(int t1, int t2)
{
    if(t1 == 0) return t1;
    if(t2 == 0) return t2;
    if(isSubtype(t1, t2)) return t2;
    if(isSubtype(t2, t1)) return t1;
    return join(classesST[t1].superclass, t2);
}

// Checks if the passed ID is in the passed class/method
// If found, returns the type
int checkForID(char* id, int class, int method)
{
    int i;

    if(id == NULL)
    {
        printf("Type error: null identifier\n");
        exit(-1);
    }
    // Check for class name
    for(i = 0; i < numClasses; i++)
    {
        if(strcmp(id, classesST[i].className) == 0)
        {
            return typeNameToNumber(classesST[i].className);
        } 
    }
    // In main
    if(class < 0)
    {
        for(i = 0; i < numMainBlockLocals; i++)
        {
            if(!strcmp(id, mainBlockST[i].varName)) 
            {
                /* This return causes a segfault during a lot of runs,
                 * but not always (i.e. some returns from here are fine).
                 * valgrind says it's accessing memory out of bounds,
                 * but since this is tested using symtbl.o, the implementation
                 * shouldn't be invald. I've observed that it will correctly
                 * return when type checking a variable, only to segfault
                 * on the next check, and since the stack depth is far too small
                 * to crash, I think I'm somehow writing into mainBlockST. I'm not
                 * sure how this actually happens, however, because I never make
                 * any explicit writes to it.
                 */
                return mainBlockST[i].type;
            }
                
        }
        return -1;
    }
    // In a class
    for(i = 0; i < classesST[class].numVars; i++)
    {
        if(!strcmp(id, classesST[class].varList[i].varName)) 
        {
            return classesST[class].varList[i].type;
        }

    }
    // In a method definition
    if(method != -1)
    {
        // Check method locals
        for(i = 0; i < classesST[class].methodList[method].numLocals; i++)
        {
            if(!strcmp(id, classesST[class].methodList[method].localST[i].varName))
            {
                return classesST[class].methodList[method].localST[i].type;
            }
        }
        // Check param locals
        for(i = 0; i < classesST[class].methodList[method].numParams; i++)
        {
            if(!strcmp(id, classesST[class].methodList[method].paramST[i].varName))
            {
                return classesST[class].methodList[method].paramST[i].type;
            }
        }
    }
    return -1;
}

// Checks if a method with the given name exists in the passed class
// If so, returns the return type if that method
// Otherwise, returns -5
int checkForMethod(char* methodName, int class)
{
    if(methodName == NULL || class < 1 || class >= numClasses)
    {
        printf("Type error: invalid method name or class\n");
        exit(-1);
    }
    int i;
    for(i = 0; i < classesST[class].numMethods; i++)
    {
        if(strcmp(methodName, classesST[class].methodList[i].methodName) == 0)
            return classesST[class].methodList[i].returnType;
    }
    return -5;
}

void typecheckProgram()
{
    int i;
    int j;
    int k;
    int x;
    int y;
    // Check that all types are valid
    for(i = 1; i < numClasses; i++)
    {
        // All (non-Object) superclass types are >= 0
        if(classesST[i].superclass < 0)
        {
            printf("Type error on line %d: invalid superclass (%s)\n", 
            classesST[i].classNameLineNumber, classesST[i].className);
            exit(-1);
        }
        
        // classtype != superclasstype (no classes extend themselves)
        if(classesST[i].superclass == i)
        {
            printf("Type error on line %d: superclass cannot be self (%s)\n", 
            classesST[i].classNameLineNumber, classesST[i].className);
            exit(-1);
        }
        // Class vars
        for( j = 0; j < classesST[i].numVars; j++)
        {
            if(classesST[i].varList[j].type < -1)
            {
                printf("Type error on line %d: invalid variable type (%s)\n", 
                classesST[i].varList[j].varNameLineNumber, classesST[i].varList[j].varName);
                exit(-1);
            }
        }
        // Method and param
        for(j = 0; j < classesST[i].numMethods; j++)
        {
            // Methods
            if(classesST[i].methodList[j].returnType < -1)
            {
                printf("Type error on line %d: invalid method return type (%s)\n", 
                classesST[i].methodList[j].returnTypeLineNumber, classesST[i].methodList[j].methodName);
                exit(-1);
            }

            // Are method names in this class unique?
            for(k = 0; k < classesST[i].numMethods; k++)
            {
                if(j == k) continue;
                if(strcmp(classesST[i].methodList[j].methodName, classesST[i].methodList[k].methodName) == 0)
                {
                    printf("Type error on line %d: duplicate method name (%s in %s)\n", 
                    classesST[i].methodList[j].methodNameLineNumber, classesST[i].methodList[j].methodName, 
                    classesST[i].className);
                    exit(-1);
                }
            }

            // Do overridden methods have the same signature?
            for(k = 0; k < classesST[classesST[i].superclass].numMethods; k++)
            {
                if(strcmp(classesST[i].methodList[j].methodName, classesST[classesST[i].superclass].methodList[k].methodName) == 0)
                {
                    // Are the return types the same?
                    if(classesST[i].methodList[j].returnType != classesST[classesST[i].superclass].methodList[k].returnType)
                    {
                        printf("Type error on line %d: overriden method has different signature (%s)\n", 
                        classesST[i].methodList[j].methodNameLineNumber, classesST[i].methodList[j].methodName);
                        exit(-1);
                    }

                    // Are the parameter types the same?
                    if(classesST[i].methodList[j].numParams == classesST[classesST[i].superclass].methodList[k].numParams)
                    {
                        for(x = 0; x < classesST[i].methodList[j].numParams; x++)
                        {
                            if(classesST[i].methodList[j].paramST[x].type != classesST[classesST[i].superclass].methodList[k].paramST[x].type)
                            {
                                printf("Type error on line %d: overriden method has different signature (%s)\n", 
                                classesST[i].methodList[j].methodNameLineNumber, classesST[i].methodList[j].methodName);
                                exit(-1);
                            }
                        }
                    }
                    else 
                    {
                        printf("Type error on line %d: overriden method has different signature (%s)\n", 
                        classesST[i].methodList[j].methodNameLineNumber, classesST[i].methodList[j].methodName);
                        exit(-1);
                    }
                    // Check for name collisions in locals
                    for(x = 0; x < classesST[i].methodList[j].numLocals; x++)
                    {
                        for(y = 0; y < classesST[classesST[i].superclass].methodList[k].numLocals; y++)
                        {
                            if(strcmp(
                                classesST[i].methodList[j].localST[x].varName,
                                classesST[classesST[i].superclass]
                                .methodList[k].localST[y].varName
                            ))
                            {
                                printf("Type error on line %d: duplicate variable in overriden method(%s)\n",
                                classesST[i].methodList[j].localST[x].varNameLineNumber,
                                classesST[i].methodList[j].localST[x].varName);
                            }
                        }
                    }
                }
            }

            // Is this method well-typed?
            
            int retType = typeExprs(classesST[i].methodList[j].bodyExprs, i, j);
            
            if(retType <= -3 || !isSubtype(retType, classesST[i].methodList[j].returnType))
            {
                printf("Type error on line %d: invalid return type (%s)\n", 
                classesST[i].methodList[j].methodNameLineNumber, classesST[i].methodList[j].methodName);
                exit(-1);
            }

            // Params
            for(k = 0; k < classesST[i].methodList[j].numParams; k++)
            {
                if(classesST[i].methodList[j].paramST[k].type < -1)
                {
                    printf("Type error on line %d: invalid parameter type (%s)\n", 
                    classesST[i].methodList[j].paramST[k].typeLineNumber, classesST[i].methodList[j].paramST[k].varName);
                    exit(-1);
                }
                // Are parameter names unique?
                for(x = 0; x < classesST[i].methodList[j].numParams; x++)
                {
                    if(k == x) continue;
                    if(strcmp(classesST[i].methodList[j].paramST[k].varName, classesST[i].methodList[j].paramST[x].varName) == 0)
                    {
                        printf("Type error on line %d: duplicate parameter name (%s)\n", 
                        classesST[i].methodList[j].paramST[k].typeLineNumber, classesST[i].methodList[j].paramST[k].varName);
                        exit(-1);
                    }
                }
            }
            // Locals
            for(k = 0; k < classesST[i].methodList[j].numLocals; k++)
            {
                if(classesST[i].methodList[j].localST[k].type < -1)
                {
                    printf("Type error on line %d: invalid local variable type (%s)\n", 
                    classesST[i].methodList[j].localST[k].typeLineNumber, classesST[i].methodList[j].localST[k].varName);
                    exit(-1);
                }
                // Are local names unique?
                for(x = 0; x < classesST[i].methodList[j].numLocals; x++)
                {
                    if(strcmp(classesST[i].methodList[j].localST[k].varName, classesST[i].methodList[j].localST[x].varName) == 0)
                    {
                        if(k == x) continue;
                        printf("Type error on line %d: duplicate local name (%s)\n", 
                        classesST[i].methodList[j].localST[k].typeLineNumber, classesST[i].methodList[j].localST[k].varName);
                        exit(-1);
                    }
                }
                // Are there duplicate names among this method's locals and params?
                for(x = 0; x < classesST[i].methodList[j].numParams; x++)
                {
                    if(strcmp(classesST[i].methodList[j].localST[k].varName, classesST[i].methodList[j].paramST[x].varName) == 0)
                    {
                        printf("Type error on line %d: duplicate local and parameter name (%s)\n", 
                        classesST[i].methodList[j].localST[k].typeLineNumber, classesST[i].methodList[j].localST[k].varName);
                        exit(-1);
                    }
                }
            }
        }
    }

    // Check that class names are unique
    for(i = 0; i < numClasses; i++)
    {
        for(j = 0; j < numClasses; j++)
        {
            if(i == j) continue;
            if(strcmp(classesST[i].className, classesST[j].className) == 0)
            {
                printf("Type error on line %d: duplicate class name (%s)\n", 
                classesST[i].classNameLineNumber, classesST[i].className);
                exit(-1);
            }
            // Check that classes aren't cyclic
            if(isSubtype(i, j) && isSubtype(j, i))
            {
                printf("Type error on line %d: cyclic class declaration (%s, %s)\n", 
                classesST[i].classNameLineNumber, classesST[i].className, classesST[j].className);
                exit(-1);
            }
        }
    }

    // Check for local collisions
    for(i = 1; i < numClasses; i++)
    {
        // In this class
        for(j = 0; j < classesST[i].numVars; j++)
        {
            for(k = 0; k < classesST[i].numVars; k++)
            {
                if(!strcmp(classesST[i].varList[j].varName, 
                classesST[i].varList[k].varName))
                {
                    if(j==k) continue;
                    printf("Type error on line %d: duplicate variable name (%s)\n", 
                    classesST[i].varList[j].varNameLineNumber, classesST[i].varList[j].varName);
                    exit(-1);
                }
            }
        }
        // In superclass
        if(classesST[i].superclass > 0)
        {
            for(j = 0; j < classesST[i].numVars; j++)
            {
                for(k = 0; k < classesST[classesST[i].superclass].numVars; k++)
                {
                    if(!strcmp(classesST[i].varList[j].varName,
                       classesST[classesST[i].superclass].varList[k].varName))
                       {
                            printf("Type error on line %d: duplicate variable name in subclass (%s)\n", 
                            classesST[i].varList[j].varNameLineNumber, classesST[i].varList[j].varName);
                            exit(-1);
                       }
                }
            }
        }
    }

    // Check that main block variable names have no duplicates
    for(i = 0; i < numMainBlockLocals; i++)
    {
        for(j = 0; j < numMainBlockLocals; j++)
        {
            if(i == j) continue;
            if(strcmp(mainBlockST[i].varName, mainBlockST[j].varName) == 0)
            {
                printf("Type error on line %d: duplicate local name (%s)\n", mainBlockST[i].varNameLineNumber, mainBlockST[i].varName);
                exit(-1);
            }
        }
    }

    // Check that the main block expression list is well-typed
    typeExprs(mainExprs, -1, -1);
}

int isSubtype(int sub, int super)
{
    if(sub == super) return 1;
    if(sub == -2) return 1; // null is AOT
    if(sub == -1 || super == -1)
        return sub == super;
        
    int it = sub;
    while(classesST[it].superclass > -1)
    {
        if(classesST[it].superclass == super) return 1;
        else it = classesST[it].superclass;
        if(it == sub)
        {
            printf("Type error on line %d: cyclic class declaration (%s, %s)\n", 
            classesST[it].classNameLineNumber, classesST[it].className, classesST[classesST[it].superclass].className);
            exit(-1);
        }
    }
    return 0;
}

int typeExpr(ASTree *t, int classContainingExpr, int methodContainingExpr)
{
    t->staticClassNum = classContainingExpr;
    t->staticMemberNum = methodContainingExpr;
    if(t == NULL)
    {
        printf("Type error on line %d: null type", t->lineNumber);
        exit(-1);
    }
    else if(t->typ == VAR_DECL)
    {
        if(t->children->data->idVal)
        {
            int type = typeNameToNumber(t->children->data->idVal);
            if(type < 0)
            {
                printf("Type error on line %d: invalid object type\n",
                t->lineNumber);
                exit(-1);
            }
            return type;
        }
        else
        {
            return -1;
        }
    }
    // E.M() -> Check if expression is valid type,
    // check if ID is valid
    // check if method exists in class or superclass
    // check if method return type is valid
    else if(t->typ == DOT_METHOD_CALL_EXPR)
    {
        int expType = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        if(expType < 0)
        {
            printf("Type error: dot method call on invalid type\n");
            exit(-1);
        }

        int idType = checkForMethod(t->children->next->data->idVal, expType);
        if(idType == -5)
            idType = checkForMethod(t->children->next->data->idVal, classesST[expType].superclass);

        
        if(!isSubtype(idType, classesST[classContainingExpr].methodList[methodContainingExpr].returnType))
        {
            printf("Type error: invalid method return type\n");
            exit(-1);
        }
        return classesST[classContainingExpr].methodList[methodContainingExpr].returnType;
    }
    // M() -> Check if id is valid, check if method return type is valid
    else if(t->typ == METHOD_CALL_EXPR)
    {
        int idType = checkForID(t->children->data->idVal, classContainingExpr, methodContainingExpr);

        return idType;
    }
    // T.ID -> check if T is type, check if ID exists in context
    else if(t->typ == DOT_ID_EXPR)
    {
        if(typeNameToNumber(t->childrenTail->data->idVal) < 1)
        {
            printf("Type error: dot id call on invalid type\n");
            exit(-1);
        }

        int type = checkForID(t->childrenTail->data->idVal, 
        classContainingExpr, methodContainingExpr);
        if(!type)
        {
            printf("Type error on line %d: invalid identifier (%s)\n", t->lineNumber, t->idVal);
            exit(-1);
        }
        return type;
    }
    // ID -> check if ID exists in context
    else if(t->typ == AST_ID)
    {
        return checkForID(t->idVal, classContainingExpr, methodContainingExpr);
    }
    else if(t->typ == ID_EXPR)
    {
        int idType = checkForID(t->children->data->idVal, 
        classContainingExpr, methodContainingExpr);
        if(!idType)
        {
            printf("Type error on line %d: invalid identifier (%s)\n", t->lineNumber, t->idVal);
            exit(-1);
        }
        return idType;
    }
    else if(t->typ == DOT_ASSIGN_EXPR)
    {
        int dotType = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        if(dotType < 0)
        {
            printf("Type error on line %d: invalid identifier (%s)\n", t->lineNumber, t->idVal);
            exit(-1);
        }

        int idType = typeExpr(t->children->next->data, dotType, methodContainingExpr);
        int expressionType = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr);
        if(!isSubtype(idType, expressionType))
        {
            printf("Type error on line %d: incompatible assignment\n", t->lineNumber);
        }

        return idType;
    }
    else if(t->typ == ASSIGN_EXPR)
    {
        int idType = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        int expressionType = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr);
        if(!isSubtype(idType, expressionType))
        {
            printf("Type error on line %d: incompatible assignment\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    // E1 + E2 -> check if both sides evaluate to nat
    else if(t->typ == PLUS_EXPR)
    {
        int lhs = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        int rhs = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr);

        if(lhs != -1 || rhs != -1)
        {
            printf("Type error on line %d: attempt to add non-nat type\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    else if(t->typ == MINUS_EXPR)
    {
        int lhs = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        int rhs = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr);

        if(lhs != -1 || rhs != -1)
        {
            printf("Type error on line %d: attempt to subtract non-nat type\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    else if(t->typ == TIMES_EXPR)
    {
        int lhs = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        int rhs = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr);

        if(lhs != -1 || rhs != -1)
        {
            printf("Type error on line %d: attempt to multiply non-nat type\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    // E1==E2 -> returns nat
    // check if E1 and E2 evaluate
    // to types that are subtypes
    else if(t->typ == EQUALITY_EXPR)
    {
        int lhs = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        int rhs = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr); 

        if(!isSubtype(lhs, rhs) && !isSubtype(rhs, lhs))
        {
            printf("Type error on line %d: equality check on non-compatible types\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }

    else if(t->typ == GREATER_THAN_EXPR)
    {
        int lhs = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        int rhs = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr);

        if(!isSubtype(lhs, rhs) && !isSubtype(rhs, lhs))
        {
            printf("Type error on line %d: attempt to compare non-compatible type\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    // !E -> check if E is nat type
    else if(t->typ == NOT_EXPR)
    {
        int type = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);

        if(type != -1)
        {
            printf("Type error on line %d: not operation on non-nat value\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    // Check if both sides are nat
    else if(t->typ == AND_EXPR)
    {
        int lhs = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        int rhs = typeExpr(t->childrenTail->data, classContainingExpr, methodContainingExpr);

        if(lhs != -1 || rhs != -1)
        {
            printf("Type error on line %d: logical operation on non-nat value\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    // if(E) then { EL1 } else { EL2 } ->
    // Check if E is a nat value
    // Check the types of EL1 and EL2
    // If both are nat, return nat
    // If both are objects, return the join of their classes
    else if(t->typ == IF_THEN_ELSE_EXPR)
    {
        int test = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        if(test != -1)
        {
            printf("Type error on line %d: if test expression does not evaluate to nat\n", t->lineNumber);
            exit(-1);
        }

        int E1 = typeExprs(t->children->next->data, classContainingExpr, methodContainingExpr);
        int E2 = typeExprs(t->childrenTail->data, classContainingExpr, methodContainingExpr);

        if(E1 == E2) // regardless of types, if these are equal, it is well-typed
        {
            return E1;
        }
        else if(E1 != -1 && E2 != -1)
        {
            return join(E1, E2);
        }
        else
        {
            printf("Type error on line %d: invalid return types from else-then branches\n", t->lineNumber);
            exit(-1);
        }

        printf("Type error on line %d: invalid return types from else-then branches\n", t->lineNumber);
        exit(-1);
    }

    else if(t->typ == WHILE_EXPR)
    {
        int testExp = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        if(testExp != -1)
        {
            printf("Type error on line %d: loop test expression does not evaluate to nat\n", t->lineNumber);
            exit(-1);
        }

        return typeExprs(t->childrenTail->data, classContainingExpr, methodContainingExpr);
    }
    // Print statements return the nat value they printed, but check if type is nat
    else if(t->typ == PRINT_EXPR)
    {
        int toPrint = typeExpr(t->children->data, classContainingExpr, methodContainingExpr);
        if(toPrint != -1)
        {
            printf("Type error on line %d: attempt to print non-nat type\n", t->lineNumber);
            exit(-1);
        }

        return -1;
    }
    // readNat returns a nat type
    else if(t->typ == READ_EXPR)
    {
        return -1;
    }
    // This refers to the class you are currently in
    else if(t->typ == THIS_EXPR)
    {
        if(classContainingExpr < 0)
        {
            printf("Type error on line %d: this does not refer to object\n", t->lineNumber);
            exit(-1);
        }
        return classContainingExpr;
    }
    // new C() -> C is first child, which is also type
    // Must be object type (>= 0)
    else if(t->typ == NEW_EXPR)
    {
        int childType = checkForID(t->children->data->idVal, classContainingExpr, methodContainingExpr);
        if(childType <= -1)
        {
            printf("Type error on line %d: object must be of class type\n", t->lineNumber);
            exit(-1);
        }

        return childType;
    }
    else if(t->typ == NULL_EXPR)
    {
        return -2;
    }
    else if(t->typ == NAT_LITERAL_EXPR)
    {
        return -1;
    }
    return -10;
}

int typeExprs(ASTree *t, int classContainingExprs, int methodContainingExprs)
{
    if(t->typ == VAR_DECL_LIST || t->typ == EXPR_LIST)
    {
        ASTList* it = t->children;
        int eval = -5;
        while(it)
        {
            eval = typeExpr(it->data, classContainingExprs, methodContainingExprs);
            it = it->next;
        }
        return eval;
    }
    else
    {
        ASTList* it = t->children;
        int eval = -5;
        while(it)
        {
            eval = typeExprs(it->data, classContainingExprs, methodContainingExprs);
            it = it->next;
        }
        return eval;
    }
}