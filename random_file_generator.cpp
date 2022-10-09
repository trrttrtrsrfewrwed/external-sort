#include <iostream>
#include <stdexcept>
#include <string>
#include <random>
#include <fstream>

std::random_device rd;
std::mt19937 gen(rd());
char letters[] = "abcdefghijklmnopqrstuvwxyz";

int read_arg(const std::string& arg) {
    int result;
    try {
        std::size_t pos;
        result = std::stoi(arg, &pos);
        if (pos < arg.size()) {
            throw std::invalid_argument("Trailing characters after number: " + arg);
        }
    } catch (std::invalid_argument const &ex) {
        throw std::invalid_argument("Invalid number: " + arg);
    } catch (std::out_of_range const &ex) {
        throw std::invalid_argument("Number out of range: " + arg);
    }
    return result;
}

int generate_line_len(int max_line_len) {
    std::uniform_int_distribution<> distr(1, max_line_len);
    return distr(gen);
}

std::string generate_line(int line_length) {
    char line[line_length + 1];
    std::uniform_int_distribution<> char_dist(0, 25);
    for (size_t i = 0; i < line_length; ++i) {
        line[i] = letters[char_dist(gen)];
    }
    line[line_length] = '\0';
    return line;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        throw std::invalid_argument("Expected three arguments: <output file name> <number of lines> <max line length>");
    }
    std::string output_file_name = argv[1];
    std::ofstream output(output_file_name);

    int lines_number = read_arg(argv[2]);
    if (lines_number < 0) {
        throw std::invalid_argument("Number of lines wasn't expected to be negative: " + std::to_string(lines_number));
    }
    int max_line_len = read_arg(argv[3]);
    if (max_line_len < 0) {
        throw std::invalid_argument("Max line length wasn't expected to be negative: " + std::to_string(lines_number));
    }

    for (int line_num = 0; line_num < lines_number; ++line_num) {
        auto line_len = generate_line_len(max_line_len);
        std::string line = generate_line(line_len);
        output << line;
        if (line_num != lines_number - 1)
            output << std::endl;
    }

    return 0;
}
