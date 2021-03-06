// Thanks for MiniScheme and "Three implementation models for scheme"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <setjmp.h>


namespace PetitScheme {
  namespace Base {


    class cell {
    public:
      typedef cell*(*funcp)(cell *, cell *);

    private:
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
          char *str_;
          size_t len_;
        } str_;
      } object_;


      cell(const cell &_cell);
    public:

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

      static cell *NIL,*T,*F;
      static cell ___NIL___,___T___,___F___;


      cell() : flag_(T_UNKNOWN) {}
      ~cell() {
        if(isstring() || issymbol() || issyntax()) free(object_.str_.str_);
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
      cell* init()
      { flag_ = T_UNKNOWN; return this; }
      cell* init(cell* (*arg)(cell *, cell *))
      { flag_ = T_PROC; object_.func_ = arg; return this; }
      cell* init(cell *arg){
        this->clear();
        flag_ = arg->flag_;
        if(isunused()){
          // do nothing;
        }else if(ispair()){
          object_.cons_.car_ = arg->object_.cons_.car_;
          object_.cons_.cdr_ = arg->object_.cons_.cdr_;
        }else if(isproc()){
          object_.func_ = arg->object_.func_;
        }else if(issymbol() || isstring() || issyntax()){
          object_.str_.str_ = strdup(arg->object_.str_.str_);
          object_.str_.len_ = arg->object_.str_.len_;
        }else if(isopcode() || isnumber()){
          object_.ivalue_ = arg->object_.ivalue_;
        }else{
          throw std::logic_error("unknown type");
        }
        return this;
      }

      cell* connect(cell *arg)
      { flag_ = T_UNKNOWN; object_.cell_ = arg; return this; }

      bool isunused() const { return flag_ == T_UNKNOWN; }
      bool isopcode() const { return flag_ & T_OPCODE; }
      bool isstring() const { return flag_ & T_STRING; }
      bool isnumber() const { return flag_ & T_NUMBER; }
      bool issymbol() const { return flag_ & T_SYMBOL; }
      bool issyntax() const { return flag_ & T_SYNTAX; }
      bool isproc() const { return flag_ & T_PROC; }
      bool ispair() const { return flag_ & T_PAIR; }
      bool isclosure() const { return flag_ & T_CLOSURE; }
      bool iscontinuation() const { return flag_ & T_CONTINUATION; }
      bool ismarked() const {return flag_ & T_MARK; }
      bool issametype(cell *a) const { return flag_ == a->flag_; }
      void setmark(){ flag_ |= T_MARK; }
      void clrmark(){ flag_ &= (T_MARK - 1); }

      int ivalue() const {
        if(!isnumber() && !isopcode()) return 0;
        else return object_.ivalue_;
      }
      const char *str() const {
        if(!isstring() && !issymbol()) return "";
        else return object_.str_.str_;
      }
      funcp func() const {
        if(!isproc()) return NULL;
        else return object_.func_;
      }
      cell *car() const {
        if(!ispair()) return NIL;
        return object_.cons_.car_;
      }
      void car(cell *cell__){
        if(ispair()) object_.cons_.car_ = cell__;
      }
      cell *cdr() const {
        if(!ispair()) return NIL;
        return object_.cons_.cdr_;
      }
      void cdr(cell *cell__){
        if(ispair()) object_.cons_.cdr_ = cell__;
      }
      cell *next_freecell() const {
        return object_.cell_;
      }

      void clear(){
        if(isstring() || issymbol() || issyntax()) free(object_.str_.str_);
        flag_ = T_UNKNOWN;
      }

      void dump(){
        printf("addr %p type: ", reinterpret_cast<void *>(this));
        if(isunused()){
          printf("unknown; next_freecell=\"%p\"", reinterpret_cast<void *>(object_.cell_));
        }else if(isstring()){
          printf("string; value=\"%s\"", object_.str_.str_);
        }else if(isnumber()){
          printf("number; value=\"%d\"", object_.ivalue_);
        }else if(issymbol()){
          printf("symbol; value=\"%s\"", object_.str_.str_);
        }else if(issyntax()){
          printf("syntax; value=\"%s\"", object_.str_.str_);
        }else if(isproc()){
          printf("proc; func_addr=\"%p\"", reinterpret_cast<void*>(reinterpret_cast<unsigned long>(object_.func_)));
        }else if(ispair()){
          printf("pair; car=\"%p\",cdr=\"%p\"",
                 reinterpret_cast<void *>(object_.cons_.car_),
                 reinterpret_cast<void *>(object_.cons_.cdr_));
        }else if(isclosure()){
        }else if(iscontinuation()){
        }else if(isopcode()){
          printf("opcode; value=\"%d\"", object_.ivalue_);
        }
        printf("\n");
      }
    };

    class cell_manager {
      struct cell_block {
        int size_;
        cell *cells_;
        cell *free_cell_;

        cell_block(int size = 512) : size_(size) {
          if(size == 0) return;
          cells_ = new cell[size_];
          connect_freecell();
        }

        void connect_freecell(){
          free_cell_ = cell::NIL;
          for(int i = size_ - 1; i >= 0; i--){
            if(cells_[i].isunused()){
              cells_[i].connect(free_cell_);
              free_cell_ = &(cells_[i]);
            }
          }
        }

        void dump(){
          for(int i = 0; i < size_; i++){
            cells_[i].dump();
          }
        }

        void sweep(){
          for(int i = 0; i < size_; i++){
            if(cells_[i].ismarked()){
              cells_[i].clrmark();
            }else{
#ifdef DEBUG
              printf("sweeped %p", &cells_[i]);
              cells_[i].dump();
#endif /* DEBUG */
              cells_[i].clear();
            }
          }
          connect_freecell();
        }

        void mark_cell(cell *bemarked){
#ifdef DEBUG
          bemarked->dump();
#endif /* DEBUG */
          if(bemarked == cell::NIL || bemarked == cell::F
             || bemarked == cell::T) return;
          if(bemarked->ismarked()) return;
          bemarked->setmark();
          if(bemarked->ispair()){
            mark_cell(bemarked->car());
            mark_cell(bemarked->cdr());
          }
        }

        void mark_register(cell **registers, int n){
          cell **register_ptr = registers;
          cell *heap_begin = cells_;
          cell *heap_end = heap_begin + size_;
#ifdef DEBUG
          printf("NIL: %p, T: %p, F: %p\n", cell::NIL,cell::T,cell::F);
          printf("heap_begin: %p, heap_end: %p\n", heap_begin, heap_end);
#endif /* DEBUG */
          for(int i = 0; i < n; i++){
            if(heap_begin <= *register_ptr && heap_end > *register_ptr){
#ifdef DEBUG
              printf("found register %p <= %p < %p \n",
                     heap_begin, *register_ptr, heap_end);
#endif /* DEBUG */
              for(int i = 0; i < size_; i++)
                if(*register_ptr == &(cells_[i]))
                  mark_cell(*register_ptr);
            }
            register_ptr++;
#ifdef DEBUG
            printf("register survey %p <= %p, %p\n", registers,
                   register_ptr, *register_ptr);
#endif /* DEBUG */
          }
        }

        void mark_stack(cell **stack_top, cell **stack_end){
          if(stack_top > stack_end){
            cell **tmp = stack_top;
            stack_top = stack_end;
            stack_end = tmp;
          }
          cell **stack_ptr = stack_top;
          cell *heap_begin = cells_;
          cell *heap_end = heap_begin + size_;
#ifdef DEBUG
          dump();
          printf("NIL: %p, T: %p, F: %p\n", cell::NIL,cell::T,cell::F);
          printf("heap_begin: %p, heap_end: %p\n", heap_begin, heap_end);
#endif /* DEBUG */
          while(stack_ptr < stack_end){
            if(heap_begin <= *stack_ptr && *stack_ptr < heap_end){
#ifdef DEBUG
              printf("found stack %p <= %p < %p \n",
                     heap_begin, *stack_ptr, heap_end);
#endif /* DEBUG */
              for(int i = 0; i < size_; i++)
                if(*stack_ptr == &(cells_[i]))
                  mark_cell(*stack_ptr);
            }
            stack_ptr++;
#ifdef DEBUG
            printf("stack survey %p <= %p < %p, %p\n",
                   stack_top, stack_ptr, stack_end, *stack_ptr);
#endif /* DEBUG */
          }
        }

        cell *get_cell(){
          if(free_cell_ == cell::NIL) return free_cell_;
          cell *ret = free_cell_;
          free_cell_ = free_cell_->next_freecell();
          return ret;
        }

        ~cell_block(){
          delete[] cells_;
        }
      };

      cell_block** blocks_;
      int block_siz_;
      cell **stack_top_;
      cell **stack_end_;

      cell_manager() : block_siz_(1) {
        blocks_ = new cell_block*[1];
        blocks_[0] = new cell_block();
      }

      ~cell_manager(){
        for(int i = 0; i < block_siz_; i++)
          delete blocks_[i];
        delete[] blocks_;
        delete cell::NIL;
        delete cell::T;
        delete cell::F;
      }

      void append_block(){
        cell_block **new_blocks = new cell_block*[block_siz_+1];
        for(int i = 0; i < block_siz_; i++)
          new_blocks[i] = blocks_[i];
        new_blocks[block_siz_] = new cell_block();
        delete[] blocks_;
        blocks_ = new_blocks;
        block_siz_++;
      }

      cell *search_cell(){
        for(int i = 0; i < block_siz_; i++){
          cell *ret = blocks_[i]->get_cell();
          if(ret != cell::NIL) return ret;
        }
        return cell::NIL;
      }

    public:
      static cell_manager& get_instance(){
        static cell_manager *instance = NULL;
        if(instance == NULL){
          instance = new cell_manager();
        }
        return *instance;
      }

      void set_stack_top(cell **stack_top){
        stack_top_ = stack_top;
      }

      cell *get_cell(){
        cell *ret;
        if((ret = search_cell()) != cell::NIL) return ret;
        gc();
        if((ret = search_cell()) != cell::NIL) return ret;
        append_block();
        ret = blocks_[block_siz_-1]->get_cell();
        if(ret == cell::NIL) throw std::logic_error("Can't allocate memory");
        return ret;
      }

      cell *clone(cell *_cell){
        if(_cell->ispair()){
          return get_cell()->init(clone(_cell->car()),clone(_cell->cdr()));
        }else{
          return get_cell()->init(_cell);
        }
      }

      void gc(){
        jmp_buf registers;
        setjmp(registers);
        cell* end;
        stack_end_ = &end;

#ifdef DEBUG
        printf("stack top is %p, stack end is %p\n", stack_top_, stack_end_);
#endif /* DEBUG */
        for(int i = 0; i < block_siz_; i++){
          blocks_[i]->mark_register(reinterpret_cast<cell **>(registers),
                                    sizeof(registers) / sizeof(cell *));
          blocks_[i]->mark_stack(stack_top_, stack_end_);
        }
        // sweep
        for(int i = 0; i < block_siz_; i++){
          blocks_[i]->sweep();
        }
      }
    };

    cell cell::___NIL___;
    cell cell::___T___;
    cell cell::___F___;
    cell* cell::NIL = &___NIL___;
    cell* cell::T = &___T___;
    cell* cell::F = &___F___;
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
    { return cons(a,cell::NIL); }
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
      if(c == cell::NIL) return c;

      cell *cdr = cur->cdr();
      if(isdot){
        cell *cddr = cdr->cdr();
        cdr->cdr(cur->car());
        cur = cdr;
        cdr = cddr;
      }else{
        cur->cdr(cell::NIL);
      }
      while(cdr != cell::NIL){
        cell *cddr = cdr->cdr();
        cdr->cdr(cur);
        cur = cdr;
        cdr = cddr;
      }
      return cur;
    }
    cell* append(cell *left, cell *right){
      if(left->ispair()){
        return cons(car(left),append(cdr(left),right));
      }else{
        return right;
      }
    }
    bool equal(cell *left, cell *right){
      if(left->issametype(right)){
        if(left->isunused()){
          if(left == right) // T == T || NIL == NIL || F == F
            return true;
          else
            return false;
        }else if(left->ispair()){
          return equal(car(left),car(right))
            && equal(cdr(left),cdr(right));
        }else if(left->isproc()){
          return left->func() == right->func();
        }else if(left->issymbol() || left->isstring() || left->issyntax()){
          return strcmp(left->str(),right->str()) == 0;
        }else if(left->isopcode() || left->isnumber()){
          return left->ivalue() == right->ivalue();
        }else{
          throw std::logic_error("unknown type in equal comparision");
        }
      }
      return false;
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
      TOK_COMMA_AT = 9,
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

        size_t offset;
        switch (current_[index_++]){
        case '(':
          return Token(TOK_LPAREN, '(');
        case ')':
          return Token(TOK_RPAREN, ')');
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
          if(current_[index_] == '@')
            return Token(TOK_COMMA_AT, current_, index_++-1, 2);
          else
            return Token(TOK_COMMA, ',');
        case '#':
          offset = index_ - 1;
          skipchar();
          return Token(TOK_SHARP, current_, offset, index_ - offset);
        case '.':
          if(isdelim(current_[index_], " \t\r\n"))
            return Token(TOK_DOT, '.');
        default:
          offset = index_ - 1;
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
        obj c, code = Base::cell::NIL;
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
        case TOK_BQUOTE:
          return list(Base::mk_symbol("quasiquote"), parse_atom());
        case TOK_COMMA:
          return list(Base::mk_symbol("unquote"), parse_atom());
        case TOK_COMMA_AT:
          return list(Base::mk_symbol("unquote-splicing"), parse_atom());
        case TOK_SHARP:
          if(strcmp(tok.str(),"#t") == 0)
            return Base::cell::T;
          else if(strcmp(tok.str(),"#f") == 0)
            return Base::cell::F;
          else
            return Base::mk_symbol(tok.str());
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
        *os << "petitsch>> ";
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

    const char *OP_CODE_STR[] = {
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
          if(code != cell::NIL)
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
        else if(code == cell::NIL)
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
      while(c != cell::NIL){
        i += car(c)->ivalue();
        c = cdr(c);
      }

      return mk_number(i);
    }

    obj OP_SUB(obj arg, obj env){
      if(arg == cell::NIL)
        return mk_number(0);

      int i = car(arg)->ivalue();
      if(cdr(arg) == cell::NIL)
        return mk_number(-i);

      obj c = cdr(arg);
      while(c != cell::NIL){
        i -= car(c)->ivalue();
        c = cdr(c);
      }

      return mk_number(i);
    }

    obj OP_MULTIPLY(obj arg, obj env){
      int i = 1;
      obj c = arg;
      while(c != cell::NIL){
        i *= car(c)->ivalue();
        c = cdr(c);
      }

      return mk_number(i);
    }

    obj OP_DIVIDE(obj arg, obj env){
      if(arg == cell::NIL)
        return mk_number(1);

      int i = car(arg)->ivalue();
      if(cdr(arg) == cell::NIL)
        return mk_number(1/i);

      obj c = cdr(arg);
      while(c != cell::NIL){
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

    obj OP_LIST_ASTA_ASTA(obj arg, obj env){
      printsexp(arg);
      obj ret = cell::NIL;
      while(arg != cell::NIL){
        ret = append(ret, car(arg));
        arg = cdr(arg);
      }
      return ret;
    }

    obj OP_BEGIN(obj arg, obj env){
      while(cdr(arg) != cell::NIL)
        arg = cdr(arg);
      return car(arg);
    }

    obj OP_DISPLAY(obj arg, obj env){
      printsexp(car(arg));
      return cell::NIL;
    }

    obj OP_EQUAL(obj arg, obj env){
      if(car(arg)->ivalue() == cadr(arg)->ivalue()){
        return cell::T;
      }else{
        return cell::F;
      }
    }

    class VM {
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

      void define(obj var, obj val, obj *genv){
        *genv = cons(cons(list(var), list(val)), *genv);
      }

      void define(const char *sym, obj val, obj *genv){
        define(mk_symbol(sym), val, genv);
      }

      void define(const char *sym, int num, obj *genv){
        define(mk_symbol(sym), mk_number(num), genv);
      }

      void define(const char *sym, const char *str,obj* genv){
        define(mk_symbol(sym), mk_string(str), genv);
      }

      void define(const char *sym, cell::funcp func, obj* genv){
        define(mk_symbol(sym), mk_proc(func), genv);
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
        while(e != cell::NIL){
          obj vars = caar(e);
          obj vals = cdar(e);
          while(vars != cell::NIL){
            if(strcmp(car(vars)->str(), var->str()) == 0)
              return vals;
            vars = cdr(vars);
            vals = cdr(vals);
          }
          e = cdr(e);
        }
        return cell::NIL;
      }

      obj lookup(obj var, obj env, obj *genv){
        obj found = _lookup(var, env);
        if(found != cell::NIL)
          return found;
        return _lookup(var, *genv);
      }

      obj assoc_lookup(obj lst, obj key_obj){
        const char *key = key_obj->str();
        while(lst != cell::NIL){
          if(strcmp(caar(lst)->str(),key) == 0){
            return cdar(lst);
          }
          lst = cdr(lst);
        }
        return cell::NIL;
      }

      bool reserved_lookup(obj reserved, obj key_obj){
        const char *key = key_obj->str();
        while(reserved != cell::NIL){
          if(strcmp(car(reserved)->str(),key) == 0)
            return true;
          reserved = cdr(reserved);
        }
        return false;
      }

      obj pattern_match(obj templ, obj real, obj reserved){
        obj res = cell::NIL;
        while(templ != cell::NIL){
#ifdef DEBUG
          cout << "templ: "; printsexp(templ);
          cout << "real: "; printsexp(real);
#endif
          if(real == cell::NIL){
            return NULL;
          }else if(strcmp(cadr(templ)->str(), "...") == 0){
            obj templ_orig = templ, real_orig = real;
            while(real != cell::NIL){
              obj ret = NULL;
              if(car(templ)->ispair()){
                ret = pattern_match(car(templ),car(real),reserved);
              }else{
                ret = pattern_match(list(car(templ)),list(car(real)),reserved);
              }
              if(ret == NULL) return NULL;
              real = cdr(real);
            }
            while(cdr(templ) != cell::NIL){
              templ = cdr(templ);
              if(strcmp(car(templ)->str(), "...") != 0)
                return NULL;
            }
            return cons(cons(templ_orig,real_orig),res);
          }else if(car(templ)->ispair()){
            if(!car(real)->ispair()){
              return NULL;
            }
            obj ret = pattern_match(car(templ), car(real), reserved);
            if(ret == NULL) return NULL;
            res = append(ret,res);
          }else{
            if(strcmp(car(templ)->str(), "_") == 0) goto skip;
            if(reserved_lookup(reserved,car(templ))){
              if(strcmp(car(templ)->str(),car(real)->str()) == 0){
                goto skip;
              }else{
                return NULL;
              }
            }
            if(car(templ) != cell::NIL && car(real) != cell::NIL){
              res = cons(cons(car(templ),car(real)),res);
            }else{
              return NULL;
            }
          }
        skip:
          if(!cdr(templ)->ispair() && cdr(templ) != cell::NIL){ //for dot list
            obj ret = pattern_match(cdr(templ),cdr(real),reserved);
            if(ret == NULL) return NULL;
            res = append(ret,res);
            templ = cdr(templ);
            real = cdr(templ);
          }
          templ = cdr(templ);
          real = cdr(real);
        }
        if(real != cell::NIL) return NULL;
        return res;
      }

      obj bind_lookup(obj bind, obj *key_objp){
        if((*key_objp)->ispair()){
          while(bind != cell::NIL){
            obj templ = caar(bind);
            obj key = *key_objp;
#ifdef DEBUG
            cout << "templ_bind: "; printsexp(templ);
            cout << "key: "; printsexp(key);
#endif
            if(!caar(bind)->ispair()) goto skip;
            if(!equal(car(templ),car(key))) goto skip;
            while(cdr(templ) != cell::NIL){
              templ = cdr(templ); key = cdr(key);
              // 同数かチェック
              if(key == cell::NIL) goto skip;
              if(strcmp(car(key)->str(),"...")  != 0) goto skip;
            }
            *key_objp = cdr(key);
            return cdar(bind);
          skip:
            bind = cdr(bind);
          }
          return NULL;
        }else{
          const char *key = (*key_objp)->str();
          while(bind != cell::NIL){
            if(caar(bind)->ispair())
              goto skip2;
            if(strcmp(caar(bind)->str(),key) == 0){
              return cdar(bind);
            }
          skip2:
            bind = cdr(bind);
          }
          return NULL;
        }
      }

      obj macro_expand(obj bind, obj templ){
#ifdef DEBUG
        cout << "templ: "; printsexp(templ);
#endif
        if(templ->ispair()){
          if(strcmp(cadr(templ)->str(),"...") == 0){
            obj val = bind_lookup(bind,&templ);
            if(val != NULL){
              return append(val,macro_expand(bind,templ));
            }
          }
          return cons(macro_expand(bind,car(templ)),
                      macro_expand(bind,cdr(templ)));
        }else{
          obj val = bind_lookup(bind,&templ);
          if(val != NULL){
#ifdef DEBUG
            cout << "binded: key: " << templ->str();
            cout << "val: "; printsexp(val);
#endif
            return val;
          }else{
            return templ;
          }
        }
      }

      obj quasiquote(obj quoted){
        if(quoted->ispair()){
          obj ret = cell::NIL;
          while(quoted != cell::NIL){
            if(car(quoted)->ispair()){
              if(strcmp(caar(quoted)->str(),"unquote") == 0){
                ret = cons(list(mk_symbol("list"),cadar(quoted)),ret);
              }else if(strcmp(caar(quoted)->str(), "unquote-splicing") == 0){
                ret = cons(cadar(quoted),ret);
              }else{
                ret = cons(list(mk_symbol("list"),quasiquote(car(quoted))),ret);
              }
            }else{
              ret = cons(list(mk_symbol("list"),quasiquote(car(quoted))),ret);
            }
            quoted = cdr(quoted);
          }
          return cons(mk_symbol("list**"),nreverse(ret));
        }else{
          return list(mk_symbol("quote"), quoted);
        }
      }

      //いつか再帰をなくす予定
      obj compile(obj code, obj next, obj *syntax){
        if(code->issymbol()){
          return list(mk_opcode(OP_REFER), code, next);
        }else if(code->ispair()){
          const char *opcode = car(code)->str();
          obj matched_syntax;
          if(strcmp(opcode, "quote") == 0){
            return list(mk_opcode(OP_CONSTANT), cadr(code), next);
          }else if(strcmp(opcode, "quasiquote") == 0){
            //printsexp(quasiquote(cadr(code)));
            return compile(quasiquote(cadr(code)), next, syntax);
          }else if(strcmp(opcode, "lambda") == 0){
            obj body = list(mk_opcode(OP_RETURN));
            obj body_exps = cddr(code);
            body_exps = nreverse(body_exps);
            while(body_exps != cell::NIL){
              body = compile(car(body_exps),body, syntax);
              body_exps = cdr(body_exps);
            }
            return list(mk_opcode(OP_CLOSE), cadr(code),
                        body, next);
          }else if(strcmp(opcode, "if") == 0){
            return compile(cadr(code),
                           list(mk_opcode(OP_TEST),
                                compile(caddr(code), next, syntax),
                                compile(cadddr(code), next, syntax)), syntax);
          }else if(strcmp(opcode, "set!") == 0){
            return compile(caddr(code),
                           list(mk_opcode(OP_ASSIGN), cadr(code), next),
                           syntax);
          }else if(strcmp(opcode, "define") == 0){
            if(cadr(code)->ispair()){
              return compile(cons(mk_symbol("lambda"),
                                  cons(cdadr(code), cddr(code))),
                             list(mk_opcode(OP_DEFINE), caadr(code), next),
                             syntax);
            }else{
              return compile(caddr(code),
                             list(mk_opcode(OP_DEFINE), cadr(code), next),
                             syntax);
            }
          }else if(strcmp(opcode, "call/cc") == 0){
            obj c = list(mk_opcode(OP_CONTI),
                         list(mk_opcode(OP_ARGUMENT),
                              compile(cadr(code), list(mk_opcode(OP_APPLY)),
                                      syntax)));
            if(car(next)->ivalue() == OP_RETURN)
              return c;
            else
              return list(mk_opcode(OP_FRAME), next, c);
          }else if(strcmp(opcode, "define-syntax") == 0){
            obj name = cadr(code);
            obj transformer = caddr(code);
            *syntax = cons(cons(name,transformer),*syntax);
            return next;
          }else if((matched_syntax = assoc_lookup(*syntax, car(code)))
                   != cell::NIL){
            //printsexp(matched_syntax);
            if(strcmp(car(matched_syntax)->str(),"syntax-rules") == 0){
              obj reserved = cadr(matched_syntax);
              obj patterns = cddr(matched_syntax);
              obj bind = NULL;
              while(patterns != cell::NIL){
                bind = pattern_match(caar(patterns),code,
                                     cons(car(code),reserved));
                if(bind != NULL) break;
                patterns = cdr(patterns);
              }
              if(bind == NULL) throw logic_error("not match macro");
#ifdef DEBUG
              cout << "binded: "; printsexp(bind);
#endif
              obj expanded = macro_expand(bind, cadar(patterns));
#ifdef DEBUG
              cout << "expanded: "; printsexp(expanded);
#endif
              return compile(expanded, next, syntax);
            }else{
              throw logic_error("not implemented other macro syntax rule");
            }
          }else{
            obj c = compile(car(code), list(mk_opcode(OP_APPLY)),syntax);
            obj args = cdr(code);
            args = nreverse(args);
            while(args != cell::NIL) {
              c = compile(car(args), list(mk_opcode(OP_ARGUMENT), c),syntax);
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

      obj run(obj code, obj *genv){
        obj acc = cell::NIL;
        obj env = cell::NIL;
        obj arg = cell::NIL;
        obj stack = cell::NIL;
      recursion:
#ifdef DEBUG
        cout << "\n";
        cout << "acc\t";  printsexp(acc);
        cout << "code\t";  printsexp(code);
        cout << "env\t"; printsexp(env);
        cout << "genv\t"; printsexp(*genv);
        cout << "arg\t"; printsexp(arg);
        cout << "stack\t"; printsexp(stack);
#endif /* DEBUG */
        switch (car(code)->ivalue()){
        case OP_HALT:
          return acc;
        case OP_REFER:
          // var x
          //eval((car (lookup var e)) x e r s)
          acc = car(lookup(cadr(code), env, genv));
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
          if(acc == cell::T)
            code = cadr(code);
          else
            code = caddr(code);
          goto recursion;
        case OP_ASSIGN:
          // var x
          // (set-car! (lookup var e) a)
          // eval(a x e r s)
          set_car(lookup(cadr(code), env, genv), acc);
          code = caddr(code);
          goto recursion;
        case OP_DEFINE:
          if(env == cell::NIL)
            define(cadr(code), acc, genv);
          else
            extend(env, cadr(code), acc);
          acc = cadr(code);
          code = caddr(code);
          goto recursion;
        case OP_CONTI:
          acc = closure(list(mk_opcode(OP_NUATE), stack,
                             mk_symbol("#<continuation arg>")),
                        cell::NIL,
                        list(mk_symbol("#<continuation arg>")));
          code = cadr(code);
          goto recursion;
        case OP_NUATE:
          acc = car(lookup(caddr(code), env, genv));
          stack = cadr(code);
          code = list(mk_opcode(OP_RETURN));
          goto recursion;
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
          arg = cell::NIL;
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
            acc = f(nreverse(arg), env);
            code = car(stack);
            env = cadr(stack);
            arg = caddr(stack);
            stack = cadddr(stack);
          }else{
            code = car(acc);
            if(code == cell::NIL)
              throw std::logic_error("It's not defined function!");
            env = extend(cadr(acc), caddr(acc), nreverse(arg));
            arg = cell::NIL;
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
        return cell::NIL;
      }


    public:
      void repl()
      {
        obj stack_top = NULL;
        cell_manager::get_instance().set_stack_top(&stack_top);

        SexpIO io;
        obj genv = cell::NIL;
        genv_init(&genv);
        obj syntax = cell::NIL;
        while(1){
          try{
#ifdef DEBUG
            //string str = io.read();
            string str = "`(3 ,(list 3 5))";
            //string str = "((lambda (a) (a a)) (lambda (a) (display 1) (a a)))";
            //string str = "(define a (lambda () (display 1) (a)))\n (a)";
            /*
            string str = "(define-syntax my-and"
              " (syntax-rules ()"
              " ((_) (t))"
              " ((_ e) e)"
              " ((_ e1 e2 ...)"
              " (if e1"
              " (my-and e2 ...)"
              " (f)))))";
            obj scode = Parser(str.c_str(), str.size()).parse();
            obj sbcode = compile(scode, list(mk_opcode(OP_HALT)), &syntax);
            run(sbcode, &genv);
            printsexp(syntax);
            str = "(if (my-and (= 1 1) (= 2 2) (= 3 3)) (display 2) (display 3))";
            */
#else
            string str = io.read();
#endif /* DEBUG */
            if(io.isfail()) break;
            obj code = Parser(str.c_str(), str.size()).parse();
#ifdef DEBUG
            printsexp(code);
#endif
            obj bcode = compile(code, list(mk_opcode(OP_HALT)), &syntax);
#ifdef DEBUG
            printsexp(bcode);
#endif
            obj ret = run(bcode, &genv);
            printsexp(ret);
#ifdef DEBUG
            break;
#endif
          }catch(std::exception &e){
            cerr << e.what() << endl;
            break;
          }
          if(io.iseof()) break;
        }
      }

      void genv_init(obj* genv){
        define("+", OP_ADD, genv);
        define("-", OP_SUB, genv);
        define("*", OP_MULTIPLY, genv);
        define("/", OP_DIVIDE, genv);
        define("=", OP_EQUAL, genv);
        define("list", OP_LIST, genv);
        define("list**", OP_LIST_ASTA_ASTA, genv);
        define("car", OP_CAR, genv);
        define("cdr", OP_CDR, genv);
        define("begin", OP_BEGIN, genv);
        define("display", OP_DISPLAY, genv);
      }

    } vm;
  }
}

int main(int argc, char *argv[])
{
  PetitScheme::VM::VM().repl();

  return 0;
}
