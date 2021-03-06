//
// Created by GentlyGuitar on 6/8/2017.
//

#ifndef LLPARSER_PASS_H
#define LLPARSER_PASS_H

#include <string>
#include <cstdio>
#include <map>
#include <utilities/macros.h>

class Module;
class Function;
class BasicBlock;
class Instruction;
class Pass;


typedef Pass* (*pass_loader)();
typedef void (*pass_unloader)(Pass*);


/* represent an immutable pass */
class Pass {
    bool _is_dynamic;
    int _priority;
    bool _is_global_pass;
    bool _is_module_pass;
    bool _is_function_pass;
    bool _is_basic_block_pass;
    bool _is_instruction_pass;

    bool _is_parse_time;
    std::map<string, string> _args;
    pass_unloader _unloader;
    string _name;
public:
    Pass();

    int priority()                  { return _priority; }
    void set_priority(int p)        { _priority = p; }

    bool is_dynamic() const {
        return _is_dynamic;
    }

    void set_is_dynamic(bool _is_dynamic=true) {
        Pass::_is_dynamic = _is_dynamic;
    }

    const string &name() const {
        return _name;
    }

    void set_name(const string &_name) {
        Pass::_name = _name;
    }

    std::map<string, string>& arguments()                  { return _args; };
    void parse_arguments(string args);
    string get_argument(string key);
    void set_argument(string key, string value)            { _args[key] = value; }
    bool has_argument(string key);
    void print_arguments();

    void set_unloader(pass_unloader v)                     { _unloader = v; }
    void unload();

    bool is_global_pass()                          { return _is_global_pass; }
    bool is_module_pass()                          { return _is_module_pass; }
    bool is_function_pass()                        { return _is_function_pass; }
    bool is_basic_block_pass()                     { return _is_basic_block_pass; }
    bool is_instruction_pass()                     { return _is_instruction_pass; }

    void set_is_global_pass(bool v=1)        { _is_global_pass = v; }
    void set_is_module_pass(bool v=1)        { _is_module_pass = v; }
    void set_is_function_pass(bool v=1)      { _is_function_pass = v; }
    void set_is_basic_block_pass(bool v=1)   { _is_basic_block_pass = v; }
    void set_is_instruction_pass(bool v=1)   { _is_instruction_pass = v; }

    bool is_run_at_parse_time()              { return  _is_parse_time; }
    void set_run_at_parse_time(bool v=1)     { _is_parse_time = v; }


    virtual bool run_on_global() {
        printf("Pass.run_on_global called: do nothing\n");
        return false; // not mutate
    }

    virtual bool run_on_module(Module* module) {
        printf("Pass.run_on_module called: do nothing\n");
        return false; // not mutate
    }

    virtual bool run_on_function(Function* func) {
        printf("Pass.run_on_function called: do nothing\n");
        return false; // not mutate
    }

    virtual bool run_on_basic_block(BasicBlock* bb) {
        printf("Pass.run_on_basic_block called: do nothing\n");
        return false; // not mutate
    }

    virtual bool run_on_instruction(Instruction* inst) {
        printf("Pass.run_on_instruction called: do nothing\n");
        return false; // not mutate
    }

    /**@brief Do initialization work for a module pass
     *
     * This happens AFTER pass's arguments are parsed,
     * so this method is often used to do some further
     * processing before it runs on any module.
     */
    virtual void do_initialization() {
        printf("<warning>: Pass::do_initialization() called, do nothing\n");
    }

    virtual void do_finalization() {
        printf("<warning>: Pass::do_finalization() called, do nothing\n");
    }

    virtual void do_initialization(Module* M) {
        printf("<warning>: Pass::do_initialization(Module*) called: do nothing\n");
    }

    virtual void do_finalization(Module* M) {
        printf("<warning>: Pass::do_finalization(Module*) called: do nothing\n");
    }

    virtual void do_initialization(Function* F) {
        printf("<warning>: Pass::do_initialization(Function*) called: do nothing\n");
    }

    virtual void do_finalization(Function* F) {
        printf("<warning>: Pass::do_finalization(Function*) called: do nothing\n");
    }

};



#define REGISTER_PASS(classname) \
    extern "C" Pass* __load_pass_##classname() { \
        /* printf("dynamically load pass " #classname "!\n"); */ \
        Pass* p = new classname(); \
        p->set_name(#classname); \
        return p; \
    } \
    extern "C" void __unload_pass_##classname(classname* p) { \
        /* printf("dynamically unload pass " #classname "!\n"); */ \
        delete p; \
    } \


#endif //LLPARSER_PASS_H
