#include <functional>
#include <argument_parser.h>
#include <text_parser_lib.h>

#include <iostream>
#include <fstream>
#include <numeric>
#include <chrono>

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    std::string inputPath;
    std::string outputPath;

    ArgumentParser::ArgParser parser("Program");

    parser.AddFlag('t', "test", "enable testing");
    parser.AddStringArgument('p', "path", "path to input file").StoreValue(inputPath);
    parser.AddStringArgument('o', "output", "path to output file").StoreValue(outputPath);
    parser.AddHelp('h', "help", "Program accumulates arguments");

    if (!parser.Parse(argc, argv)) {
        std::cerr << "Wrong argument" << std::endl;
        std::cout << parser.HelpDescription() << std::endl;
        return 1;
    }

    if (parser.Help()) {
        std::cout << parser.HelpDescription() << std::endl;
        return 0;
    }

    if (parser.GetFlag("test")) {
        if (inputPath.empty() || outputPath.empty()) {
            std::cerr << "Input or output path is empty." << std::endl;
            std::cout << parser.HelpDescription() << std::endl;
            return 1;
        }

        try {
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile) {
                throw std::runtime_error("Failed to open output file: " + outputPath);
            }

            FileParser::FileBlockIterator begin(inputPath, 4096);
            FileParser::FileBlockIterator end(inputPath, 4096, true);
            auto start = std::chrono::steady_clock::now();

            while (begin != end) {
                outFile.write(begin->data(), begin->size());
                ++begin;
            }

            outFile.close();
            auto endTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = endTime - start;

            std::cout << "Data successfully written to: " << outputPath << std::endl;
            std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;

        } catch (const std::exception& ex) {
            std::cerr << "Exception: " << ex.what() << std::endl;
            return 1;
        }
    }

    return 0;
}
