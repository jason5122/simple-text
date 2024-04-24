#include "base/filesystem/file_reader.h"
#include "color_scheme.h"
#include "glaze/glaze.hpp"
#include <iostream>

namespace config {
struct person {
    std::string first_name;
    std::string last_name;
    uint32_t age{};
};

ColorScheme::ColorScheme() {
    fs::path color_scheme_path = DataPath() / "color_scheme.json";

    // TODO: Handle errors.
    glz::parse_error error;

    person p;
    error = glz::read_file_json(p, color_scheme_path.string(), std::string{});

    std::cerr << "person: " << p.first_name << ' ' << p.last_name << '\n';

    std::vector<person> directory;
    directory.emplace_back(person{"John", "Doe", 33});
    directory.emplace_back(person{"Alice", "Right", 22});

    std::string buffer;
    glz::write_json(directory, buffer);

    std::cout << buffer << "\n\n";

    std::array<person, 2> another_directory;
    error = glz::read_json(another_directory, buffer);
    if (error) {
        std::cerr << glz::format_error(error, buffer) << '\n';
    }

    std::string another_buffer;
    glz::write_json(another_directory, another_buffer);

    if (buffer == another_buffer) {
        std::cout << "Directories are the same!\n";
    }
}
}
