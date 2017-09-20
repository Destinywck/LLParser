//
// Created by tzhou on 9/18/17.
//

#ifndef LLPARSER_CONTEXTGENERATOR_H
#define LLPARSER_CONTEXTGENERATOR_H

#include <ir/module.h>

struct XPath {
    std::vector<CallInstFamily*> path;
    int hotness;
};

class ContextGenerator {
    std::vector<XPath*> _paths;
    std::vector<CallInstFamily*> _stack;
public:
    void generate(Module* module, string alloc="malloc", int nlevel=1);
    void traverse();
};

#endif //LLPARSER_CONTEXTGENERATOR_H