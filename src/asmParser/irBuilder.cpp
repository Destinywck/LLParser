//
// Created by tlaber on 6/17/17.
//

#include "irBuilder.h"
#include "sysDict.h"
#include <utilities/strings.h>
#include <inst/instEssential.h>
#include <asmParser/llParser.h>
#include <asmParser/instParser.h>
#include <utilities/flags.h>
#include <inst/branchInst.h>

/** Create an instruction from a string
 *
 * This function creates an instruction according to its type and dispatches the parsing
 * work to a InstParser instance. Before handing the Instruction to an InstParser, IRBuilder
 * will set has_assignment, and name.
 *
 * @param text: the string representing the instruction
 * @param bb: defaults to zero, if specified, the new instruction will be appended to bb
 * @param llparser: specify which llparser to use, defaults to SysDict::parser
 * @return
 */
Instruction* IRBuilder::create_instruction(string &text, BasicBlock* bb, LLParser* llparser) {
    if (llparser == NULL) {
        llparser = SysDict::parser;
    }

    string op;
    string name;
    bool has_assignment = true;

    if (Strings::startswith(text, "%")) {
        int pos1 = text.find('=');

        /* set name */
        int name_start_pos = text.find_first_not_of(" ");
        int name_end_pos = pos1 - 1;
        while (text[name_end_pos] == ' ') {
            name_end_pos--;
        }
        name = text.substr(name_start_pos, name_end_pos+1-name_start_pos);

        guarantee(text[pos1+1] == ' ', " ");
        pos1 += 2;
        int pos2 = text.find(' ', pos1);
        op = text.substr(pos1, pos2-pos1);
    }
    else {
        guarantee(text[0] == ' ' && text[1] == ' ', " ");
        int pos1 = 2;
        int pos2 = text.find(' ', pos1);
        op = text.substr(pos1, pos2-pos1);
        has_assignment = false;
    }

    
    //zps(op.c_str())
    // only deal with CallInst for now
    // determine the opcode by the first word
    Instruction* inst = NULL;
    if (op == "call" || op == "tail" || op == "musttail" || op == "notail") {
        op = "call";
    }

    //void (InstParser::*parse_routine)(Instruction*) = NULL;

    void (*parse_routine) (Instruction*) = NULL;


    /* unaware of the instruction type at this point */
    switch (op[0]) {
        case 'b':
            if (op == "bitcast") {
                inst = new BitCastInst();
            }
            else if (op == "br") {
                inst = new BranchInst();
            }
            break;
        case 'c': {
            if (op == "call") {
                inst = new CallInst();
            }
            else {

            }

            break;
        }
        case 'i': {
            if (op == "invoke") {
                inst = new InvokeInst();
            }
        }
        case 'l': {
            if (op == "load") {
                inst = new LoadInst();
            }
        }
        default: {
            break;
        }
    }

    if (!inst) {
        inst = new Instruction();
    }

    inst->set_has_assignment(has_assignment); // have to repeat this for every inst
    if (has_assignment) {
        inst->set_name(name);
    }
    inst->set_raw_text(text);

    if (bb) {
        bb->append_instruction(inst);
        inst->set_owner(bb->parent()->name());  // todo: only for debug use, this _owner will not change when the owner's name changes
    }


    if (inst->type() != Instruction::UnknownInstType) {
        if (ParallelInstruction) {
            SysDict::worker_push_inst(inst);
        }
        else {
            llparser->inst_parser()->parse(inst);
        }
    }

//    if (sync) {
//        if (inst->type() != Instruction::UnknownInstType) {
//            SysDict::llparser()->inst_parser()->parse(inst);
//            //parse_routine(inst);
//        }
//    }
//    else {
//        guarantee(0, "unimplemented");
//        /* todo: push to a stack and each inst parser thread should call its own InstParser instance to parse it */
//    }

    if (InstFlags::in_terminator_insts(op)) {
        inst->set_opstr(op);
    }
    inst->set_raw_text(text);
    return inst;
}

Function* IRBuilder::create_function_declaration(string &text, LLParser* llparser) {
    if (llparser == NULL) {
        llparser = SysDict::parser;
    }
    Function* f = llparser->create_function(text);
    f->set_is_external();
    return f;
}
