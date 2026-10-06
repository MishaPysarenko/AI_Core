#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace simplepdf {

struct Color {
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    double alpha = 1.0;

    static constexpr Color rgb(double r, double g, double b,
                               double a = 1.0) noexcept {
        return {r, g, b, a};
    }

    static constexpr Color black() noexcept { return {0.0, 0.0, 0.0, 1.0}; }
    static constexpr Color white() noexcept { return {1.0, 1.0, 1.0, 1.0}; }
};

struct Rect {
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct PageSize {
    double width = 595.28;   // A4, PDF points
    double height = 841.89;
};

struct TextStyle {
    std::string fontFamily = "Arial";
    double fontSize = 12.0;
    Color color = Color::black();
    bool bold = false;
    bool italic = false;
};

struct StrokeStyle {
    Color color = Color::black();
    double width = 1.0;
};

struct TableStyle {
    TextStyle headerText{"Arial", 10.0, Color::white(), true, false};
    TextStyle bodyText{"Arial", 10.0, Color::black(), false, false};
    Color headerBackground = Color::rgb(0.16, 0.32, 0.55);
    Color alternateRowBackground = Color::rgb(0.95, 0.96, 0.98);
    Color gridColor = Color::rgb(0.55, 0.55, 0.55);
    double gridWidth = 0.75;
    double cellPadding = 4.0;
    bool firstRowIsHeader = true;
};

struct ChartStyle {
    Color foreground = Color::rgb(0.12, 0.42, 0.75);
    Color axis = Color::rgb(0.25, 0.25, 0.25);
    Color grid = Color::rgb(0.85, 0.85, 0.85);
    Color background = Color::white();
    double lineWidth = 2.0;
    bool drawGrid = true;
};

class Document {
public:
    Document();
    explicit Document(const std::string& inputPath);
    ~Document();

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;

    // Clears the current state and opens a PDF or another MuPDF-supported file.
    void open(const std::string& inputPath);

    // Clears the current state and starts a new empty PDF.
    void create();

    [[nodiscard]] std::size_t pageCount() const noexcept;
    [[nodiscard]] PageSize pageSize(std::size_t pageIndex) const;
    [[nodiscard]] std::string extractText(std::size_t pageIndex) const;
    [[nodiscard]] std::vector<Rect> findText(
        std::size_t pageIndex,
        const std::string& text,
        std::size_t maxHits = 256) const;

    std::size_t addPage(PageSize size = {});
    void removePage(std::size_t pageIndex);

    // Replaces matches visually: covers the old text and writes new text.
    // Existing source pages are rasterized during save().
    std::size_t replaceText(
        std::size_t pageIndex,
        const std::string& oldText,
        const std::string& newText,
        const TextStyle& style = {},
        Color coverColor = Color::white(),
        bool replaceAll = true,
        double coverPadding = 1.0);

    void addText(std::size_t pageIndex, double x, double y,
                 std::string text, TextStyle style = {});
    void addLine(std::size_t pageIndex,
                 double x1, double y1, double x2, double y2,
                 StrokeStyle style = {});
    void addRectangle(std::size_t pageIndex, Rect rect,
                      StrokeStyle stroke = {},
                      Color fill = Color::rgb(0.0, 0.0, 0.0, 0.0));
    void addImage(std::size_t pageIndex, std::string imagePath,
                  Rect destination, bool preserveAspectRatio = true);
    void addTable(std::size_t pageIndex,
                  std::vector<std::vector<std::string>> cells,
                  Rect destination, TableStyle style = {});
    void addLineChart(std::size_t pageIndex,
                      std::vector<double> values,
                      Rect destination, ChartStyle style = {});
    void addBarChart(std::size_t pageIndex,
                     std::vector<double> values,
                     Rect destination, ChartStyle style = {});

    // outputPath must differ from the currently opened input path.
    // Original pages are rendered at rasterDpi; new Cairo content remains vector.
    void save(const std::string& outputPath, double rasterDpi = 144.0) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace simplepdf
