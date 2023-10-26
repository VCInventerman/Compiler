#ifndef COMPILER_ERROR_H
#define COMPILER_ERROR_H

#include <string>
#include <stdexcept>

#include "source.h"

class SourceError : public std::runtime_error
{
    std::string message;

public:
    const std::string& createMessage(std::string message_, SourcePos position_) {
        message = message_;
        message.append("  ");
        message.append(std::string(position_));
        return message;
    }

    SourceError(std::string message_) :
        runtime_error(message_) {}

    SourceError(std::string message_, SourcePos position_) : 
        //runtime_error(createMessage(message_, position_)) {}
        runtime_error(message_) {}
};

#define eassert_STR_HELPER(x) #x
#define eassert_STR(x) eassert_STR_HELPER(x)
#define eassert(cond) if (!(cond)) { throw SourceError("Failed assertion in " __FILE__ " on line " eassert_STR(__LINE__)); }

#endif // ifndef COMPILER_ERROR_H