#include <expected>
#include <print>

namespace {

enum class MathError { NegativeInput, ZeroInput };

std::expected<double, MathError> square_root(double value) {
    if (value < 0.0) {
        return std::unexpected(MathError::NegativeInput);
    } else if (value == 0.0) {
        return std::unexpected(MathError::ZeroInput);
    }
    return std::sqrt(value);
}

std::expected<double, MathError> reciprocal(double v) {
    if (v == 0.0) {
        return std::unexpected(MathError::ZeroInput);
    }
    return 1.0 / v;
}

}  // namespace

int main() {
    double val = 16.0;
    auto result = square_root(val).and_then(reciprocal);
    std::println("Square root of {} (or default): {}", val, result.value_or(0.0));
}
