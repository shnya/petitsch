// Thanks for MiniScheme and "Three implementation models for scheme"

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <stdexcept>

namespace PetitScheme {
  namespace Base {

    struct cell {
      typedef cell*(*funcp)(cell *, cell *);

      short flag_;
      union _object {
        cell *cell_;
        int ivalue_;
        void *cobj_;
        funcp func_;
        struct {
          cell* car_;
          cell* cdr_;
        } cons_;
        struct {
          const char *str_;
          size_t len_;
        } str_;
      } object_;

      enum CELL_TYPE {
        T_UNKNOWN = 0,
        T_STRING  = 1,
        T_NUMBER = 2,
        T_SYMBOL = 4,
        T_SYNTAX = 8,
        T_PROC = 16,
        T_PAIR = 32,
        T_CLOSURE = 64,
        T_CONTINUATION = 128,
        T_OPCODE = 256,
        T_MARK = 32768
      };

      cell() : flag_(T_UNKNOWN) {}
      cell(const cell &);
      cell &operator=(const cell &);
      ~cell() {
        if(isstring()) delete[] object_.str_.str_;
      }
      cell* init(CELL_TYPE type, int arg)
      { flag_ = type; object_.ivalue_ = arg; return this; }
      cell* init(CELL_TYPE type, const char *arg){
        flag_ = type;
        object_.str_.str_ = strdup(arg);
        object_.str_.len_ = strlen(arg);
        return this;
      }
      cell* init(cell *arg1, cell *arg2){
        flag_ = T_PAIR;
        object_.cons_.car_ = arg1;
        object_.cons_.cdr_ = arg2;
        return this;
      }
      cell* init(cell *arg)
      { flag_ = T_UNKNOWN; object_.cell_ = arg; return this; }
      cell* init(cell* (*arg)(cell *, cell *))
      { flag_ = T_PROC; object_.func_ = arg; return this; }

      bool isunused(){ return flag_ == T_UNKNOWN; }
      bool isopcode(){ return flag_ & T_OPCODE; }
      bool isstring(){ return flag_ & T_STRING; }
      bool isnumber(){ return flag_ & T_NUMBER; }
      bool issymbol(){ return flag_ & T_SYMBOL; }
      bool issyntax(){ return flag_ & T_SYNTAX; }
      bool isproc(){ return flag_ & T_PROC; }
      bool ispair(){ return flag_ & T_PAIR; }
      bool isclosure(){ return flag_ & T_CLOSURE; }
      bool iscontinueation(){ return flag_ & T_CONTINUATION; }
      bool ismark(){return flag_ & T_MARK; }
      void setmark(){ flag_ |= T_MARK; }
      void clrmark(){ flag_ &= (T_MARK - 1); }

      int ivalue(){
        if(!isnumber() && !isopcode()) return 0;
        else return object_.ivalue_;
      }
      const char *str(){
        if(!isstring() && !issymbol()) return empty_string();
        else return object_.str_.str_;
      }
      funcp func(){
        if(!isproc()) return NULL;
        else return object_.func_;
      }
      cell *car(){
        if(!ispair()) return NIL();
        return object_.cons_.car_;
      }
      void car(cell *cell__){
        if(ispair()) object_.cons_.car_ = cell__;
      }
      cell *cdr(){
        if(!ispair()) return NIL();
        else return object_.cons_.cdr_;
      }
      void cdr(cell *cell__){
        if(ispair()) object_.cons_.cdr_ = cell__;
      }
      cell *next_freecell(){
        if(!isunused()) return NULL;
        return object_.cell_;
      }

      static cell* NIL(){
        static cell *NIL_ = NULL;
        if(NIL_ == NULL) NIL_ = new cell();
        return NIL_;
      }
      static cell* T(){
        static cell *T_ = NULL;
        if(T_ == NULL) T_ = new cell();
        return T_;
      }
      static cell* F(){
        static cell *F_ = NULL;
        if(F_ == NULL) F_ = new cell();
        return F_;
      }

      static const char *empty_string()
      {
        static const char *empty_ = "";
        return empty_;
      }
    };

    class cell_manager {
      cell **cells_;
      cell *free_cell;
      size_t capacity_;
      size_t size_;

      cell_manager() : capacity_(1), size_(0) {
        cells_ = new cell*[1];
        free_cell = cells_[0] = new cell();
        cells_[0]->init(cell::NIL());
      }


      void resize(size_t n){
        if(n <= capacity_)
          return;

        size_t newsize = capacity_ * 2;
        while(newsize < n){
          newsize *= 2;
        }

        cell **new_ptr = new cell*[newsize];
        for(size_t i = 0; i < capacity_; i++)
          new_ptr[i] = cells_[i];
        for(size_t i = newsize - 1; i >= capacity_; i--){
          new_ptr[i] = new cell();
          new_ptr[i]->init(free_cell);
          free_cell = new_ptr[i];
        }
        delete[] cells_;
        cells_ = new_ptr;
        capacity_ = newsize;
      }

      ~cell_manager(){
        for(size_t i = 0; i < capacity_; i++)
          delete cells_[i];
        delete[] cells_;
      }

    public:
      static cell_manager& get_instance(){
        static cell_manager *instance = NULL;
        if(instance == NULL){
          instance = new cell_manager();
        }
        return *instance;
      }

      cell *get_cell(){
        size_++;
        if(free_cell == cell::NIL())
          resize(size_);
        cell *ret = free_cell;
        free_cell = free_cell->next_freecell();
        if(free_cell == NULL) throw std::logic_error("Can't allocate memory!");
        return ret;
      }
    };

    void set_car(cell *c, cell *d){ c->car(d); }
    void set_cdr(cell *c, cell *d){ c->cdr(d); }
    cell* car(cell *c){ return c->car(); }
    cell* cdr(cell *c){ return c->cdr(); }
    cell* caar(cell *c){ return car(car(c)); }
    cell* cdar(cell *c){ return cdr(car(c)); }
    cell* cadr(cell *c){ return car(cdr(c)); }
    cell* cddr(cell *c){ return cdr(cdr(c)); }
    cell* caaar(cell *c){ return car(caar(c)); }
    cell* cdaar(cell *c){ return cdr(caar(c)); }
    cell* cadar(cell *c){ return car(cdar(c)); }
    cell* cddar(cell *c){ return cdr(cdar(c)); }
    cell* caadr(cell *c){ return car(cadr(c)); }
    cell* cdadr(cell *c){ return cdr(cadr(c)); }
    cell* caddr(cell *c){ return car(cddr(c)); }
    cell* cdddr(cell *c){ return cdr(cddr(c)); }
    cell* caaaar(cell *c){ return car(caaar(c)); }
    cell* cdaaar(cell *c){ return cdr(caaar(c)); }
    cell* cadaar(cell *c){ return car(cdaar(c)); }
    cell* cddaar(cell *c){ return cdr(cdaar(c)); }
    cell* caadar(cell *c){ return car(cadar(c)); }
    cell* cdadar(cell *c){ return cdr(cadar(c)); }
    cell* caddar(cell *c){ return car(cddar(c)); }
    cell* cdddar(cell *c){ return cdr(cddar(c)); }
    cell* caaadr(cell *c){ return car(caadr(c)); }
    cell* cdaadr(cell *c){ return cdr(caadr(c)); }
    cell* cadadr(cell *c){ return car(cdadr(c)); }
    cell* cddadr(cell *c){ return cdr(cdadr(c)); }
    cell* caaddr(cell *c){ return car(caddr(c)); }
    cell* cdaddr(cell *c){ return cdr(caddr(c)); }
    cell* cadddr(cell *c){ return car(cdddr(c)); }
    cell* cddddr(cell *c){ return cdr(cdddr(c)); }

    cell* cons(cell *a, cell *b)
    { return cell_manager::get_instance().get_cell()->init(a,b); }
    cell* list(cell *a)
    { return cons(a,cell::NIL()); }
    cell* list(cell *a, cell *b)
    { return cons(a,list(b)); }
    cell* list(cell *a, cell *b, cell *c)
    { return cons(a,list(b,c)); }
    cell* list(cell *a, cell *b, cell *c, cell *d)
    { return cons(a,list(b,c,d)); }
    cell* list(cell *a, cell *b, cell *c, cell *d, cell *e)
    { return cons(a,list(b,c,d,e)); }

    cell* mk_proc(cell* (*func)(cell *, cell *)){
      return cell_manager::get_instance().get_cell()->init(func);
    }
    cell* mk_number(int arg){
      return cell_manager::get_instance().get_cell()->init(cell::T_NUMBER, arg);
    }
    cell* mk_opcode(int arg){
      return cell_manager::get_instance().get_cell()->init(cell::T_OPCODE, arg);
    }
    cell* mk_string(const char *arg){
      return cell_manager::get_instance().get_cell()->init(cell::T_STRING, arg);
    }
    cell* mk_symbol(const char *arg){
      return cell_manager::get_instance().get_cell()->init(cell::T_SYMBOL, arg);
    }
    cell* mk_atom(const char *arg){
      char *endptr;
      int n = strtol(arg, &endptr, 0);
      if(endptr != arg && *endptr == '\0'){
        return mk_number(n);
      }else{
        return mk_symbol(arg);
      }
    }
    cell* nreverse(cell *c, bool isdot = false){
      cell *cur = c;
      if(c == cell::NIL()) return c;

      cell *cdr = cur->cdr();
      if(isdot){
        cell *cddr = cdr->cdr();
        cdr->cdr(cur->car());
        cur = cdr;
        cdr = cddr;
      }else{
        cur->cdr(cell::NIL());
      }
      while(cdr != cell::NIL()){
        cell *cddr = cdr->cdr();
        cdr->cdr(cur);
        cur = cdr;
        cdr = cddr;
      }
      return cur;
    }
  }

  typedef Base::cell* obj;
}


namespace PetitScheme
{
  namespace Sexp {
    enum TOKEN_TYPE {
      TOK_LPAREN = 0,
      TOK_RPAREN = 1,
      TOK_DOT = 2,
      TOK_ATOM = 3,
      TOK_QUOTE = 4,
      TOK_COMMENT = 5,
      TOK_DQUOTE = 6,
      TOK_BQUOTE = 7,
      TOK_COMMA = 8,
      TOK_ATMARK = 9,
      TOK_SHARP = 10,
      TOK_STR = 11,
      TOK_EOF = 254,
      TOK_FAIL = 255
    };


    class Token {
      const char *token_str_;
      TOKEN_TYPE type_;

    public:
      TOKEN_TYPE type(){ return type_; }
      const char *str(){ return token_str_; }
      Token(){ token_str_ = new char[1]; }
      Token(TOKEN_TYPE type, char c) : type_(type) {
        char *str = new char[2];
        str[0] = c;
        str[1] = '\0';
        token_str_ = str;
      }
      Token(TOKEN_TYPE type, const char * arg,
            size_t offset, size_t len) : type_(type) {
        char *str = new char[len];
        strncpy(str, arg + offset, len);
        str[len] = '\0';
        token_str_ = str;
      }
      Token(const Token &tok) : type_(tok.type_) {
        size_t len = strlen(tok.token_str_);
        char *str = new char[len];
        strncpy(str, tok.token_str_, len);
        token_str_ = str;
      }

      Token& operator=(const Token &tok){
        if(this == &tok) return *this;
        type_ = tok.type_;
        delete[] token_str_;
        size_t len = strlen(tok.token_str_);
        char *str = new char[len];
        strncpy(str, tok.token_str_, len);
        token_str_ = str;
        return *this;
      }

      ~Token(){
        delete[] token_str_;
      }
    };

    class Tokenizer {
      size_t index_, size_;
      const char *current_;

      bool isdelim(char c, const char *delim){
        size_t len = strlen(delim);
        for(size_t i = 0; i < len + 1; i++){
          if(c == delim[i]) return true;
        }
        return false;
      }

      void skipspace(){
        while(index_ <= size_ && isdelim(current_[index_], " \t\r\n"))
          index_++;
      }

      void skipchar(){
        while(index_ <= size_ && !isdelim(current_[index_], "();\t\r\n "))
          index_++;
      }

    public:
      Tokenizer(const char *str, size_t size)
        : index_(0), size_(size), current_(str) {}

      Token readstrexp(){
        size_t offset = index_;

        while(index_ < size_
              && current_[index_] != '"'
              && current_[index_-1] != '\\') index_++ ;
        int pos = index_ != size_ ? index_ : -1;
        if(pos == -1)
          return Token(TOK_FAIL, '\0');

        return Token(TOK_STR, current_, offset, index_ - offset);
      }

      Token next(){
        skipspace();

        switch (current_[index_++]){
        case '(':
          return Token(TOK_LPAREN, '(');
        case ')':
          return Token(TOK_RPAREN, ')');
        case '.':
          return Token(TOK_DOT, '.');
        case ';':
          while(!isdelim(current_[index_++], "\n")) ;
          return Token(TOK_COMMENT, ';');
        case '"':
          return Token(TOK_DQUOTE, '"');
        case '\'':
          return Token(TOK_QUOTE, '\'');
        case '`':
          return Token(TOK_BQUOTE, '`');
        case ',':
          return Token(TOK_COMMA, ',');
        case '#':
          return Token(TOK_SHARP, '#');
        default:
          size_t offset = index_ - 1;
          skipchar();
          return Token(TOK_ATOM, current_, offset, index_ - offset);
        }
      }
    };


    class Parser {
      Tokenizer tokenizer;
      Base::cell *rparen, *rdot;

      //いつか再帰をなくす予定
      Base::cell *parse_list(){
        obj c, code = Base::cell::NIL();
        while((c = parse_atom()) != rparen){
          if(c == rdot){ //DOT LIST
            c = parse_atom();
            if(c == rparen || parse_atom() != rparen)
              throw std::logic_error("Can't parse sexpression!");

            code = cons(c, code);
            return nreverse(code, true);
          }
          code = cons(c, code);
        }
        return nreverse(code);
      }

      Base::cell *parse_atom(){
        Token tok = tokenizer.next();

        switch(tok.type()){
        case TOK_LPAREN:
          return parse_list();
        case TOK_RPAREN:
          return rparen;
        case TOK_ATOM:
          return Base::mk_atom(tok.str());
        case TOK_DQUOTE:
          tok = tokenizer.readstrexp();
          if(tok.type() == TOK_STR)
            return Base::mk_string(tok.str());
        case TOK_COMMENT:
          return parse_atom();
        case TOK_QUOTE:
          return list(Base::mk_symbol("quote"), parse_atom());
        case TOK_DOT:
          return rdot;
#ifdef FUTURE_FUNCTION
        case TOK_BQUOTE:
        case TOK_COMMA:
        case TOK_ATMARK:
        case TOK_SHARP:
#endif
        default:
          abort();
        }
      }
    public:
      Parser(const char* str, size_t size) : tokenizer(str, size) {}
      Base::cell *parse() {
        return parse_atom();
      };
    };
  }
}


namespace PetitScheme {
  namespace IO {
    class SexpIO {
      std::istream *is;
      std::ostream *os;
      bool failed, eof;

    public:
      SexpIO(){
        is = &std::cin;
        os = &std::cout;
        failed = false;
        eof = false;
      }

      std::string read() {
        if(failed) return "";
        std::string current, line;
        current.clear();
        int paren = 0;
        *os << "sh>> ";
        while(std::getline(*is, line)){
          paren += std::count(line.begin(), line.end(), '(');
          paren -= std::count(line.begin(), line.end(), ')');
          current += line;
          current += '\n';
          if(paren <= 0)
            break;
        }
        if(paren != 0 || !*is) failed = true;
        if(is->eof() && paren == 0) eof = true;

        return current;
      }

      bool isfail() { return failed; }
      bool iseof() { return eof; }
    };
  }
}


using namespace PetitScheme::Base;
using namespace PetitScheme::Sexp;
using namespace PetitScheme::IO;
using namespace std;

namespace PetitScheme {
  namespace VM{

    const char *OP_CODE_STR[14] = {
      "NULL",
      "HALT",
      "REFER",
      "CONSTANT",
      "CLOSE",
      "TEST",
      "ASSIGN",
      "CONTI",
      "NUATE",
      "FRAME",
      "ARGUMENT",
      "APPLY",
      "RETURN",
      "DEFINE"
    };

    void _printsexp(obj code){
      if(code->isopcode()){
        cout << OP_CODE_STR[code->ivalue()];
      }else if(code->isproc()){
        cout << "#" << reinterpret_cast<long>(code->func());
      }else if(code->issymbol()){
        cout << code->str();
      }else if(code->ispair()){
        cout << '(';
        while(1){
          if(!code->ispair()){
            cout << ". ";
            _printsexp(code);
          }else{
            _printsexp(car(code));
          }
          code = cdr(code);
          if(code != cell::NIL())
            cout << ' ';
          else
            break;
        }
        cout << ')';
      }else{
        if(code->isnumber())
          cout << code->ivalue();
        else if(code->isstring())
          cout << code->str();
        else if(code == cell::NIL())
          cout << "()";
      }
    }

    void printsexp(obj code){
      _printsexp(code);
      cout << endl;
    }

    obj OP_ADD(obj arg, obj env){
      int i = 0;
      obj c = arg;
      while(c != cell::NIL()){
        i += car(c)->ivalue();
        c = cdr(c);
      }

      return mk_number(i);
    }

    obj OP_SUB(obj arg, obj env){
      if(arg == cell::NIL())
        return mk_number(0);

      int i = car(arg)->ivalue();
      if(cdr(arg) == cell::NIL())
        return mk_number(-i);

      obj c = cdr(arg);
      while(c != cell::NIL()){
        i -= car(c)->ivalue();
        c = cdr(c);
      }

      return mk_number(i);
    }

    obj OP_MULTIPLY(obj arg, obj env){
      int i = 1;
      obj c = arg;
      while(c != cell::NIL()){
        i *= car(c)->ivalue();
        c = cdr(c);
      }

      return mk_number(i);
    }

    obj OP_DIVIDE(obj arg, obj env){
      if(arg == cell::NIL())
        return mk_number(1);

      int i = car(arg)->ivalue();
      if(cdr(arg) == cell::NIL())
        return mk_number(1/i);

      obj c = cdr(arg);
      while(c != cell::NIL()){
        i /= car(c)->ivalue();
        c = cdr(c);
      }

      return mk_number(i);
    }

    obj OP_CAR(obj arg, obj env){
      return car(car(arg));
    }

    obj OP_CDR(obj arg, obj env){
      return cdr(car(arg));
    }

    obj OP_LIST(obj arg, obj env){
      return arg;
    }

    class VM {
      obj genv;
      enum OP_CODE {
        OP_HALT = 1,
        OP_REFER = 2,
        OP_CONSTANT = 3,
        OP_CLOSE = 4,
        OP_TEST = 5,
        OP_ASSIGN = 6,
        OP_CONTI = 7,
        OP_NUATE = 8,
        OP_FRAME = 9,
        OP_ARGUMENT = 10,
        OP_APPLY = 11,
        OP_RETURN = 12,
        OP_DEFINE = 13
      };

      void define(obj var, obj val){
        genv = cons(cons(list(var), list(val)), genv);
      }

      void define(const char *sym, obj val){
        define(mk_symbol(sym), val);
      }

      void define(const char *sym, int num){
        define(mk_symbol(sym), mk_number(num));
      }

      void define(const char *sym, const char *str){
        define(mk_symbol(sym), mk_string(str));
      }

      void define(const char *sym, cell::funcp func){
        define(mk_symbol(sym), mk_proc(func));
      }

      obj extend(obj env, obj vars, obj vals){
        return cons(cons(vars, vals), env);
      }
      obj call_frame(obj code, obj env, obj rib, obj stack){
        return list(code, env, rib, stack);
      }

      obj closure(obj body, obj env, obj vars){
        return list(body, env, vars);
      }

      obj _lookup(obj var, obj env){
        obj e = env;
        while(e != cell::NIL()){
          obj vars = caar(e);
          obj vals = cdar(e);
          while(vars != cell::NIL()){
            if(strcmp(car(vars)->str(), var->str()) == 0)
              return vals;
            vars = cdr(vars);
            vals = cdr(vals);
          }
          e = cdr(e);
        }
        return cell::NIL();
      }

      obj lookup(obj var, obj env){
        obj found = _lookup(var, env);
        if(found != cell::NIL())
          return found;
        return _lookup(var, genv);
      }

      //いつか再帰をなくす予定
      obj compile(obj code, obj next){
        if(code->issymbol()){
          return list(mk_opcode(OP_REFER), code, next);
        }else if(code->ispair()){
          const char *opcode = car(code)->str();
          if(strcmp(opcode, "quote") == 0){
            return list(mk_opcode(OP_CONSTANT), cadr(code), next);
          }else if(strcmp(opcode, "lambda") == 0){
            return list(mk_opcode(OP_CLOSE),
                        cadr(code),
                        compile(caddr(code), list(mk_opcode(OP_RETURN))),
                        next);
          }else if(strcmp(opcode, "if") == 0){
            return compile(cadr(code),
                           list(mk_opcode(OP_TEST),
                                compile(caddr(code), next),
                                compile(cadddr(code), next)));
          }else if(strcmp(opcode, "set!") == 0){
            return compile(caddr(code),
                           list(mk_opcode(OP_ASSIGN), cadr(code), next));
          }else if(strcmp(opcode, "define") == 0){
            return compile(caddr(code),
                           list(mk_opcode(OP_DEFINE), cadr(code), next));
#ifdef FUTURE_FUNCTION
          }else if(strcmp(opcode, "call/cc") == 0){
#endif
          }else{
            obj c = compile(car(code), list(mk_opcode(OP_APPLY)));
            obj args = cdr(code);
            while(args != cell::NIL()) {
              c = compile(car(args), list(mk_opcode(OP_ARGUMENT), c));
              args = cdr(args);
            }
            if(car(next)->ivalue() == OP_RETURN)
              return c;
            else
              return list(mk_opcode(OP_FRAME), next, c);
          }
        }else{
          return list(mk_opcode(OP_CONSTANT), code, next);
        }
      }

      obj run(obj code){
        obj acc = cell::NIL();
        obj env = cell::NIL();
        obj arg = cell::NIL();
        obj stack = cell::NIL();
      recursion:
#ifdef DEBUG
        cout << "\n";
        cout << "acc\t";  printsexp(acc);
        cout << "code\t";  printsexp(code);
        cout << "env\t"; printsexp(env);
        cout << "genv\t"; printsexp(genv);
        cout << "arg\t"; printsexp(arg);
        cout << "stack\t"; printsexp(stack);
#endif /* DEBUG */
        switch (car(code)->ivalue()){
        case OP_HALT:
          return acc;
        case OP_REFER:
          // var x
          //eval((car (lookup var e)) x e r s)
          acc = car(lookup(cadr(code), env));
          code = caddr(code);
          goto recursion;
        case OP_CONSTANT:
          // x
          //eval(car->cdar() x e r s)
          acc = cadr(code);
          code = caddr(code);
          goto recursion;
        case OP_CLOSE:
          // vars body x
          //eval((closure body e vars) x e r s)
          acc = closure(caddr(code), env, cadr(code));
          code = cadddr(code);
          goto recursion;
        case OP_TEST:
          // then else
          //eval(a (if a then else) e r s)
          if(acc == cell::T())
            code = cadr(code);
          else
            code = caddr(code);
          goto recursion;
        case OP_ASSIGN:
          // var x
          // (set-car! (lookup var e) a)
          // eval(a x e r s)
          set_car(lookup(cadr(code), env), acc);
          code = caddr(code);
          goto recursion;
        case OP_DEFINE:
          define(cadr(code), acc);
          code = caddr(code);
          goto recursion;
#ifdef FUTURE_FUNCTION
        case OP_CONTI:
        case OP_NUTATE:
#endif
        case OP_ARGUMENT:
          // x
          // eval(a x e cons(a r) s)
          arg = cons(acc,arg);
          code = cadr(code);
          goto recursion;
        case OP_FRAME:
          // ret x
          // eval(a x e '() (call-frame ret e r s))
          stack = call_frame(cadr(code), env, arg, stack);
          arg = cell::NIL();
          code = caddr(code);
          goto recursion;
        case OP_APPLY:
          // (record a (body e vars)
          // (apply (lambda (a) eval() ) (body e vars))
          // eval(a body (extend e vars arg) '() s))
          if(acc->isproc()){
            cell::funcp f = acc->func();
            if(f == NULL)
              throw std::logic_error("Can't found this procedure!");
            acc = f(arg, env);
            code = car(stack);
            env = cadr(stack);
            arg = caddr(stack);
            stack = cadddr(stack);
          }else{
            code = car(acc);
            env = extend(cadr(acc), caddr(acc), arg);
            arg = cell::NIL();
          }
          goto recursion;
        case OP_RETURN:
          code = car(stack);
          env = cadr(stack);
          arg = caddr(stack);
          stack = cadddr(stack);
          goto recursion;
        default:
          throw std::logic_error("Evaluation Error");
        }
        return cell::NIL();
      }


    public:
      void repl()
      {
        SexpIO io;
        while(1){
          try{
            string str = io.read();
            if(io.isfail()) break;
            obj code = Parser(str.c_str(), str.size()).parse();
#ifdef DEBUG
            printsexp(code);
#endif
            obj bcode = compile(code, list(mk_opcode(OP_HALT)));
#ifdef DEBUG
            printsexp(bcode);
#endif
            obj ret = run(bcode);
            printsexp(ret);
          }catch(std::exception &e){
            cerr << e.what() << endl;
            break;
          }
          if(io.iseof()) break;
        }
      }

      VM() : genv(cell::NIL()) {
        define("+", OP_ADD);
        define("-", OP_SUB);
        define("*", OP_MULTIPLY);
        define("/", OP_DIVIDE);
        define("list", OP_LIST);
        define("car", OP_CAR);
        define("cdr", OP_CDR);
      }
    } vm;
  }
}

int main(int argc, char *argv[])
{
  PetitScheme::VM::VM().repl();

  return 0;
}
