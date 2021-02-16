

#ifndef _H_codegen
#define _H_codegen

#include <cstdlib>
#include <list>
#include "ds.h"


typedef enum { Alloc, ReadLine, ReadInteger, StringEqual,
               PrintInt, PrintString, PrintBool, Halt, NumBuiltIns } BuiltIn;

class Location;
class Mips;
class Instruction;
class BeginFunc;

class CodeGenerator {
  private:
    std::list<Instruction*> code;
    int local_loc;
    int param_loc;
    int globl_loc;

  public:










    static const int OffsetToFirstLocal = -8,
                     OffsetToFirstParam = 4,
                     OffsetToFirstGlobal = 0;
    static const int VarSize = 4;


    int GetNextLocalLoc();
    int GetNextParamLoc();
    int GetNextGlobalLoc();
    int GetFrameSize();
    void ResetFrameSize();

    static Location* ThisPtr;

    CodeGenerator();



    char *NewLabel();



    Location *GenTempVar();










    Location *GenLoadConstant(int value);
    Location *GenLoadConstant(const char *str);
    Location *GenLoadLabel(const char *label);


    void GenAssign(Location *dst, Location *src);






    void GenStore(Location *addr, Location *val, int offset = 0);








    Location *GenLoad(Location *addr, int offset = 0);





    Location *GenBinaryOp(const char *opName, Location *op1, Location *op2);





    void GenPushParam(Location *param);




    void GenPopParams(int numBytesOfParams);







    Location *GenLCall(const char *label, bool fnHasReturnValue);






    Location *GenACall(Location *fnAddr, bool fnHasReturnValue);










    Location *GenBuiltInCall(BuiltIn b, Location *arg1 = NULL,
            Location *arg2 = NULL);






    void GenIfZ(Location *test, const char *label);
    void GenGoto(const char *label);
    void GenReturn(Location *val = NULL);
    void GenLabel(const char *label);



    BeginFunc *GenBeginFunc();
    void GenEndFunc();






    void GenVTable(const char *className, List<const char*> *methodLabels);







    void DoFinalCodeGen();
};



typedef enum {fpRelative, gpRelative} Segment;

class Location
{
protected:
    const char *variableName;
    Segment segment;
    int offset;
    Location* base;

public:
    Location(Segment seg, int offset, const char *name);
    Location(Segment seg, int offset, const char *name, Location *base);

    const char *GetName() const     { return variableName; }
    Segment GetSegment() const      { return segment; }
    int GetOffset() const           { return offset; }
    Location* GetBase() const       { return base; }


};




class Instruction {
protected:
    char printed[128];

public:
    virtual void EmitSpecific(Mips *mips) = 0;
    void Emit(Mips *mips);
};




class LoadConstant;
class LoadStringLiteral;
class LoadLabel;
class Assign;
class Load;
class Store;
class BinaryOp;
class Label;
class Goto;
class IfZ;
class BeginFunc;
class EndFunc;
class Return;
class PushParam;
class PopParams;
class LCall;
class ACall;
class VTable;

class LoadConstant: public Instruction
{
    Location *dst;
    int val;
public:
    LoadConstant(Location *dst, int val);
    void EmitSpecific(Mips *mips);
};

class LoadStringLiteral: public Instruction
{
    Location *dst;
    char *str;
public:
    LoadStringLiteral(Location *dst, const char *s);
    void EmitSpecific(Mips *mips);
};

class LoadLabel: public Instruction
{
    Location *dst;
    const char *label;
public:
    LoadLabel(Location *dst, const char *label);
    void EmitSpecific(Mips *mips);
};

class Assign: public Instruction
{
    Location *dst, *src;
public:
    Assign(Location *dst, Location *src);
    void EmitSpecific(Mips *mips);
};

class Load: public Instruction
{
    Location *dst, *src;
    int offset;
public:
    Load(Location *dst, Location *src, int offset = 0);
    void EmitSpecific(Mips *mips);
};

class Store: public Instruction
{
    Location *dst, *src;
    int offset;
public:
    Store(Location *d, Location *s, int offset = 0);
    void EmitSpecific(Mips *mips);
};

class BinaryOp: public Instruction
{
public:
    typedef enum {
        Add, Sub, Mul, Div, Mod,
        Eq, Ne, Lt, Le, Gt, Ge,
        And, Or,
        NumOps
    } OpCode;
    static const char * const opName[NumOps];
    static OpCode OpCodeForName(const char *name);

protected:
    OpCode code;
    Location *dst, *op1, *op2;
public:
    BinaryOp(OpCode c, Location *dst, Location *op1, Location *op2);
    void EmitSpecific(Mips *mips);
};

class Label: public Instruction
{
    const char *label;
public:
    Label(const char *label);

    void EmitSpecific(Mips *mips);
    const char* text() const { return label; }
};

class Goto: public Instruction
{
    const char *label;
public:
    Goto(const char *label);
    void EmitSpecific(Mips *mips);
    const char* branch_label() const { return label; }
};

class IfZ: public Instruction
{
    Location *test;
    const char *label;
public:
    IfZ(Location *test, const char *label);
    void EmitSpecific(Mips *mips);
    const char* branch_label() const { return label; }
};

class BeginFunc: public Instruction
{
    int frameSize;
public:
    BeginFunc();

    void SetFrameSize(int numBytesForAllLocalsAndTemps);
    void EmitSpecific(Mips *mips);
};

class EndFunc: public Instruction
{
public:
    EndFunc();
    void EmitSpecific(Mips *mips);
};

class Return: public Instruction
{
    Location *val;
public:
    Return(Location *val);
    void EmitSpecific(Mips *mips);
};

class PushParam: public Instruction
{
    Location *param;
public:
    PushParam(Location *param);
    void EmitSpecific(Mips *mips);
};

class PopParams: public Instruction
{
    int numBytes;
public:
    PopParams(int numBytesOfParamsToRemove);
    void EmitSpecific(Mips *mips);
};

class LCall: public Instruction
{
    const char *label;
    Location *dst;
public:
    LCall(const char *labe, Location *result);
    void EmitSpecific(Mips *mips);
};

class ACall: public Instruction
{
    Location *dst, *methodAddr;
public:
    ACall(Location *meth, Location *result);
    void EmitSpecific(Mips *mips);
};

class VTable: public Instruction
{
    List<const char *> *methodLabels;
    const char *label;
public:
    VTable(const char *labelForTable, List<const char *> *methodLabels);

    void EmitSpecific(Mips *mips);
};





class Mips
{
private:
    typedef enum {
        zero, at, v0, v1, a0, a1, a2, a3,
        s0, s1, s2, s3, s4, s5, s6, s7,
        t0, t1, t2, t3, t4, t5, t6, t7,
        t8, t9, k0, k1, gp, sp, fp, ra, NumRegs
    } Register;

    struct RegContents {
        bool isDirty;
        Location *var;
        const char *name;
        bool isGeneralPurpose;
    } regs[NumRegs];

    Register rs, rt, rd;

    typedef enum { ForRead, ForWrite } Reason;

    void FillRegister(Location *src, Register reg);
    void SpillRegister(Location *dst, Register reg);

    void EmitCallInstr(Location *dst, const char *fn, bool isL);

    static const char *mipsName[BinaryOp::NumOps];
    static const char *NameForTac(BinaryOp::OpCode code);

    Instruction* currentInstruction;

public:
    Mips();

    static void Emit(const char *fmt, ...);

    void EmitLoadConstant(Location *dst, int val);
    void EmitLoadStringLiteral(Location *dst, const char *str);
    void EmitLoadLabel(Location *dst, const char *label);

    void EmitLoad(Location *dst, Location *reference, int offset);
    void EmitStore(Location *reference, Location *value, int offset);
    void EmitCopy(Location *dst, Location *src);

    void EmitBinaryOp(BinaryOp::OpCode code, Location *dst,
                      Location *op1, Location *op2);

    void EmitLabel(const char *label);
    void EmitGoto(const char *label);
    void EmitIfZ(Location *test, const char*label);
    void EmitReturn(Location *returnVal);

    void EmitBeginFunction(int frameSize);
    void EmitEndFunction();

    void EmitParam(Location *arg);
    void EmitLCall(Location *result, const char* label);
    void EmitACall(Location *result, Location *fnAddr);
    void EmitPopParams(int bytes);

    void EmitVTable(const char *label, List<const char*> *methodLabels);

    void EmitPreamble();

    class CurrentInstruction;
};


class Mips::CurrentInstruction
{
public:
    CurrentInstruction(Mips& mips, Instruction* instr) : mips( mips ) {
        mips.currentInstruction= instr;
    }

    ~CurrentInstruction() {
        mips.currentInstruction= NULL;
    }

private:
    Mips& mips;
};

#endif

