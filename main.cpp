#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <cmath>

size_t read_arg(const std::string& arg) {
    size_t result;
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

size_t BLOCK_SIZE = 100000;

std::string temp_file_name(size_t fileno) {
    return "__dump__" + std::to_string(fileno) + ".txt";
}

class Line : public std::string {
    friend std::istream & operator>>(std::istream & is, Line & line) {
        return std::getline(is, line);
    }
};

typedef std::pair<Line, int> h_node;
bool comp(const h_node &x, const h_node &y) {
    return x.first > y.first;
}

/**
 * Merges input files sorted by sentences in lexicographic order
 * into one sorted output file
 * given their input streams and output stream respectively
 * @param inputs - vector of input streams
 * @param output - output stream
 */
void k_merge(std::vector<std::istream *> &inputs, std::ostream &output) {
    std::vector<h_node> heap;

    Line line;
    for (size_t i = 0; i < inputs.size(); ++i) {
        auto &input = inputs[i];
        if (*input >> line) {
            heap.emplace_back(line, i);
        }
    }
    std::make_heap(heap.begin(), heap.end(), &comp);

    while (!heap.empty()) {
        std::pop_heap(heap.begin(), heap.end(), &comp);

        auto min = heap.back();
        auto idx = min.second;
        auto &input = inputs[idx];
        if (*input >> line) {
            heap[heap.size() - 1] = std::make_pair(line, idx);
            std::push_heap(heap.begin(), heap.end(), &comp);
        } else {
            heap.pop_back();
        }
        if (!heap.empty()) {
            output << min.first << std::endl;
        } else {
            output << min.first;
        }
    }
}

/**
 * Given buffer of sentences in memory
 * writes it to a temporary file on disk
 * @param buffer - buffer of sentences
 * @param curr_fileno - temporary file number
 */
void dump_buffer(std::vector<std::string>& buffer, size_t curr_fileno) {
    std::ofstream output(temp_file_name(curr_fileno));
    for (size_t i = 0; i < buffer.size() - 1; ++i) {
        output << buffer[i] << std::endl;
    }
    if (!buffer.empty()) {
        output << buffer[buffer.size() - 1];
    }
    buffer.clear();
}

/**
 * Given name of input file and available memory (in bytes),
 * sorts it by sentences in lexicographic order (using merge sort algorithm with k-way merge)
 * and writes result to output file
 * @param input_file_name
 * @param output_file_name
 * @param AVAILABLE_MEMORY
 */
void sort_lines(std::string& input_file_name, std::string& output_file_name, size_t AVAILABLE_MEMORY) {
    size_t k = AVAILABLE_MEMORY / BLOCK_SIZE - 1;

    size_t memory_used = 0;
    size_t curr_dumps = 0;
    std::vector<std::string> buffer;
    std::ifstream input_fs(input_file_name.c_str());
    Line line;
    // Read input file line by line into buffer until the total amount of used memory
    // exceeds AVAILABLE_MEMORY,
    // then sort buffer and dump it on a disk
    while (input_fs >> line) {
        if (memory_used + line.size() > AVAILABLE_MEMORY) {
            std::sort(buffer.begin(), buffer.end());
            dump_buffer(buffer, curr_dumps++);
            memory_used = 0;
        }
        memory_used += line.size();
        buffer.push_back(line);
    }
    std::sort(buffer.begin(), buffer.end());
    dump_buffer(buffer, curr_dumps++);

    // merge sorted pieces of input file using k-way merge
    // until one piece remains
    size_t lbound = 0, rbound = curr_dumps;
    while (lbound + 1 < rbound) {
        for (size_t i = lbound; i < rbound; i += k) {
            std::vector<std::istream *> inputs;
            for (size_t j = i; j < std::min(i + k, rbound); ++j) {
                auto input = new std::ifstream();
                input->open(temp_file_name(j));
                inputs.emplace_back(input);
            }
            std::ofstream output(temp_file_name(curr_dumps++));

            k_merge(inputs, output);

            for (auto input: inputs) {
                ((std::ifstream *) input)->close();
                delete input;
            }
            for (size_t j = i; j < std::min(i + k, rbound); ++j) {
                std::remove(temp_file_name(j).c_str());
            }
        }
        lbound = rbound;
        rbound = curr_dumps;
    }
    // rename last temporary file to <output file name>
    std::rename(temp_file_name(lbound).c_str(), output_file_name.c_str());

}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        throw std::invalid_argument("Expected three arguments: <input file name> <output file name> <available memory>");
    }
    std::string input_file_name = argv[1];
    std::string output_file_name = argv[2];

    size_t available_memory = read_arg(argv[3]);
    if (available_memory <  3 * BLOCK_SIZE) {
        throw std::invalid_argument("Available memory wasn't expected to be less then 30 Mb: "
                                                                + std::to_string(available_memory));
    }

    auto start = std::chrono::steady_clock::now();
    sort_lines(input_file_name, output_file_name, available_memory);
    auto end = std::chrono::steady_clock::now();

    std::cout << "Elapsed time: "
         << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
         << " ms" << std::endl;
    return 0;
}
