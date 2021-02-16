/* File: codegen.cc
 * ----------------
 * Implementation for the CodeGenerator class. The methods don't do anything
 * too fancy, mostly just create objects of the various Tac instruction
 * classes and append them to the list.
 */

#include "codegen.h"
#include <string.h>
#include "tac.h"
#include "mips.h"
#include <iostream>
#include <fstream>
#include <sstream>

Location* CodeGenerator::ThisPtr = new Location(fpRelative, 4, "this");

CodeGenerator::CodeGenerator() {
    local_loc = OffsetToFirstLocal;     // -8, -12, -16, ...
    param_loc = OffsetToFirstParam;     // 4, 8, 12, ...
    globl_loc = OffsetToFirstGlobal;    // 0, 4, 8, ...
}

int CodeGenerator::GetNextLocalLoc() {
    int n = local_loc;
    local_loc -= VarSize;
    return n;
}

int CodeGenerator::GetNextParamLoc() {
    int n = param_loc;
    param_loc += VarSize;
    return n;
}

int CodeGenerator::GetNextGlobalLoc() {
    int n = globl_loc;
    globl_loc += VarSize;
    return n;
}

int CodeGenerator::GetFrameSize() {
    return OffsetToFirstLocal - local_loc;
}

void CodeGenerator::ResetFrameSize() {
    local_loc = OffsetToFirstLocal;
    param_loc = OffsetToFirstParam;
}

char *CodeGenerator::NewLabel() {
    static int nextLabelNum = 0;
    char temp[10];
    sprintf(temp, "_L%d", nextLabelNum++);
    return strdup(temp);
}

Location *CodeGenerator::GenTempVar() {
    static int nextTempNum;
    char temp[10];
    Location *result = NULL;
    sprintf(temp, "_tmp%d", nextTempNum++);
    /* pp5: need to create variable in proper location
       in stack frame for use as temporary. Until you
       do that, the assert below will always fail to remind
       you this needs to be implemented  */
    result = new Location(fpRelative, GetNextLocalLoc(), temp);
    Assert(result != NULL);
    return result;
}

Location *CodeGenerator::GenLoadConstant(int value) {
    Location *result = GenTempVar();
    code.push_back(new LoadConstant(result, value));
    return result;
}

Location *CodeGenerator::GenLoadConstant(const char *s) {
    Location *result = GenTempVar();
    code.push_back(new LoadStringConstant(result, s));
    return result;
}

Location *CodeGenerator::GenLoadLabel(const char *label) {
    Location *result = GenTempVar();
    code.push_back(new LoadLabel(result, label));
    return result;
}

void CodeGenerator::GenAssign(Location *dst, Location *src) {
    code.push_back(new Assign(dst, src));
}

Location *CodeGenerator::GenLoad(Location *ref, int offset) {
    Location *result = GenTempVar();
    code.push_back(new Load(result, ref, offset));
    return result;
}

void CodeGenerator::GenStore(Location *dst,Location *src, int offset) {
    code.push_back(new Store(dst, src, offset));
}

Location *CodeGenerator::GenBinaryOp(const char *opName, Location *op1,
        Location *op2)
{
    Location *result = GenTempVar();
    code.push_back(new
            BinaryOp(BinaryOp::OpCodeForName(opName), result, op1, op2));
    return result;
}


void CodeGenerator::GenLabel(const char *label) {
    code.push_back(new Label(label));
}

void CodeGenerator::GenIfZ(Location *test, const char *label) {
    code.push_back(new IfZ(test, label));
}

void CodeGenerator::GenGoto(const char *label) {
    code.push_back(new Goto(label));
}

void CodeGenerator::GenReturn(Location *val) {
    code.push_back(new Return(val));
}

BeginFunc *CodeGenerator::GenBeginFunc() {
    ResetFrameSize();
    BeginFunc *result = new BeginFunc;
    code.push_back(result);
    return result;
}

void CodeGenerator::GenEndFunc() {
    code.push_back(new EndFunc());
}

void CodeGenerator::GenPushParam(Location *param) {
    code.push_back(new PushParam(param));
}

void CodeGenerator::GenPopParams(int numBytesOfParams) {
    Assert(numBytesOfParams >= 0
            && numBytesOfParams % VarSize == 0); // sanity check
    if (numBytesOfParams > 0)
        code.push_back(new PopParams(numBytesOfParams));
}

Location *CodeGenerator::GenLCall(const char *label, bool fnHasReturnValue) {
    Location *result = fnHasReturnValue ? GenTempVar() : NULL;
    code.push_back(new LCall(label, result));
    return result;
}

Location *CodeGenerator::GenACall(Location *fnAddr, bool fnHasReturnValue) {
    Location *result = fnHasReturnValue ? GenTempVar() : NULL;
    code.push_back(new ACall(fnAddr, result));
    return result;
}

static struct _builtin {
    const char *label;
    int numArgs;
    bool hasReturn;
} builtins[] = {
    {"_Alloc", 1, true},
    {"_ReadLine", 0, true},
    {"_ReadInteger", 0, true},
    {"_StringEqual", 2, true},
    {"_PrintInt", 1, false},
    {"_PrintString", 1, false},
    {"_PrintBool", 1, false},
    {"_Halt", 0, false},
//    {"_SemanError", 0, false}
};

Location *CodeGenerator::GenBuiltInCall(BuiltIn bn,Location *arg1,
        Location *arg2)
{
    Assert(bn >= 0 && bn < NumBuiltIns);
    struct _builtin *b = &builtins[bn];
    Location *result = NULL;

    if (b->hasReturn) result = GenTempVar();
    // verify appropriate number of non-NULL arguments given
    Assert((b->numArgs == 0 && !arg1 && !arg2)
            || (b->numArgs == 1 && arg1 && !arg2)
            || (b->numArgs == 2 && arg1 && arg2));
    if (arg2) code.push_back(new PushParam(arg2));
    if (arg1) code.push_back(new PushParam(arg1));
    code.push_back(new LCall(b->label, result));
    GenPopParams(VarSize*b->numArgs);
    return result;
}

void CodeGenerator::GenVTable(const char *className,
        List<const char *> *methodLabels)
{
    code.push_back(new VTable(className, methodLabels));
}

void CodeGenerator::DoFinalCodeGen() {
    if (IsDebugOn("tac")) { // if debug don't translate to mips, just print Tac
        std::list<Instruction*>::iterator p;
        for (p= code.begin(); p != code.end(); ++p) {
            (*p)->Print();
        }
    }  else {
        Mips mips;
        mips.EmitPreamble();

        std::list<Instruction*>::iterator p;
        for (p= code.begin(); p != code.end(); ++p) {
            (*p)->Emit(&mips);
        }

        printf("    # Prewritten asm\n");
        std::ifstream i("./src/defs.asm");
        std::stringstream buf;
        buf << i.rdbuf();
        printf("%s",buf.str().c_str());
    }
}

