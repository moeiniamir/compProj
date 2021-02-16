

#include "codegen.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <stdarg.h>


Location *CodeGenerator::ThisPtr = new Location(fpRelative, 4, "this");

CodeGenerator::CodeGenerator() {
    local_loc = OffsetToFirstLocal;
    param_loc = OffsetToFirstParam;
    globl_loc = OffsetToFirstGlobal;
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

    result = new Location(fpRelative, GetNextLocalLoc(), temp);

    return result;
}

Location *CodeGenerator::GenLoadConstant(int value) {
    Location *result = GenTempVar();
    code.push_back(new LoadConstant(result, value));
    return result;
}

Location *CodeGenerator::GenLoadConstant(const char *s) {
    Location *result = GenTempVar();
    code.push_back(new LoadStringLiteral(result, s));
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

void CodeGenerator::GenStore(Location *dst, Location *src, int offset) {
    code.push_back(new Store(dst, src, offset));
}

Location *CodeGenerator::GenBinaryOp(const char *opName, Location *op1,
                                     Location *op2) {
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
        {"_Alloc",       1, true},
        {"_ReadLine",    0, true},
        {"_ReadInteger", 0, true},
        {"_StringEqual", 2, true},
        {"_PrintInt",    1, false},
        {"_PrintString", 1, false},
        {"_PrintBool",   1, false},
        {"_Halt",        0, false},

};

Location *CodeGenerator::GenBuiltInCall(BuiltIn bn, Location *arg1,
                                        Location *arg2) {

    struct _builtin *b = &builtins[bn];
    Location *result = NULL;

    if (b->hasReturn) result = GenTempVar();
    if (arg2) code.push_back(new PushParam(arg2));
    if (arg1) code.push_back(new PushParam(arg1));
    code.push_back(new LCall(b->label, result));
    GenPopParams(VarSize * b->numArgs);
    return result;
}

void CodeGenerator::GenVTable(const char *className,
                              List<const char *> *methodLabels) {
    code.push_back(new VTable(className, methodLabels));
}

void CodeGenerator::DoFinalCodeGen() {

    Mips mips;
    mips.EmitPreamble();

    std::list<Instruction *>::iterator p;
    for (p = code.begin(); p != code.end(); ++p) {
        (*p)->Emit(&mips);
    }

    printf("    # Prewritten asm\n");
    std::ifstream i("./src/builtin.asm");
    std::stringstream buf;
    buf << i.rdbuf();
    printf("%s", buf.str().c_str());
}


Location::Location(Segment s, int o, const char *name) :
        variableName(strdup(name)), segment(s), offset(o), base(NULL) {}

Location::Location(Segment s, int o, const char *name, Location *b) :
        variableName(strdup(name)), segment(s), offset(o), base(b) {}


void Instruction::Emit(Mips *mips) {
    Mips::CurrentInstruction ci(*mips, this);
    if (*printed)
        mips->Emit("# %s", printed);
    EmitSpecific(mips);
}

LoadConstant::LoadConstant(Location *d, int v)
        : dst(d), val(v) {
    ;
    sprintf(printed, "%s = %d", dst->GetName(), val);
}

void LoadConstant::EmitSpecific(Mips *mips) {
    mips->EmitLoadConstant(dst, val);
}

LoadStringLiteral::LoadStringLiteral(Location *d, const char *s)
        : dst(d) {
    ;
    const char *quote = (*s == '"') ? "" : "\"";
    str = new char[strlen(s) + 2 * strlen(quote) + 1];
    sprintf(str, "%s%s%s", quote, s, quote);
    quote = (strlen(str) > 50) ? "...\"" : "";
    sprintf(printed, "%s = %.50s%s", dst->GetName(), str, quote);
}

void LoadStringLiteral::EmitSpecific(Mips *mips) {
    mips->EmitLoadStringLiteral(dst, str);
}

LoadLabel::LoadLabel(Location *d, const char *l)
        : dst(d), label(strdup(l)) {
    ;
    sprintf(printed, "%s = %s", dst->GetName(), label);
}

void LoadLabel::EmitSpecific(Mips *mips) {
    mips->EmitLoadLabel(dst, label);
}


Assign::Assign(Location *d, Location *s)
        : dst(d), src(s) {
    ;
    sprintf(printed, "%s = %s", dst->GetName(), src->GetName());
}

void Assign::EmitSpecific(Mips *mips) {
    mips->EmitCopy(dst, src);
}

Load::Load(Location *d, Location *s, int off)
        : dst(d), src(s), offset(off) {
    ;
    if (offset)
        sprintf(printed, "%s = *(%s + %d)", dst->GetName(), src->GetName(),
                offset);
    else
        sprintf(printed, "%s = *(%s)", dst->GetName(), src->GetName());
}

void Load::EmitSpecific(Mips *mips) {
    mips->EmitLoad(dst, src, offset);
}

Store::Store(Location *d, Location *s, int off)
        : dst(d), src(s), offset(off) {
    ;
    if (offset)
        sprintf(printed, "*(%s + %d) = %s", dst->GetName(), offset,
                src->GetName());
    else
        sprintf(printed, "*(%s) = %s", dst->GetName(), src->GetName());
}

void Store::EmitSpecific(Mips *mips) {
    mips->EmitStore(dst, src, offset);
}

const char *const BinaryOp::opName[BinaryOp::NumOps] = {
        "+", "-", "*", "/", "%",
        "==", "!=", "<", "<=", ">", ">=",
        "&&", "||"
};

BinaryOp::OpCode BinaryOp::OpCodeForName(const char *name) {
    for (int i = 0; i < NumOps; i++)
        if (opName[i] && !strcmp(opName[i], name))
            return (OpCode) i;

    return Add;
}

BinaryOp::BinaryOp(OpCode c, Location *d, Location *o1, Location *o2)
        : code(c), dst(d), op1(o1), op2(o2) {
    ;;
    sprintf(printed, "%s = %s %s %s", dst->GetName(), op1->GetName(),
            opName[code], op2->GetName());
}

void BinaryOp::EmitSpecific(Mips *mips) {
    mips->EmitBinaryOp(code, dst, op1, op2);
}

Label::Label(const char *l) : label(strdup(l)) {
    ;
    *printed = '\0';
}


void Label::EmitSpecific(Mips *mips) {
    mips->EmitLabel(label);
}

Goto::Goto(const char *l) : label(strdup(l)) {
    ;
    sprintf(printed, "Goto %s", label);
}

void Goto::EmitSpecific(Mips *mips) {
    mips->EmitGoto(label);
}

IfZ::IfZ(Location *te, const char *l)
        : test(te), label(strdup(l)) {
    ;
    sprintf(printed, "IfZ %s Goto %s", test->GetName(), label);
}

void IfZ::EmitSpecific(Mips *mips) {
    mips->EmitIfZ(test, label);
}

BeginFunc::BeginFunc() {
    sprintf(printed, "BeginFunc (unassigned)");
    frameSize = -555;
}

void BeginFunc::SetFrameSize(int numBytesForAllLocalsAndTemps) {
    frameSize = numBytesForAllLocalsAndTemps;
    sprintf(printed, "BeginFunc %d", frameSize);
}

void BeginFunc::EmitSpecific(Mips *mips) {
    mips->EmitBeginFunction(frameSize);
}

EndFunc::EndFunc() : Instruction() {
    sprintf(printed, "EndFunc");
}

void EndFunc::EmitSpecific(Mips *mips) {
    mips->EmitEndFunction();
}

Return::Return(Location *v) : val(v) {
    sprintf(printed, "Return %s", val ? val->GetName() : "");
}

void Return::EmitSpecific(Mips *mips) {
    mips->EmitReturn(val);
}

PushParam::PushParam(Location *p)
        : param(p) {
    ;
    sprintf(printed, "PushParam %s", param->GetName());
}

void PushParam::EmitSpecific(Mips *mips) {
    mips->EmitParam(param);
}

PopParams::PopParams(int nb)
        : numBytes(nb) {
    sprintf(printed, "PopParams %d", numBytes);
}

void PopParams::EmitSpecific(Mips *mips) {
    mips->EmitPopParams(numBytes);
}

LCall::LCall(const char *l, Location *d)
        : label(strdup(l)), dst(d) {
    sprintf(printed, "%s%sLCall %s", dst ? dst->GetName() : "", dst ? " = " : "",
            label);
}

void LCall::EmitSpecific(Mips *mips) {
    mips->EmitLCall(dst, label);
}

ACall::ACall(Location *ma, Location *d)
        : dst(d), methodAddr(ma) {
    ;
    sprintf(printed, "%s%sACall %s", dst ? dst->GetName() : "", dst ? " = " : "",
            methodAddr->GetName());
}

void ACall::EmitSpecific(Mips *mips) {
    mips->EmitACall(dst, methodAddr);
}

VTable::VTable(const char *l, List<const char *> *m)
        : methodLabels(m), label(strdup(l)) {
    ;
    sprintf(printed, "VTable for class %s", l);
}


void VTable::EmitSpecific(Mips *mips) {
    mips->EmitVTable(label, methodLabels);
}


static bool LocationsAreSame(Location *var1, Location *var2) {
    return (var1 == var2 ||
            (var1 && var2
             && !strcmp(var1->GetName(), var2->GetName())
             && var1->GetSegment() == var2->GetSegment()
             && var1->GetOffset() == var2->GetOffset()));
}


void Mips::SpillRegister(Location *dst, Register reg) {
    ;
    const char *offsetFromWhere = dst->GetSegment() == fpRelative
                                  ? regs[fp].name : regs[gp].name;;
    Emit("sw %s, %d(%s)\t# spill %s from %s to %s%+d", regs[reg].name,
         dst->GetOffset(), offsetFromWhere, dst->GetName(), regs[reg].name,
         offsetFromWhere, dst->GetOffset());
}


void Mips::FillRegister(Location *src, Register reg) {
    ;
    const char *offsetFromWhere = src->GetSegment() == fpRelative
                                  ? regs[fp].name : regs[gp].name;;
    Emit("lw %s, %d(%s)\t# fill %s to %s from %s%+d", regs[reg].name,
         src->GetOffset(), offsetFromWhere, src->GetName(), regs[reg].name,
         offsetFromWhere, src->GetOffset());
}


void Mips::Emit(const char *fmt, ...) {
    va_list args;
    char buf[1024];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    for (int i = 1023; i >= 0; i--) {


        if (buf[i] == '#' && buf[i + 1] == ' ') {
            buf[i] = 0;
            break;
        }
    }

    if (buf[strlen(buf) - 1] != ':') printf("\t");
    if (buf[0] != '#') printf("  ");
    printf("%s", buf);
    if (buf[strlen(buf) - 1] != '\n') printf("\n");
}


void Mips::EmitLoadConstant(Location *dst, int val) {
    Register r = rd;
    Emit("li %s, %d\t\t# load constant value %d into %s", regs[r].name,
         val, val, regs[r].name);
    SpillRegister(dst, rd);
}


void Mips::EmitLoadStringLiteral(Location *dst, const char *str) {
    static int strNum = 1;
    char label[16];
    sprintf(label, "_string%d", strNum++);
    Emit(".data\t\t\t# create string constant marked with label");
    Emit("%s: .asciiz %s", label, str);
    Emit(".text");
    EmitLoadLabel(dst, label);
}


void Mips::EmitLoadLabel(Location *dst, const char *label) {
    Emit("la %s, %s\t# load label", regs[rd].name, label);
    SpillRegister(dst, rd);
}


void Mips::EmitCopy(Location *dst, Location *src) {
    FillRegister(src, rd);
    SpillRegister(dst, rd);
}


void Mips::EmitLoad(Location *dst, Location *reference, int offset) {
    FillRegister(reference, rs);
    Emit("lw %s, %d(%s) \t# load with offset", regs[rd].name,
         offset, regs[rs].name);
    SpillRegister(dst, rd);
}


void Mips::EmitStore(Location *reference, Location *value, int offset) {
    FillRegister(value, rs);
    FillRegister(reference, rd);
    Emit("sw %s, %d(%s) \t# store with offset",
         regs[rs].name, offset, regs[rd].name);
}


void Mips::EmitBinaryOp(BinaryOp::OpCode code, Location *dst,
                        Location *op1, Location *op2) {
    FillRegister(op1, rs);
    FillRegister(op2, rt);
    Emit("%s %s, %s, %s\t", NameForTac(code), regs[rd].name,
         regs[rs].name, regs[rt].name);
    SpillRegister(dst, rd);
}


void Mips::EmitLabel(const char *label) {
    Emit("%s:", label);
}


void Mips::EmitGoto(const char *label) {
    Emit("b %s\t\t# unconditional branch", label);
}


void Mips::EmitIfZ(Location *test, const char *label) {
    FillRegister(test, rs);
    Emit("beqz %s, %s\t# branch if %s is zero ", regs[rs].name, label,
         test->GetName());
}


void Mips::EmitParam(Location *arg) {
    Emit("subu $sp, $sp, 4\t# decrement sp to make space for param");
    FillRegister(arg, rs);
    Emit("sw %s, 4($sp)\t# copy param value to stack", regs[rs].name);
}


void Mips::EmitCallInstr(Location *result, const char *fn, bool isLabel) {
    Emit("%s %-15s\t# jump to function", isLabel ? "jal" : "jalr", fn);
    if (result != NULL) {
        Emit("move %s, %s\t\t# copy function return value from $v0",
             regs[rd].name, regs[v0].name);
        SpillRegister(result, rd);
    }
}


void Mips::EmitLCall(Location *dst, const char *label) {
    EmitCallInstr(dst, label, true);
}

void Mips::EmitACall(Location *dst, Location *fn) {
    FillRegister(fn, rs);
    EmitCallInstr(dst, regs[rs].name, false);
}


void Mips::EmitPopParams(int bytes) {
    if (bytes != 0)
        Emit("add $sp, $sp, %d\t# pop params off stack", bytes);
}


void Mips::EmitReturn(Location *returnVal) {
    if (returnVal != NULL) {
        FillRegister(returnVal, rd);
        Emit("move $v0, %s\t\t# assign return value into $v0",
             regs[rd].name);
    }
    Emit("move $sp, $fp\t\t# pop callee frame off stack");
    Emit("lw $ra, -4($fp)\t# restore saved ra");
    Emit("lw $fp, 0($fp)\t# restore saved fp");
    Emit("jr $ra\t\t# return from function");
}


void Mips::EmitBeginFunction(int stackFrameSize) {
    ;
    Emit("subu $sp, $sp, 8\t# decrement sp to make space to save ra, fp");
    Emit("sw $fp, 8($sp)\t# save fp");
    Emit("sw $ra, 4($sp)\t# save ra");
    Emit("addiu $fp, $sp, 8\t# set up new fp");

    if (stackFrameSize != 0)
        Emit(
                "subu $sp, $sp, %d\t# decrement sp to make space for locals/temps",
                stackFrameSize);
}


void Mips::EmitEndFunction() {
    Emit("# (below handles reaching end of fn body with no explicit return)");
    EmitReturn(NULL);
}


void Mips::EmitVTable(const char *label, List<const char *> *methodLabels) {
    Emit(".data");
    Emit(".align 2");
    Emit("%s:\t\t# label for class %s vtable", label, label);
    for (int i = 0; i < methodLabels->NumElements(); i++)
        Emit(".word %s\n", methodLabels->Nth(i));
    Emit(".text");
}


void Mips::EmitPreamble() {
    Emit("# standard Decaf preamble ");
    Emit(".text");
    Emit(".align 2");
    Emit(".globl main");
}


const char *Mips::NameForTac(BinaryOp::OpCode code) {
    ;
    const char *name = mipsName[code];;
    return name;
}


Mips::Mips() {
    mipsName[BinaryOp::Add] = "add";
    mipsName[BinaryOp::Sub] = "sub";
    mipsName[BinaryOp::Mul] = "mul";
    mipsName[BinaryOp::Div] = "div";
    mipsName[BinaryOp::Mod] = "rem";
    mipsName[BinaryOp::Eq] = "seq";
    mipsName[BinaryOp::Ne] = "sne";
    mipsName[BinaryOp::Lt] = "slt";
    mipsName[BinaryOp::Le] = "sle";
    mipsName[BinaryOp::Gt] = "sgt";
    mipsName[BinaryOp::Ge] = "sge";
    mipsName[BinaryOp::And] = "and";
    mipsName[BinaryOp::Or] = "or";
    regs[zero] = (RegContents) {false, NULL, "$zero", false};
    regs[at] = (RegContents) {false, NULL, "$at", false};
    regs[v0] = (RegContents) {false, NULL, "$v0", false};
    regs[v1] = (RegContents) {false, NULL, "$v1", false};
    regs[a0] = (RegContents) {false, NULL, "$a0", false};
    regs[a1] = (RegContents) {false, NULL, "$a1", false};
    regs[a2] = (RegContents) {false, NULL, "$a2", false};
    regs[a3] = (RegContents) {false, NULL, "$a3", false};
    regs[k0] = (RegContents) {false, NULL, "$k0", false};
    regs[k1] = (RegContents) {false, NULL, "$k1", false};
    regs[gp] = (RegContents) {false, NULL, "$gp", false};
    regs[sp] = (RegContents) {false, NULL, "$sp", false};
    regs[fp] = (RegContents) {false, NULL, "$fp", false};
    regs[ra] = (RegContents) {false, NULL, "$ra", false};
    regs[t0] = (RegContents) {false, NULL, "$t0", true};
    regs[t1] = (RegContents) {false, NULL, "$t1", true};
    regs[t2] = (RegContents) {false, NULL, "$t2", true};
    regs[t3] = (RegContents) {false, NULL, "$t3", true};
    regs[t4] = (RegContents) {false, NULL, "$t4", true};
    regs[t5] = (RegContents) {false, NULL, "$t5", true};
    regs[t6] = (RegContents) {false, NULL, "$t6", true};
    regs[t7] = (RegContents) {false, NULL, "$t7", true};
    regs[t8] = (RegContents) {false, NULL, "$t8", true};
    regs[t9] = (RegContents) {false, NULL, "$t9", true};
    regs[s0] = (RegContents) {false, NULL, "$s0", true};
    regs[s1] = (RegContents) {false, NULL, "$s1", true};
    regs[s2] = (RegContents) {false, NULL, "$s2", true};
    regs[s3] = (RegContents) {false, NULL, "$s3", true};
    regs[s4] = (RegContents) {false, NULL, "$s4", true};
    regs[s5] = (RegContents) {false, NULL, "$s5", true};
    regs[s6] = (RegContents) {false, NULL, "$s6", true};
    regs[s7] = (RegContents) {false, NULL, "$s7", true};
    rs = t0;
    rt = t1;
    rd = t2;
}

const char *Mips::mipsName[BinaryOp::NumOps];