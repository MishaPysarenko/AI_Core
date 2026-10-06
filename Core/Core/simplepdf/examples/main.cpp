#include "simplepdf/Document.hpp"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    try {
        simplepdf::Document pdf;

        if (argc >= 2) {
            pdf.open(argv[1]);
            std::cout << "Pages: " << pdf.pageCount() << '\n';
            if (pdf.pageCount() > 0) {
                std::cout << pdf.extractText(0) << '\n';

                simplepdf::TextStyle replacement;
                replacement.fontFamily = "Arial";
                replacement.fontSize = 12.0;
                replacement.color = simplepdf::Color::black();

                const auto replaced = pdf.replaceText(
                    0, "Old text", "New text", replacement);
                std::cout << "Replaced: " << replaced << '\n';
            }
        } else {
            pdf.create();
            pdf.addPage();
            pdf.addText(0, 48.0, 36.0, "SimplePDF example",
                        {"Arial", 20.0, simplepdf::Color::black(), true, false});
        }

        if (pdf.pageCount() == 0) {
            pdf.addPage();
        }

        const std::size_t page = pdf.pageCount() - 1;
        pdf.addTable(
            page,
            {{"Parameter", "Value"},
             {"Voltage", "3.3 V"},
             {"Current", "250 mA"}},
            {48.0, 90.0, 300.0, 110.0});

        pdf.addLineChart(page, {2.0, 4.5, 3.2, 7.0, 6.1, 9.0},
                         {48.0, 230.0, 300.0, 180.0});
        pdf.addBarChart(page, {5.0, 8.0, 3.0, 6.0},
                        {48.0, 440.0, 300.0, 180.0});

        const std::string output = argc >= 3 ? argv[2] : "result.pdf";
        pdf.save(output);
        std::cout << "Saved: " << output << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

