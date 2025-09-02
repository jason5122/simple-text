#pragma once

namespace base {

class Location {
public:
    static Location current(const char* function_name = __builtin_FUNCTION(),
                            const char* file_name = __builtin_FILE(),
                            int line_number = __builtin_LINE());

    const char* function_name() const { return function_name_; }
    const char* file_name() const { return file_name_; }
    int line_number() const { return line_number_; }

private:
    // Constructor should be called with a long-lived char*, such as __FILE__. It assumes the
    // provided value will persist as a global constant, and it will not make a copy of it.
    Location(const char* function_name, const char* file_name, int line_number);

    const char* function_name_ = nullptr;
    const char* file_name_ = nullptr;
    int line_number_ = -1;
};

}  // namespace base
