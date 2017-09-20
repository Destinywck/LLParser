//
// Created by tlaber on 6/9/17.
//

#ifndef LLPARSER_SYSDICT_H
#define LLPARSER_SYSDICT_H

#include <vector>
#include <ir/irEssential.h>

class LLParser;
class InstParser;

class SysDict {
public:
    typedef std::map<pthread_t , Module*> ModuleTableType;
private:
    static std::vector<Instruction*> _inst_stack;
    static std::map<pthread_t , Module*> _module_table;
    //static std::map<string, Module*> module_table;
    static std::vector<Module*> _modules;
public:

    static void init();
    static void destroy();

    //static Module* get_module(string name);


    static void worker_push_inst(Instruction*);
    static Instruction* worker_fetch_instruction();
    static bool inst_stack_is_empty();

    /* thread specific */
    static void add_module(LLParser* );
    static LLParser* llparser();
    static Module* module();
    static const string& filename();
    static const string filedir();

    /* for UseSplitModule */
    static void merge_modules();



    static LLParser* parser;

    //static InstParser* instParser;
};

#endif //LLPARSER_SYSDICT_H
