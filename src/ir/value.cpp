//
// Created by GentlyGuitar on 6/6/2017.
//

#include <fstream>
#include <algorithm>
#include <cstdio>
#include "value.h"

Value::Value(): Shadow() {
    _name = "";
    _copy_cnt = 1000;  // use a large number to avoid collision with llvm's renaming scheme
    _copy_prototype = NULL;
}
//
//bool Value::has_property(string key) {
//    if (_properties.find(key) == _properties.end()) {
//        return false;
//    }
//    else {
//        return true;
//    }
//}
//
//string Value::get_properties(string key) {
//    if (has_property(key)) {
//        return _properties[key];
//    }
//    else {
//        return "";
//    }
//}
//
//void Value::set_property(string key, string value) {
//    _properties[key] = value;
//}

string Value::prototype_name() {
    Value* orig = this;
    while (orig->copy_prototype()) {
        orig = orig->copy_prototype();
    }
    return orig->name();
}

void Value::append_user(Instruction *user) {
    guarantee(_users.find(user) == _users.end(), "A value is used twice by instruction %p", user);
    _users.insert(user);
}

void Value::remove_user(Instruction *user) {
    auto& vec = user_set();
    vec.erase(user);  // remove from set is simpler than remove from vector
}

void Value::print_to_file(const char *file) {
    std::ofstream ofs;
    ofs.open(file);
    if (!ofs.good()) {
        fprintf(stderr, "open file %s failed.\n", file);
    }
    ofs << this;
    ofs.close();
//    FILE* fp = fopen(file, "w");
//    if (fp == NULL) {
//        fprintf(stderr, "open file %s failed", file);
//        return;
//    }
//    print_to_stream(fp);
//    fclose(fp);
}

void Value::print_to_file(string file) {
    print_to_file(file.c_str());
//    std::ofstream ofs(file);
//    print_to_stream(ofs);
//    ofs.close();
}

void Value::print_to_stream(std::ostream &os) {
    os << raw_text() << std::endl;
}

std::ostream& operator<<(std::ostream& os, Value* v) {
    v->print_to_stream(os);  // virtual
    return os;
}

void Value::print_to_stream(FILE *fp) {
    fprintf(fp, "%s\n", raw_text().c_str());
    //fprintf(fp, "Value::print_to_stream called, are you sure this should be called?\n");
}