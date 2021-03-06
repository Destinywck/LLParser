//
// Created by GentlyGuitar on 6/6/2017.
//

#ifndef LLPARSER_LLPARSER_H
#define LLPARSER_LLPARSER_H

#include <fstream>
#include <peripheral/FileParser.h>
#include "ir/basicBlock.h"
#include "ir/module.h"
#include "utilities/symbol.h"
#include "utilities/macros.h"
#include "instParser.h"
#include "irBuilder.h"

class Module;
class StructType;
class Function;
class BasicBlock;
class GlobalVariable;

class DIFile;
class DISubprogram;
class DILocation;
class DILexicalBlock;
class DILexicalBlockFile;

class InstParser;
class SysDict;

class InstStats {
    size_t _parsed;
    std::map<Instruction::InstType, size_t> _inst_count;
    std::map<string, size_t> _op_count;
public:
    InstStats(): _parsed(0) {}
    void collect_inst_stats(Instruction* ins);
    void report();
};

class LLParser: public FileParser, public IRParser {
    int _major_version;
    float _minor_version;
    int _asm_format;
    bool _has_structs;
    bool _has_globals;
    int _unresolved;  // The number of unresolved functions
    bool _debug_info;
    Module* _module;
    InstParser* _inst_parser;
    bool _done;

    /* some basic statistics */
    InstStats _stats;
public:
    int MAX_LINE_LEN;
    int MAX_VALUE_LEN;

    LLParser();
    ~LLParser();
    LLParser(const char* file);
    void initialize();

    Module* module()                                                        { return _module; }
    InstParser* inst_parser()                                               { return _inst_parser; }

    void set_done(bool v=1);
    bool is_done();

    void inc_inline_pos(int steps=1)                                        { inc_intext_pos(steps); }
    void set_line(string l)                                                 { set_text(l); }

    void set_llvm_version(string v);
    Module* parse();
    Module* parse(string file);
    void parse_header(Module* );
    void parse_module_level_asms();
    void parse_structs(Module* );
    void parse_comdats();
    void parse_globals(Module* );
    GlobalVariable* parse_global(Module* );
    void parse_aliases();
    void parse_functions();
    void parse_function_declaration();
    void parse_function_definition();
    Function* parse_function_header();
    Function* create_function(string& text);
    Function* parse_function_name_and_args();
    void parse_basic_block(BasicBlock* bb);
    void parse_basic_block_header(BasicBlock* bb, bool use_comment=false);
    Instruction* parse_instruction_line(BasicBlock* bb);
    void parse_instruction_table(BasicBlock* bb, string op="");
    void parse_attributes(Module* module);
    void parse_metadatas(Module* module);
    DIFile* parse_difile();
    DISubprogram* parse_disubprogram();
    DILexicalBlock* parse_dilexicalblock();
    DILexicalBlockFile* parse_dilexicalblockfile();
    DILocation* parse_dilocation();
    void parse_di_fields(MetaData* data);
    void parse_debug_info(Instruction* );
    void parse_function_pointer_type();


    void set_line_to_full_instruction();
    void remove_tail_comments();

    void resolve();
    void print_stats();
};







#endif //LLPARSER_LLPARSER_H
