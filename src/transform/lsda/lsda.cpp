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


struct MFunc {
    string old_name;
    string new_name;
    bool add_id;

    MFunc(string oldname, string newname, bool addid):
            old_name(oldname), new_name(newname), add_id(addid) {}
};

/**@brief This class don't use SysDict::module()
 *
 */
class LSDAPass: public Pass {
    /* language specific stuff */
    string _lang;
    std::vector<MFunc*> _alloc_set;
    std::vector<MFunc*> _free_set;
    bool _use_indi;
public:
    LSDAPass() {
        set_is_module_pass();

        _lang = "all";
        _use_indi = false;
    }

    ~LSDAPass() {

    }

    const std::vector<MFunc *> &alloc_set() const {
        return _alloc_set;
    }

    void set_alloc_set(const std::vector<MFunc *> &_alloc_set) {
        LSDAPass::_alloc_set = _alloc_set;
    }

    const std::vector<MFunc *> &free_set() const {
        return _free_set;
    }

    void set_free_set(const std::vector<MFunc *> &_free_set) {
        LSDAPass::_free_set = _free_set;
    }

    bool use_indi() const {
        return _use_indi;
    }

    void set_use_indi(bool _use_indi) {
        LSDAPass::_use_indi = _use_indi;
    }

    void set_lang(string lang) {
        guarantee(lang == "c" || lang == "cpp" || lang == "fortran" || lang == "all", "");
        _lang = lang;
    }

    const string& lang() {
        return _lang;
    }

    void init_lang(Module* module) {
        if (_lang == "c" || _lang == "cpp" || _lang == "all") {
            _alloc_set.push_back(new MFunc("malloc", "ben_malloc", true));
            _alloc_set.push_back(new MFunc("calloc", "ben_calloc", true));
            _alloc_set.push_back(new MFunc("realloc", "ben_realloc", true));
            _free_set.push_back(new MFunc("free", "ben_free", false));
        }

        if (_lang == "cpp" || _lang == "all") {
            _alloc_set.push_back(new MFunc("_Znam", "ben_malloc", true));
            _alloc_set.push_back(new MFunc("_Znwm", "ben_malloc", true));
            _free_set.push_back(new MFunc("_ZdaPv", "ben_free", false));
            _free_set.push_back(new MFunc("_ZdlPv", "ben_free", false));
        }


//
//        insert_declaration("malloc", "ben_malloc", true);
//        insert_declaration("calloc", "ben_calloc", true);
//        insert_declaration("realloc", "ben_realloc", true);
//        insert_declaration("free", "ben_free", false);
//
//        insert_declaration("malloc", "indi_malloc", false);
//        insert_declaration("calloc", "indi_calloc", false);
//        insert_declaration("realloc", "indi_realloc", false);
//        insert_declaration("free", "indi_free", false);

    }

    void insert_lsd(Module* module) {
        for (auto t: _alloc_set) {
            insert_declaration(module, t->old_name, t->new_name, t->add_id);
            if (_use_indi) {
                string indi_name = t->new_name;
                Strings::replace(indi_name, "ben_", "indi_");
                insert_declaration(module, t->old_name, indi_name, false);
            }
        }

        for (auto t: _free_set) {
            insert_declaration(module, t->old_name, t->new_name, t->add_id);
            if (_use_indi) {
                string indi_name = t->new_name;
                Strings::replace(indi_name, "ben_", "indi_");
                insert_declaration(module, t->old_name, indi_name, false);
            }
        }
    }

    bool run_on_module(Module* module) override {
        if (has_argument("lang")) {
            _lang = get_argument("lang");
        }
        if (has_argument("indi")) {
            _use_indi = true;
        }

        init_lang(module);
        insert_lsd(module);
    }
//
//    void replace_free() {
//        for (auto t: _free_set) {
//            if (Function* free_fp = SysDict::module()->get_function(t->old_name)) {
//                for (auto ci: free_fp->caller_list()) {
//                    ci->replace_callee(t->new_name);
//                }
//            }
//        }
//
//        //todo: non-dirty way
//        if (!_use_indi) {
//            return;
//        }
//
//        string suffixes[3] = {" ", ",", ")"};
//        for (auto F: SysDict::module()->function_list()) {
//            for (auto B: F->basic_block_list()) {
//                for (auto I: B->callinst_list()) {
//                    for (auto& suf: suffixes) {
//                        //string targets[4] = {"malloc", "calloc", "realloc", "free"};
//                        for (auto& t: _alloc_set) {
//                            string old = "@"+t->old_name+suf;
//                            if (I->raw_text().find(old) != string::npos) {
//                                Strings::replace(I->raw_text(), old, "@indi_"+t->old_name+suf);
//                            }
//                        }
//                        for (auto& t: _free_set) {
//                            string old = "@"+t->old_name+suf;
//                            if (I->raw_text().find(old) != string::npos) {
//                                Strings::replace(I->raw_text(), old, "@indi_"+t->old_name+suf);
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }

    bool insert_declaration(Module* m, string oldname, string newname, bool add_id=true) {
        Function* func = m->get_function(oldname);

        if (func == NULL) {
            return false;
        }
        guarantee(func->is_external(), "malloc family should be external");

        if (m->get_function(newname)) {
            return false; // return if already inserted
        }

        /* manipulate the text */
        string text = func->raw_text();
        string old_call = oldname + '(';
        string new_call = newname + "(i32, ";
        //if (!add_id || Strings::startswith(oldname, "f90_")) {  // a bit funky
        if (!add_id) {
            new_call = newname + '(';
        }

        Strings::replace(text, old_call, new_call);
        Function* newfunc = IRBuilder::create_function_declaration(text);
        m->insert_function_after(func, newfunc);
    }

};


REGISTER_PASS(LSDAPass);
