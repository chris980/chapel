#include <typeinfo>
#include <string.h>
#include "analysis.h"
#include "expr.h"
#include "fa.h"
#include "misc.h"
#include "stmt.h"
#include "stringutil.h"
#include "sym.h"
#include "symscope.h"



static char* cUnOp[NUM_UNOPS] = {
  "+",
  "-",
  "!",
  "~"
};


static char* cBinOp[NUM_BINOPS] = {
  "+",
  "-",
  "*",
  "/",
  "%",
  "==",
  "<=",
  ">=",
  ">",
  "<",
  "!=",
  "&",
  "|",
  "^",
  "<<",
  ">>",
  "&&",
  "||",
  "**",

  " by ",

  "???"
};


static char* cGetsOp[NUM_GETS_OPS] = {
  "=",
  "+=",
  "-=",
  "*=",
  "/=",
  "&=",
  "|=",
  "^=",
  "<<=",
  ">>="
};


// Never change the ordering of this, since it's indexed using the 
// binOpType enumeration.

static precedenceType binOpPrecedence[NUM_BINOPS] = {
  PREC_PLUSMINUS,     // BINOP_PLUS
  PREC_PLUSMINUS,     // BINOP_MINUS
  PREC_MULTDIV,       // BINOP_MULT
  PREC_MULTDIV,       // BINOP_DIV
  PREC_MULTDIV,       // BINOP_MOD
  PREC_EQUALITY,      // BINOP_EQUAL
  PREC_COMPARE,       // BINOP_LEQUAL
  PREC_COMPARE,       // BINOP_GEQUAL
  PREC_COMPARE,       // BINOP_GTHAN
  PREC_COMPARE,       // BINOP_LTHAN
  PREC_EQUALITY,      // BINOP_NEQUAL
  PREC_BITAND,        // BINOP_BITAND
  PREC_BITOR,         // BINOP_BITOR
  PREC_BITXOR,        // BINOP_BITXOR
  PREC_BITS,          // BINOP_BITSL
  PREC_BITS,          // BINOP_BITSR
  PREC_LOGAND,        // BINOP_LOGAND
  PREC_LOGOR,         // BINOP_LOGOR
  PREC_EXP            // BINOP_EXP
};


Expr::Expr(astType_t astType) :
  BaseAST(astType),
  stmt(nilStmt),
  ainfo(NULL),
  parent(nilExpr)
{}


bool Expr::isNull(void) {
  return (this == nilExpr);
}


void Expr::traverse(Traversal* traversal, bool atTop) {
  if (isNull()) {
    return;
  }

  // explore Expr and components
  if (traversal->processTop || !atTop) {
    traversal->preProcessExpr(this);
  }
  if (atTop || traversal->exploreChildExprs) {
    this->traverseExpr(traversal);
  }
  if (traversal->processTop || !atTop) {
    traversal->postProcessExpr(this);
  }
}


void Expr::traverseExpr(Traversal* traversal) {
}


Type* Expr::typeInfo(void) {
  return dtUnknown;
}


bool Expr::isComputable(void) {
  return false;
}


long Expr::intVal(void) {
  INT_FATAL(this, "intVal() called on non-computable/non-int expression");

  return 0;
}


int Expr::rank(void) {
  Type* exprType = this->typeInfo();

  return exprType->rank();
}

precedenceType Expr::precedence(void) {
  return PREC_LOWEST;

}

int
Expr::getTypes(Vec<BaseAST *> &asts) {
  return asts.n;
}


Literal::Literal(astType_t astType, char* init_str) :
  Expr(astType),
  str(copystring(init_str))
{}


bool Literal::isComputable(void) {
  return true;
}


void Literal::print(FILE* outfile) {
  fprintf(outfile, "%s", str);
}


void Literal::codegen(FILE* outfile) {
  fprintf(outfile, "%s", str);
}


IntLiteral::IntLiteral(char* init_str, int init_val) :
  Literal(EXPR_INTLITERAL, init_str),
  val(init_val) 
{}


long IntLiteral::intVal(void) {
  return val;
}


Type* IntLiteral::typeInfo(void) {
  return dtInteger;
}


void IntLiteral::codegen(FILE* outfile) {
  // This is special cased to ensure that we don't generate C octal
  // literals accidentally
  int len = strlen(str);
  int i;
  for (i=0; i<len; i++) {
    if (str[i] != '0') {
      fprintf(outfile, "%s", str+i);
      return;
    }
  }
  fprintf(outfile, "0");
}


FloatLiteral::FloatLiteral(char* init_str, double init_val) :
  Literal(EXPR_FLOATLITERAL, init_str),
  val(init_val) 
{}


StringLiteral::StringLiteral(char* init_val) :
  Literal(EXPR_STRINGLITERAL, init_val)
{}


Type* StringLiteral::typeInfo(void) {
  return dtString;
}


void StringLiteral::print(FILE* outfile) {
  fprintf(outfile, "\"%s\"", str);
}


void StringLiteral::codegen(FILE* outfile) {
  fprintf(outfile, "\"%s\"", str);
}


Variable::Variable(Symbol* init_var) :
  Expr(EXPR_VARIABLE),
  var(init_var) 
{}


void Variable::traverseExpr(Traversal* traversal) {
  var->traverse(traversal, false);
}


Type* Variable::typeInfo(void) {
  return var->type;
}


void Variable::print(FILE* outfile) {
  var->print(outfile);
}

void Variable::codegen(FILE* outfile) {
  var->codegen(outfile);
}


int
Variable::getSymbols(Vec<BaseAST *> &asts) {
  asts.add(var);
  return asts.n;
}


UnOp::UnOp(unOpType init_type, Expr* op) :
  Expr(EXPR_UNOP),
  type(init_type),
  operand(op) 
{
  operand->parent = this;
}


void UnOp::traverseExpr(Traversal* traversal) {
  operand->traverse(traversal, false);
}


void UnOp::print(FILE* outfile) {
  fprintf(outfile, "(%s", cUnOp[type]);
  operand->print(outfile);
  fprintf(outfile, ")");
}


void UnOp::codegen(FILE* outfile) {
  fprintf(outfile, "%s", cUnOp[type]);
  operand->codegen(outfile);
}


Type* UnOp::typeInfo(void) {
  return operand->typeInfo();
}


precedenceType UnOp::precedence(void) {
  return PREC_UNOP;
}


int
UnOp::getExprs(Vec<BaseAST *> &asts) {
  asts.add(operand);
  return asts.n;
}


BinOp::BinOp(binOpType init_type, Expr* l, Expr* r) :
  Expr(EXPR_BINOP),
  type(init_type),
  left(l),
  right(r)
{
  left->parent = this;
  right->parent = this;
}


void BinOp::traverseExpr(Traversal* traversal) {
  left->traverse(traversal, false);
  right->traverse(traversal, false);
}


Type* BinOp::typeInfo(void) {
  Type* leftType = left->typeInfo();
  Type* rightType = right->typeInfo();

  // TODO: This is a silly placeholder until we get type inference
  // hooked in or implement something better here.
  if (leftType != dtUnknown) {
    return leftType;
  } else {
    return rightType;
  }
}

void BinOp::print(FILE* outfile) {
  fprintf(outfile, "(");
  left->print(outfile);
  fprintf(outfile, "%s", cBinOp[type]);
  right->print(outfile);
  fprintf(outfile, ")");
}

void BinOp::codegen(FILE* outfile) {
  precedenceType parentPrecedence = parent->precedence();
  bool parensRequired = this->precedence() <= parentPrecedence;

  bool parensRecommended = false;
  if ((parentPrecedence == PREC_BITOR)  ||
      (parentPrecedence == PREC_BITXOR) ||
      (parentPrecedence == PREC_BITAND) ||
      (parentPrecedence == PREC_LOGOR)) {
    parensRecommended = true;
  }    

  if (parensRequired || parensRecommended) {
    fprintf(outfile, "(");
  }

  // those outer parens really aren't necessary around (pow()) look up a line
  if (type == BINOP_EXP) {
    fprintf(outfile, "pow(");
    left->codegen(outfile);
    fprintf(outfile, ", ");
    right->codegen(outfile);
    fprintf(outfile, ")");
  } else {
    left->codegen(outfile);
    fprintf(outfile, "%s", cBinOp[type]);
    right->codegen(outfile);
  }

  if (parensRequired || parensRecommended) {
    fprintf(outfile, ")");
  } 
}


precedenceType BinOp::precedence(void) {
  return binOpPrecedence[this->type];
}


int
BinOp::getExprs(Vec<BaseAST *> &asts) {
  asts.add(left);
  asts.add(right);
  return asts.n;
}


SpecialBinOp::SpecialBinOp(binOpType init_type, Expr* l, Expr* r) :
  BinOp(init_type, l, r)
{
  astType = EXPR_SPECIALBINOP;
}


precedenceType SpecialBinOp::precedence(void) {
  return PREC_LOWEST;
}


MemberAccess::MemberAccess(Expr* init_base, Symbol* init_member) :
  Expr(EXPR_MEMBERACCESS),
  base(init_base),
  member(init_member)
{}


void MemberAccess::traverseExpr(Traversal* traversal) {
  base->traverse(traversal, false);
  member->traverse(traversal, false);
}


Type* MemberAccess::typeInfo(void) {
  return member->type;
}


void MemberAccess::print(FILE* outfile) {
  base->print(outfile);
  fprintf(outfile, ".");
  member->print(outfile);
}


void MemberAccess::codegen(FILE* outfile) {
  base->codegen(outfile);
  fprintf(outfile, "->");
  member->print(outfile);
}


int
MemberAccess::getExprs(Vec<BaseAST *> &asts) {
  asts.add(base);
  return asts.n;
}


int
MemberAccess::getSymbols(Vec<BaseAST *> &asts) {
  asts.add(member);
  return asts.n;
}


AssignOp::AssignOp(getsOpType init_type, Expr* l, Expr* r) :
  BinOp(BINOP_OTHER, l, r),
  type(init_type)
{
  astType = EXPR_ASSIGNOP;
}


void AssignOp::print(FILE* outfile) {
  left->print(outfile);
  fprintf(outfile, " %s ", cGetsOp[type]);
  right->print(outfile);
}


void AssignOp::codegen(FILE* outfile) {
  left->codegen(outfile);
  fprintf(outfile, " %s ", cGetsOp[type]);
  right->codegen(outfile);
}


precedenceType AssignOp::precedence(void) {
 return PREC_LOWEST;
}


SimpleSeqExpr::SimpleSeqExpr(Expr* init_lo, Expr* init_hi, Expr* init_str) :
  Expr(EXPR_SIMPLESEQ),
  lo(init_lo),
  hi(init_hi),
  str(init_str) 
{}


void SimpleSeqExpr::traverseExpr(Traversal* traversal) {
  lo->traverse(traversal, false);
  hi->traverse(traversal, false);
  str->traverse(traversal, false);
}


Type* SimpleSeqExpr::typeInfo(void) {
  return new DomainType(1);
}


void SimpleSeqExpr::print(FILE* outfile) {
  lo->print(outfile);
  printf("..");
  hi->print(outfile);
  if (str->isComputable() && str->typeInfo() == dtInteger && 
      str->intVal() != 1) {
    printf(" by ");
    str->print(outfile);
  }
}


void SimpleSeqExpr::codegen(FILE* outfile) {
  fprintf(outfile, "This is SimpleSeqExpr's codegen.\n");
}


int
SimpleSeqExpr::getExprs(Vec<BaseAST *> &asts) {
  asts.add(lo);
  asts.add(hi);
  asts.add(str);
  return asts.n;
}


SizeofExpr::SizeofExpr(Type* init_type) :
  Expr(EXPR_SIZEOF),
  type(init_type)
{}


void SizeofExpr::traverseExpr(Traversal* traversal) {
  type->traverse(traversal);
}


Type* SizeofExpr::typeInfo(void) {
  return dtInteger;
}


void SizeofExpr::print(FILE* outfile) {
  fprintf(outfile, "sizeof(");
  type->print(outfile);
  fprintf(outfile, ")");
}


void SizeofExpr::codegen(FILE* outfile) {
  fprintf(outfile, "sizeof(");
  if (typeid(*type) == typeid(ClassType)) {
    fprintf(outfile, "_");
  }
  type->codegen(outfile);
  fprintf(outfile, ")");
}


ParenOpExpr* ParenOpExpr::classify(Expr* base, Expr* arg) {
  if (typeid(*base) == typeid(Variable)) {
    Symbol* baseVar = ((Variable*)base)->var;

    // ASSUMPTION: Anything used before it is defined is a function
    if (typeid(*baseVar) == typeid(UseBeforeDefSymbol) ||
	typeid(*baseVar) == typeid(FnSymbol) ||
	typeid(*baseVar) == typeid(ClassSymbol)) {

      if (baseVar->scope->level == SCOPE_PRELUDE) {
	bool isWrite = (strcmp(baseVar->name, "write") == 0);
	bool isWriteln = (strcmp(baseVar->name, "writeln") == 0);

	if (isWrite || isWriteln) {
	  return new WriteCall(isWriteln, base, arg);
	}
      }

      if (typeid(*baseVar) == typeid(ClassSymbol)) {
	ClassSymbol* classVar = (ClassSymbol*)baseVar;
	ClassType* classType = dynamic_cast<ClassType*>(classVar->type);
	if (!classType) {
	  INT_FATAL(baseVar, "ClassSymbol type is not ClassType");
	}
	base = new Variable(classType->constructor->fn);
      }

      /*
      printf("Found a function call: ");
      base->print(stdout);
      printf("\n");
      */

      return new FnCall(base, arg);
    } else {
      /*
      printf("Found an array ref: ");
      base->print(stdout);
      printf("\n");
      */

      return new ArrayRef(base, arg);
    }
  } else {
    /*
    printf("Found an unknown paren op expr: ");
    base->print(stdout);
    printf("\n");
    */
    
    return new ParenOpExpr(base, arg);
  }
}


ParenOpExpr::ParenOpExpr(Expr* init_base, Expr* init_arg) :
  Expr(EXPR_PARENOP),
  baseExpr(init_base),
  argList(init_arg) 
{}


void ParenOpExpr::traverseExpr(Traversal* traversal) {
  baseExpr->traverse(traversal, false);
  argList->traverseList(traversal, false);
}


void ParenOpExpr::print(FILE* outfile) {
  baseExpr->print(outfile);
  fprintf(outfile, "(");
  if (!argList->isNull()) {
    argList->printList(outfile);
  }
  fprintf(outfile, ")");
}

void ParenOpExpr::codegen(FILE* outfile) {
  baseExpr->codegen(outfile);
  fprintf(outfile, "(");
  if (!argList->isNull()) {
    argList->codegenList(outfile, ", ");
  }
  fprintf(outfile, ")");
}


int
ParenOpExpr::getExprs(Vec<BaseAST *> &asts) {
  asts.add(baseExpr);
  BaseAST *l = argList;
  while (l) {
    asts.add(l);
    l = dynamic_cast<BaseAST *>(l->next);
  }
  return asts.n;
}


CastExpr::CastExpr(Type* init_castType, Expr* init_argList) :
  ParenOpExpr(nilExpr, init_argList),
  castType(init_castType)
{
  astType = EXPR_CAST;
}


void CastExpr::traverseExpr(Traversal* traversal) {
  castType->traverse(traversal, false);
  argList->traverseList(traversal, false);
}


Type* CastExpr::typeInfo(void) {
  return castType;
}


void CastExpr::print(FILE* outfile) {
  castType->print(outfile);
  fprintf(outfile, "(");
  argList->printList(outfile);
  fprintf(outfile, ")");
}


void CastExpr::codegen(FILE* outfile) {
  fprintf(outfile, "(");
  castType->codegen(outfile);
  fprintf(outfile, ")(");
  argList->codegenList(outfile);
  fprintf(outfile, ")");
}


int
CastExpr::getTypes(Vec<BaseAST *> &asts) {
  Expr::getTypes(asts);
  asts.add(castType);
  return asts.n;
}


FnCall::FnCall(Expr* init_base, Expr* init_arg) :
  ParenOpExpr(init_base, init_arg)
{
  astType = EXPR_FNCALL;
}


WriteCall::WriteCall(bool init_writeln, Expr* init_base, Expr* init_arg) :
  FnCall(init_base, init_arg),
  writeln(init_writeln)
{
  astType = EXPR_WRITECALL;
}


Type* WriteCall::typeInfo(void) {
  return dtVoid;
}


void WriteCall::codegen(FILE* outfile) {
  while (argList && !argList->isNull()) {
    Expr* format = nilExpr;
    Expr* arg = argList;

    if (typeid(*arg) == typeid(Tuple)) {
      Tuple* tupleArg = dynamic_cast<Tuple*>(arg);
      format = tupleArg->exprs;
      arg = nextLink(Expr, format);
    }

    Type* argdt = arg->typeInfo();

    fprintf(outfile, "_write");
    if (argdt == dtUnknown) {
      argdt = type_info(this);
      if (argdt == dtUnknown) {
	argdt = dtInteger; // BLC: hack!  Remove once type_info() working
      }
    }
    argdt->codegen(outfile);
    fprintf(outfile, "(");
    fprintf(outfile, "stdout, ");
    if (format->isNull()) {
      argdt->codegenDefaultFormat(outfile);
    } else {
      format->codegen(outfile);
    }
    fprintf(outfile, ", ");
    arg->codegen(outfile);
    fprintf(outfile, ");\n");

    argList = nextLink(Expr, argList);
  }
  if (writeln) {
    fprintf(outfile, "_write_linefeed(stdout);");
  }
}


ArrayRef::ArrayRef(Expr* init_base, Expr* init_arg) :
  ParenOpExpr(init_base, init_arg)
{
  astType = EXPR_ARRAYREF;
}


Type* ArrayRef::typeInfo(void) {
  // The Type of this expression may be a user type in which case we
  // need to walk past these names to the real definition of the array
  // type
  Type* baseExprType = baseExpr->typeInfo();
  while (typeid(*baseExprType) == typeid(UserType)) {
    baseExprType = ((UserType*)baseExprType)->definition;
  }
  // At this point, if we don't have an array type, we shouldn't
  // be in an array reference...
  ArrayType* arrayType = dynamic_cast<ArrayType*>(baseExprType);
  if (!arrayType) {
    INT_FATAL(this, "array type is Null?");
  }
  // BLC: Assumes that each array index fully indexes array.
  // This isn't how David thinks about it, and more care would
  // need to be taken to get that scheme implemented.  Just
  // trying to get something reasonable implemented now.
  return arrayType->elementType;
}


void ArrayRef::codegen(FILE* outfile) {
  fprintf(outfile, "_ACC%d(", baseExpr->rank());
  baseExpr->codegen(outfile);
  fprintf(outfile, ", ");
  argList->codegenList(outfile, ", ");
  fprintf(outfile, ")");
}


FloodExpr::FloodExpr(void) :
  Expr(EXPR_FLOOD)
{}


void FloodExpr::print(FILE* outfile) {
  fprintf(outfile, "*");
}


void FloodExpr::codegen(FILE* outfile) {
  fprintf(outfile, "This is FloodExpr's codegen method.\n");
}


CompleteDimExpr::CompleteDimExpr(void) :
  Expr(EXPR_COMPLETEDIM)
{}


void CompleteDimExpr::print(FILE* outfile) {
  fprintf(outfile, "..");
}

void CompleteDimExpr::codegen(FILE* outfile) {
  fprintf(outfile, "This is CompleteDimExpr's codegen method.\n");
}


DomainExpr::DomainExpr(Expr* init_domains, VarSymbol* init_indices) :
  Expr(EXPR_DOMAIN),
  domains(init_domains),
  indices(init_indices),
  forallExpr(nilExpr)
{}


void DomainExpr::traverseExpr(Traversal* traversal) {
  indices->traverseList(traversal, false);
  domains->traverseList(traversal, false);
  forallExpr->traverse(traversal, false);
}


void DomainExpr::setForallExpr(Expr* exp) {
  forallExpr = exp;
}


Type* DomainExpr::typeInfo(void) {
  Type* exprType = domains->typeInfo();

  if (typeid(*exprType) == typeid(DomainType)) {
    return exprType;
  } else {
    Expr* domainExprs = domains;
    int rank = 0;
    while (domainExprs) {
      rank++;
      domainExprs = nextLink(Expr, domainExprs);
    }
    return new DomainType(rank);
  }
}


void DomainExpr::print(FILE* outfile) {
  fprintf(outfile, "[");
  if (!indices->isNull()) {
    indices->printList(outfile);
    fprintf(outfile, ":");
  }
  domains->printList(outfile);
  fprintf(outfile, "]");
  if (!forallExpr->isNull()) {
    fprintf(outfile, " ");
    forallExpr->print(outfile);
  }
}


void DomainExpr::codegen(FILE* outfile) {
  if (domains->next->isNull()) {
    domains->codegen(outfile);
  } else {
    INT_FATAL(this, "Don't know how to codegen lists of domains yet");
  }
}


int
DomainExpr::getExprs(Vec<BaseAST *> &asts) {
  BaseAST *l = domains;
  while (l) {
    asts.add(l);
    l = dynamic_cast<BaseAST *>(l->next);
  }
  asts.add(forallExpr);
  return asts.n;
}


int
DomainExpr::getSymbols(Vec<BaseAST *> &asts) {
  BaseAST *l = indices;
  while (l) {
    asts.add(l);
    l = dynamic_cast<BaseAST *>(l->next);
  }
  return asts.n;
}


ReduceExpr::ReduceExpr(Symbol* init_reduceType, Expr* init_redDim, 
		       Expr* init_argExpr) :
  Expr(EXPR_REDUCE),
  reduceType(init_reduceType),
  redDim(init_redDim),
  argExpr(init_argExpr)
{}


void ReduceExpr::traverseExpr(Traversal* traversal) {
  reduceType->traverse(traversal, false);
  redDim->traverseList(traversal, false);
  argExpr->traverse(traversal, false);
}


void ReduceExpr::print(FILE* outfile) {
  fprintf(outfile, "reduce ");
  if (!redDim->isNull()) {
    fprintf(outfile, "(dim=");
    redDim->print(outfile);
    fprintf(outfile, ") ");
  }
  fprintf(outfile, "by ");
  reduceType->print(outfile);
  fprintf(outfile, " ");
  argExpr->print(outfile);
}

void ReduceExpr::codegen(FILE* outfile) {
  fprintf(outfile, "This is ReduceExpr's codegen method.\n");
}


int
ReduceExpr::getSymbols(Vec<BaseAST *> &asts) {
  asts.add(reduceType);
  return asts.n;
}


int
ReduceExpr::getExprs(Vec<BaseAST *> &asts) {
  asts.add(redDim);
  asts.add(argExpr);
  return asts.n;
}


Tuple::Tuple(Expr* init_exprs) :
  Expr(EXPR_TUPLE),
  exprs(init_exprs)
{}


void Tuple::traverseExpr(Traversal* traversal) {
  Expr* expr = exprs;
  while (expr) {
    expr->traverse(traversal, false);

    expr = nextLink(Expr, expr);
  }
}


void Tuple::print(FILE* outfile) {
  fprintf(outfile, "(");
  Expr* expr = exprs;
  while (expr) {
    expr->print(outfile);

    expr = nextLink(Expr, expr);
    if (expr) {
      fprintf(outfile, ", ");
    }
  }
  fprintf(outfile, ")");
}


void Tuple::codegen(FILE* outfile) {
  INT_FATAL(this, "can't codegen tuples yet");
}


int
Tuple::getExprs(Vec<BaseAST *> &asts) {
  BaseAST *l = exprs;
  while (l) {
    asts.add(l);
    l = dynamic_cast<BaseAST *>(l->next);
  }
  return asts.n;
}


DomainExpr* unknownDomain;

void initExpr(void) {
  Symbol* pst = new Symbol(SYMBOL, "?anonDomain");
  Variable* var = new Variable(pst);
  unknownDomain = new DomainExpr(var);
}
