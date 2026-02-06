#include <expected>
#include <print>

enum class SquareRootError { NegativeInput, ZeroInput };

std::expected<double, SquareRootError> square_root(double value) {
    if (value < 0.0) {
        return std::unexpected(SquareRootError::NegativeInput);
    } else if (value == 0.0) {
        return std::unexpected(SquareRootError::ZeroInput);
    }
    return std::sqrt(value);
}

int main() {
    auto result1 = square_root(16.0);
    if (result1.has_value()) {
        std::println("Square root of 16 is: {}", *result1);
    }

    auto result2 = square_root(-4.0);
    if (!result2.has_value()) {
        // Access the error using .error()
        if (result2.error() == SquareRootError::NegativeInput) {
            std::println("Error: Cannot take the square root of a negative number.");
        }
    }

    auto result3 = square_root(0.0);
    std::println("Square root of 0 (or default): {}", result3.value_or(0.0));
}
