// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH
// Copyright (c) 2015 Nic30

#pragma once

#include <exception>
#include <string>

namespace parser {

class ParseException : public std::exception {
private:
    std::string _msg;

public:
    ParseException(std::string msg) throw()
    : _msg(msg) {}
    virtual ~ParseException() = default;
    virtual const char* what() const throw() { return _msg.c_str(); }
};

} // namespace parser
