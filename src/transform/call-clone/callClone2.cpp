//
// Created by tzhou on 8/10/17.
//

#include <set>
#include <algorithm>
#include <utilities/macros.h>
#include <asmParser/sysDict.h>
#include <passes/pass.h>
#include <di/diEssential.h>
#include <ir/irEssential.h>
#include <utilities/strings.h>
#include <peripheral/sysArgs.h>
#include <inst/instEssential.h>
#include <asmParser/irBuilder.h>
#include <passes/passManager.h>
#include <transform/ben-alloc/benAlloc.cpp>

#include "contextGenerator.h"


class CallClonePass: public Pass {
    /* clone algorithm data structure */
    std::vector<XPath*> _all_paths;
    std::vector<CallInstFamily*> _stack;
    string _caller;
    string _callee;
    int _min_cxt;

    /* flags */
    bool _done;
    bool _skip;
    bool _noben;
    bool _use_indi;
    bool _logclone;
    bool _path_check;
    bool _print_cloning;
    bool _verbose_match;
    bool _verbose_clone;

    /* statistics */
    int _ben_num;
    int _cloned;
    int _hot_counter;
    int _path_counter;
    int _cxt_counter;
    int _recursive;
    int _recognized;
    int _hot_cxt_counter;
    int _used_hot_cxt;

    Timer _timer;
    std::ofstream _clone_log;

    /* language specific stuff */
    string _lang;
    std::vector<MFunc*> _alloc_set;
    std::vector<MFunc*> _free_set;
public:
    CallClonePass() {
        _timer.start();

        set_is_module_pass();

        _done = false;
        _verbose_clone = false;
        _verbose_match = false;
        _path_check = false;
        _print_cloning = false;
        _skip = false;
        _logclone = false;
        _use_indi = false;
        _noben = false;
        _min_cxt = 1;
        _path_counter = 1;
        _cxt_counter = 0;
        _recursive = 0;
        _cloned = 0;

        _ben_num = 0;
        _recognized = 0;
        _hot_counter = 0;
        _hot_cxt_counter = 0;
        _used_hot_cxt = 0;

        _lang = "all";
    }

    ~CallClonePass() {
        if (_logclone) {
            _clone_log.close();
        }
    }

    bool check_common_edge(std::vector<XPath*>& paths, int pos) {
        guarantee(pos < paths[0]->path.size(),
                  "pos: %d, size: %lu", pos, paths.size());
        auto sentinel = paths[0]->path[pos];
        for (int i = 1; i < paths.size(); ++i) {
            if (paths[i]->path[pos] != sentinel) {
                return false;
            }
        }
        return true;
    }

    void scan() {
        bool replaced = false;
        // This map uses the first call site from each path
        // as the key, and each value is a set of paths that
        // start with that key.
        // If each path has a distinct first call site (no
        // call sites share the same first call site), the
        // cloning algorithm terminates.
        std::map<CallInstFamily*, std::vector<XPath*>> head_map;
        for (auto xpath: _all_paths) {
            auto dr = xpath->path[0];
            if (head_map.find(dr) == head_map.end()) {
                head_map[dr] = std::vector<XPath*>();
            }
            head_map[dr].push_back(xpath);
        }

        for (auto it: head_map) {
            auto& paths = it.second;
            if (paths.size() == 1) {
                continue;
            }

            // If N paths share a first call site, clone
            // the callee N-1 times
            replaced = true;
            int i = 0;
            for (; check_common_edge(paths, i) ; ++i) {}
            if (i == 0) {
                for (auto p: paths) {
                    check_path(p->path, true);
                }
            }
            guarantee(i > 0, "");

            /* paths is a set of XPS_Paths that share i
             * common items, for example
             * p1: I1 < I2 < I3
             * p2: I1 < I2 < I4
             *
             * paths: {p1, p2}
             * i is the position where divergence starts,
             * which would be 2 in this example.
             * i-1 (1) points to the last shared item
             */

            /* the last shared instruction */
            auto root = paths[0]->path[i-1];
            auto callee = root->function();

            /* Always let the first path remain the same
             * and rewrite other paths
             */
            Instruction* last_updated_caller = NULL;
            for (int j = 1; j < paths.size(); ++j) {
                auto& this_path = paths[j]->path;

                if (this_path[i] != last_updated_caller) {
                    auto cloned_callee = callee->clone();

                    if (_logclone) {
                        record_clone(callee, cloned_callee);
                    }
                    SysDict::module()->append_new_function(
                        cloned_callee);
                    _cloned++;
                    update_callee_in_all_paths(this_path[i],
                                               cloned_callee);
                    last_updated_caller = this_path[i];
                }
            }
            break;

        }

        if (!replaced) {
            _done = true;
        }
    }

    void record_clone(Function* host, Function* cloned) {
        auto sp = host->di_subprogram();
        guarantee(sp, "Function %s has no debug info",
                  host->name_as_c_str());
        string host_log_name = sp->to_string();
        string cloned_log_name = cloned->name();
        _clone_log << host_log_name << " "
                   << cloned_log_name << " ";
    }

    void record_callee_update(Instruction* caller,
                              Function* new_callee) {
        auto sp = caller->function()->di_subprogram();
        guarantee(sp, "Function %s has no debug info",
                  caller->function()->name_as_c_str());
        string caller_log_name = sp->to_string();
        _clone_log << caller_log_name << " "
                   << caller->get_position_in_function()
                   << " " + new_callee->name() << "\n";
    }

    void update_callee_in_all_paths(CallInstFamily* caller,
                                    Function* new_callee) {
        bool is_replaced = false;
        for (auto& xps_path: _all_paths) {
            auto& stack = xps_path->path;
            guarantee(caller != NULL, " ");
            for (int i = 1; i < stack.size(); ++i) {
                CallInstFamily* I = stack[i];
                if (I != caller) {
                    continue;
                }

                /* Replace I's callee to new_callee */
                if (!is_replaced) {
                    if (_verbose_clone) {
                        zpl("in callinst repalce %s to %s in %p",
                            I->called_function()->name_as_c_str(),
                            new_callee->name_as_c_str(), I)
                    }

                    if (_logclone) {
                        record_callee_update(I, new_callee);
                    }

                    I->replace_callee(new_callee->name());
                    is_replaced = true;

                }

                /* Replace the i-1th element with new_callee's inst
                 * in the stack */
                auto ci_to_replace = stack[i-1];
                auto call_pos = ci_to_replace->get_position_in_function();
                auto ci_in_new_callee = static_cast<CallInstFamily*>(
                    new_callee->get_instruction(call_pos));
                stack[i-1] = ci_in_new_callee;
                if (_verbose_clone) {
                    zpl("path pos %d to %s", i - 1,
                        ci_in_new_callee->function()->name_as_c_str())
                }
                //check_path(stack);

            }

        }
    }

    void get_distinct_all_paths() {
        std::vector<XPath*> distinct_set;
        std::set<string> contexts;

        int hotnesses[2] = {1, 0}; // hot first, cold second
        for (auto tp: hotnesses) {
            for (auto v: _all_paths) {
                if (v->hotness == tp) {
                    string context;
                    for (auto I: v->path) {
                        char buf[128];
                        sprintf(buf, "%p", I);
                        context += string(buf) + ' ';
                    }

                    if (contexts.find(context) != contexts.end()) {

                    }
                    else {
                        contexts.insert(context);
                        distinct_set.push_back(v);
                    }

                }
            }
        }

        _all_paths = distinct_set;
    }

    bool has_direct_recursion() {
        std::map<CallInstFamily*, int> counters;
        for (auto I: _stack) {
            counters[I]++;
        }
        for (auto i: counters) {
            if (i.second > 1) {
                _recursive++;

                auto& v = _stack;
                string callee = v[0]->called_function()->name();
                printf("skip: %s", callee.c_str());
                for (auto I: v) {
                    guarantee(I->called_function()->name() == callee,
                              "%s, %s", I->called_function()->name_as_c_str(),
                              callee.c_str());
                    printf(" <- %s(%p)", I->function()->name_as_c_str(), I);
                    callee = I->function()->name();
                }
                return true;
            }
        }

        return false;
    }

    bool has_continuous_recursion() {
        std::map<CallInstFamily*, int> counters;
        for (int i = 1; i < _stack.size(); i++) {
            if (_stack[i] == _stack[i-1]) {
                return true;
            }
        }

        return false;
    }

    void generate(Module* module, int nlevel) {
        ContextGenerator cg;

        for (auto target: _alloc_set) {
            auto paths = cg.generate(module,
                                     target->old_name, nlevel);
            _all_paths.insert(_all_paths.end(),
                              paths.begin(), paths.end());
        }
    }

    void check_unused(Module* module) {
        printf("unused clone check...\n");
        for (auto F:module->function_list()) {
            if (F->user_set().empty() && F->is_clone()) {
                printf("Function %s is unused\n", F->name_as_c_str());
            }
        }
    }

    string get_apid_from_args(string& args) {
        int p1 = args.find(' ');
        int p2 = args.find(',');
        return args.substr(p1+1, p2-p1-1);
    }

    void generate_post_contexts() {
        std::ofstream ctx_log(SysDict::filedir() + "post-contexts.txt");
        std::ofstream ben_log(SysDict::filedir() + "ben.log");
        std::set<string> alloc_files;
        printf("call site num: %d\n", _all_paths.size());
        for (auto xpath: _all_paths) {
            CallInstFamily* alloc_caller = xpath->path[0];
            string args = alloc_caller->get_raw_field("args");
            ctx_log << get_apid_from_args(args) << " "
                    << alloc_caller->called_function()->name()
                    << std::endl;
            bool is_first_line = true;
            for (auto ci: xpath->path) {
                DILocation* loc = ci->debug_loc();
                ctx_log << '(' << ci->function()->name()
                        << '+' << ci->get_position_in_function()
                        << ") " << loc->filename()
                        << ':' << loc->line() << std::endl;

                /* Also record what files have allocations */
                if (!is_first_line) {
                    continue;
                }

                if (ci->function()->name().find("__gnu_cxx") == string::npos) {
                    if (alloc_files.find(loc->filename()) == alloc_files.end()) {
                        ben_log << loc->filename() << std::endl;
                        alloc_files.insert(loc->filename());
                    }
                }

                is_first_line = false;

            }
            ctx_log << std::endl;
        }
        
        ctx_log.close();
        ben_log.close();
    }

    void check_all_paths(bool do_print=false) {
        for (auto v: _all_paths) {
            string callee = v->path[0]->called_function()->name();
            if (do_print)
                printf("%d: %s", v->hotness, callee.c_str());
            for (auto I: v->path) {
                guarantee(I->called_function()->name() == callee, "%s, %s",
                          I->called_function()->name_as_c_str(), callee.c_str());
                if (do_print)
                    printf(" <- %p %s(%d, %d)", I, I->function()->name_as_c_str(),
                           I->parent()->get_index_in_function(),
                           I->get_index_in_block());
                callee = I->function()->name();
            }
            if (do_print)
                printf("\n");
        }
    }

    void check_path(std::vector<CallInstFamily*>& path, bool do_print=false) {
        string callee = path[0]->called_function()->name();
        bool terminate = false;
        string msg;
        for (auto I: path) {
            if (I->called_function()->name() != callee) {
                terminate = true;
                msg = "I called: " + I->called_function()->name()
                    + " but should call " + callee + " at "
                    + I->function()->name_as_c_str() + ":"
                    + I->get_position_in_function().c_str();
                if (do_print) {
                    zpl("wrong %p called %p: %s", I,
                        I->called_function(), msg.c_str())
                }
            }
            if (do_print)
                printf("%s <- %s(%p, %s)\n", callee.c_str(),
                       I->function()->name_as_c_str(), I,
                       I->get_position_in_function().c_str());
            callee = I->function()->name();
        }
        if (terminate) {
            guarantee(0, " ");
        }

        if (do_print)
            printf("\n");
    }

    void do_initialization() override {}
    void do_finalization() override {}

    bool run_on_module(Module* module) override {
        int nlevel = 2;
        if (has_argument("nlevel")) {
            nlevel = std::stoi(get_argument("nlevel"));
        }
        if (has_argument("logclone")) {
            _logclone = (bool)std::stoi(get_argument("logclone"));
            _clone_log.open(SysDict::filedir()+"clone.log");
        }
        if (has_argument("min-cxt")) {
            _min_cxt = std::stoi(get_argument("min-cxt"));
            zpd(_min_cxt)
        }
        if (has_argument("lang")) {
            _lang = get_argument("lang");
        }
        if (has_argument("indi")) {
            _use_indi = (bool)std::stoi(get_argument("indi"));
        }
        if(has_argument("noben")) {
            _noben = true;
        }

        auto lsda = new BenAllocPass(_lang);
        lsda->do_initialization();
        zpd(_use_indi)
        lsda->set_use_indi(_use_indi);
        lsda->insert_lsd(module);
        _alloc_set = lsda->alloc_set();
        _free_set = lsda->free_set();

        string arg_name = "hot_aps_file";
        if (has_argument(arg_name)) {
            string hot_aps_file = get_argument(arg_name);
            //load_hot_aps_file(hot_aps_file);
        }
        else {
            //load_hot_aps_file(SysDict::filedir() + "contexts.txt");
            generate(module, nlevel);
        }


        get_distinct_all_paths();
        check_all_paths();


        int round = 0;
        while (!_done) {
            scan();
            if (_verbose_clone) {
                zpl("one clone done.")
            }

            round++;
            zpl("round: %d", round);
        }

        if (!_noben) {
            lsda->replace_alloc(module);
            lsda->replace_free(module);
            if (_use_indi) {
                lsda->replace_indi(module);
            }
        }

        zpl("to generate post")
        generate_post_contexts();
        zpl("done generating post")

        //        replace_alloc();
        //        replace_free();

        string out = SysArgs::get_option("output");
        if (out.empty()) {
            out = SysDict::filename();
            if (Strings::contains(out, ".ll")) {
                Strings::ireplace(out, ".ll", ".clone.ll");
            }
            else {
                out += ".clone.ll";
            }

            if (_noben) {
                Strings::ireplace(out, ".clone", ".expand");
            }
        }


        zpl("callclone2 output to %s", out.c_str())
        module->print_to_file(out);

        check_all_paths();
        check_unused(module);

        zpl("======== Summary ======");
        zpl("recog: %d, cxt: %d, recursive: %d, "
                "distinct: %d, cloned: %d, round: %d",
            _recognized, _cxt_counter, _recursive,
            _all_paths.size(), _cloned, round);
  }

};


REGISTER_PASS(CallClonePass);
