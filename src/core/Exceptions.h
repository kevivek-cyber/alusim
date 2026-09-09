// ---------------------------------------------------------------------------
// Exceptions.h  --  our own exception hierarchy
//
// Covers: PSOOP Practical 8 (exception handling, user-defined exception class)
//         PSOOP Practical 6 (inheritance -- all derive from SimulatorException)
// ---------------------------------------------------------------------------
#ifndef CORE_EXCEPTIONS_H
#define CORE_EXCEPTIONS_H

#include <exception>
#include <string>

namespace core {

// Base class for every error the simulator can raise.
class SimulatorException : public std::exception {
protected:
    std::string message_;
public:
    explicit SimulatorException(const std::string& msg) : message_(msg) {}
    virtual ~SimulatorException() throw() {}

    virtual const char* what() const throw() { return message_.c_str(); }
    virtual std::string kind() const { return "SimulatorException"; }
};

class InvalidOpcodeException : public SimulatorException {
public:
    explicit InvalidOpcodeException(const std::string& msg)
        : SimulatorException("Invalid opcode: " + msg) {}
    std::string kind() const { return "InvalidOpcodeException"; }
};

class StackUnderflowException : public SimulatorException {
public:
    explicit StackUnderflowException(const std::string& msg)
        : SimulatorException("Stack underflow: " + msg) {}
    std::string kind() const { return "StackUnderflowException"; }
};

class StackOverflowException : public SimulatorException {
public:
    explicit StackOverflowException(const std::string& msg)
        : SimulatorException("Stack overflow: " + msg) {}
    std::string kind() const { return "StackOverflowException"; }
};

class IndexOutOfRangeException : public SimulatorException {
public:
    explicit IndexOutOfRangeException(const std::string& msg)
        : SimulatorException("Index out of range: " + msg) {}
    std::string kind() const { return "IndexOutOfRangeException"; }
};

class InvalidRegisterException : public SimulatorException {
public:
    explicit InvalidRegisterException(const std::string& msg)
        : SimulatorException("Invalid register: " + msg) {}
    std::string kind() const { return "InvalidRegisterException"; }
};

class AssemblyErrorException : public SimulatorException {
public:
    explicit AssemblyErrorException(const std::string& msg)
        : SimulatorException("Assembly error: " + msg) {}
    std::string kind() const { return "AssemblyErrorException"; }
};

class DivideByZeroException : public SimulatorException {
public:
    explicit DivideByZeroException(const std::string& msg)
        : SimulatorException("Divide by zero: " + msg) {}
    std::string kind() const { return "DivideByZeroException"; }
};

} // namespace core

#endif // CORE_EXCEPTIONS_H
