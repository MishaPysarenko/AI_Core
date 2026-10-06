#include "simplepdf/Document.hpp"

#include <mupdf/fitz.h>

#include <cairo.h>
#include <cairo-pdf.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace simplepdf {
namespace {

struct TextCommand {
    double x;
    double y;
    std::string text;
    TextStyle style;
};

struct LineCommand {
    double x1;
    double y1;
    double x2;
    double y2;
    StrokeStyle style;
};

struct RectangleCommand {
    Rect rect;
    StrokeStyle stroke;
    Color fill;
};

struct ImageCommand {
    std::string path;
    Rect destination;
    bool preserveAspectRatio;
};

struct TableCommand {
    std::vector<std::vector<std::string>> cells;
    Rect destination;
    TableStyle style;
};

struct ChartCommand {
    std::vector<double> values;
    Rect destination;
    ChartStyle style;
    bool bars;
};

struct ReplaceTextCommand {
    Rect area;
    std::string text;
    TextStyle style;
    Color coverColor;
    double coverPadding;
};

using Command = std::variant<TextCommand, LineCommand, RectangleCommand,
                             ImageCommand, TableCommand, ChartCommand,
                             ReplaceTextCommand>;

struct PageData {
    int sourceIndex = -1;
    double sourceX = 0.0;
    double sourceY = 0.0;
    PageSize size;
    std::vector<Command> commands;
};

struct CairoSurfaceDeleter {
    void operator()(cairo_surface_t* surface) const noexcept {
        if (surface) {
            cairo_surface_destroy(surface);
        }
    }
};

struct CairoContextDeleter {
    void operator()(cairo_t* context) const noexcept {
        if (context) {
            cairo_destroy(context);
        }
    }
};

using SurfacePtr = std::unique_ptr<cairo_surface_t, CairoSurfaceDeleter>;
using CairoPtr = std::unique_ptr<cairo_t, CairoContextDeleter>;

[[noreturn]] void throwMuPdf(fz_context* context, std::string_view action) {
    const char* message = fz_caught_message(context);
    throw std::runtime_error(std::string(action) + ": " +
                             (message ? message : "unknown MuPDF error"));
}

void checkCairo(cairo_status_t status, std::string_view action) {
    if (status != CAIRO_STATUS_SUCCESS) {
        throw std::runtime_error(std::string(action) + ": " +
                                 cairo_status_to_string(status));
    }
}

void setColor(cairo_t* cr, const Color& color) {
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
}

void setFont(cairo_t* cr, const TextStyle& style) {
    const auto slant = style.italic ? CAIRO_FONT_SLANT_ITALIC
                                    : CAIRO_FONT_SLANT_NORMAL;
    const auto weight = style.bold ? CAIRO_FONT_WEIGHT_BOLD
                                   : CAIRO_FONT_WEIGHT_NORMAL;
    cairo_select_font_face(cr, style.fontFamily.c_str(), slant, weight);
    cairo_set_font_size(cr, style.fontSize);
}

void removeLastUtf8CodePoint(std::string& text) {
    if (text.empty()) {
        return;
    }

    std::size_t pos = text.size() - 1;
    while (pos > 0 &&
           (static_cast<unsigned char>(text[pos]) & 0xC0U) == 0x80U) {
        --pos;
    }
    text.erase(pos);
}

std::string fitText(cairo_t* cr, const std::string& text, double maxWidth) {
    if (maxWidth <= 0.0) {
        return {};
    }

    cairo_text_extents_t extents{};
    cairo_text_extents(cr, text.c_str(), &extents);
    if (extents.x_advance <= maxWidth) {
        return text;
    }

    std::string fitted = text;
    constexpr std::string_view suffix = "...";
    while (!fitted.empty()) {
        removeLastUtf8CodePoint(fitted);
        const std::string candidate = fitted + std::string(suffix);
        cairo_text_extents(cr, candidate.c_str(), &extents);
        if (extents.x_advance <= maxWidth) {
            return candidate;
        }
    }
    return {};
}

void drawText(cairo_t* cr, double x, double top,
              const std::string& text, const TextStyle& style,
              double maxWidth = std::numeric_limits<double>::infinity()) {
    setFont(cr, style);
    setColor(cr, style.color);

    double y = top + style.fontSize;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t newline = text.find('\n', start);
        const std::string line = text.substr(
            start, newline == std::string::npos ? std::string::npos
                                                : newline - start);
        const std::string visible = std::isfinite(maxWidth)
                                        ? fitText(cr, line, maxWidth)
                                        : line;
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, visible.c_str());
        if (newline == std::string::npos) {
            break;
        }
        start = newline + 1;
        y += style.fontSize * 1.2;
    }
}

void drawTable(cairo_t* cr, const TableCommand& table) {
    const std::size_t rows = table.cells.size();
    std::size_t columns = 0;
    for (const auto& row : table.cells) {
        columns = std::max(columns, row.size());
    }
    if (rows == 0 || columns == 0 || table.destination.width <= 0.0 ||
        table.destination.height <= 0.0) {
        return;
    }

    const double rowHeight = table.destination.height /
                             static_cast<double>(rows);
    const double columnWidth = table.destination.width /
                               static_cast<double>(columns);

    for (std::size_t row = 0; row < rows; ++row) {
        const bool header = table.style.firstRowIsHeader && row == 0;
        if (header || (row % 2U == 1U &&
                       table.style.alternateRowBackground.alpha > 0.0)) {
            setColor(cr, header ? table.style.headerBackground
                                : table.style.alternateRowBackground);
            cairo_rectangle(cr, table.destination.x,
                            table.destination.y + rowHeight * row,
                            table.destination.width, rowHeight);
            cairo_fill(cr);
        }
    }

    setColor(cr, table.style.gridColor);
    cairo_set_line_width(cr, table.style.gridWidth);
    for (std::size_t row = 0; row <= rows; ++row) {
        const double y = table.destination.y + rowHeight * row;
        cairo_move_to(cr, table.destination.x, y);
        cairo_line_to(cr, table.destination.x + table.destination.width, y);
    }
    for (std::size_t column = 0; column <= columns; ++column) {
        const double x = table.destination.x + columnWidth * column;
        cairo_move_to(cr, x, table.destination.y);
        cairo_line_to(cr, x, table.destination.y + table.destination.height);
    }
    cairo_stroke(cr);

    for (std::size_t row = 0; row < rows; ++row) {
        const TextStyle& textStyle =
            table.style.firstRowIsHeader && row == 0
                ? table.style.headerText
                : table.style.bodyText;
        for (std::size_t column = 0; column < table.cells[row].size(); ++column) {
            const double cellX = table.destination.x + columnWidth * column;
            const double cellY = table.destination.y + rowHeight * row;
            const double padding = table.style.cellPadding;

            cairo_save(cr);
            cairo_rectangle(cr, cellX, cellY, columnWidth, rowHeight);
            cairo_clip(cr);
            const double textTop = cellY +
                std::max(0.0, (rowHeight - textStyle.fontSize) * 0.5 - 1.0);
            drawText(cr, cellX + padding, textTop, table.cells[row][column],
                     textStyle, std::max(0.0, columnWidth - 2.0 * padding));
            cairo_restore(cr);
        }
    }
}

double mapValue(double value, double minimum, double maximum,
                double top, double height) {
    if (maximum == minimum) {
        return top + height * 0.5;
    }
    return top + height - (value - minimum) / (maximum - minimum) * height;
}

void drawChart(cairo_t* cr, const ChartCommand& chart) {
    if (chart.values.empty() || chart.destination.width <= 0.0 ||
        chart.destination.height <= 0.0) {
        return;
    }

    setColor(cr, chart.style.background);
    cairo_rectangle(cr, chart.destination.x, chart.destination.y,
                    chart.destination.width, chart.destination.height);
    cairo_fill(cr);

    constexpr double leftMargin = 28.0;
    constexpr double rightMargin = 10.0;
    constexpr double topMargin = 10.0;
    constexpr double bottomMargin = 22.0;

    const double plotX = chart.destination.x + leftMargin;
    const double plotY = chart.destination.y + topMargin;
    const double plotWidth = std::max(
        1.0, chart.destination.width - leftMargin - rightMargin);
    const double plotHeight = std::max(
        1.0, chart.destination.height - topMargin - bottomMargin);

    auto [minIt, maxIt] = std::minmax_element(chart.values.begin(),
                                              chart.values.end());
    double minimum = *minIt;
    double maximum = *maxIt;
    if (chart.bars) {
        minimum = std::min(0.0, minimum);
        maximum = std::max(0.0, maximum);
    }
    if (minimum == maximum) {
        minimum -= 1.0;
        maximum += 1.0;
    }

    if (chart.style.drawGrid) {
        setColor(cr, chart.style.grid);
        cairo_set_line_width(cr, 0.5);
        for (int i = 0; i <= 4; ++i) {
            const double y = plotY + plotHeight * i / 4.0;
            cairo_move_to(cr, plotX, y);
            cairo_line_to(cr, plotX + plotWidth, y);
        }
        cairo_stroke(cr);
    }

    setColor(cr, chart.style.axis);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, plotX, plotY, plotWidth, plotHeight);
    cairo_stroke(cr);

    setColor(cr, chart.style.foreground);
    cairo_set_line_width(cr, chart.style.lineWidth);

    if (chart.bars) {
        const double slot = plotWidth / static_cast<double>(chart.values.size());
        const double barWidth = slot * 0.72;
        const double zeroY = mapValue(0.0, minimum, maximum, plotY, plotHeight);
        for (std::size_t i = 0; i < chart.values.size(); ++i) {
            const double valueY = mapValue(chart.values[i], minimum, maximum,
                                           plotY, plotHeight);
            const double x = plotX + slot * i + (slot - barWidth) * 0.5;
            cairo_rectangle(cr, x, std::min(valueY, zeroY), barWidth,
                            std::max(0.5, std::abs(zeroY - valueY)));
            cairo_fill(cr);
        }
    } else {
        const double denominator = chart.values.size() > 1
                                       ? static_cast<double>(chart.values.size() - 1)
                                       : 1.0;
        for (std::size_t i = 0; i < chart.values.size(); ++i) {
            const double x = plotX + plotWidth * static_cast<double>(i) /
                                       denominator;
            const double y = mapValue(chart.values[i], minimum, maximum,
                                      plotY, plotHeight);
            if (i == 0) {
                cairo_move_to(cr, x, y);
            } else {
                cairo_line_to(cr, x, y);
            }
        }
        cairo_stroke(cr);
    }
}

class TemporaryPng {
public:
    TemporaryPng() {
        static std::atomic<unsigned long long> counter{0};
        const auto ticks = std::chrono::steady_clock::now()
                               .time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("simplepdf-" + std::to_string(ticks) + "-" +
                 std::to_string(counter.fetch_add(1)) + ".png");
    }

    ~TemporaryPng() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

std::filesystem::path normalizedAbsolutePath(const std::string& path) {
    return std::filesystem::absolute(std::filesystem::path(path))
        .lexically_normal();
}

Rect quadBounds(const fz_quad& quad) {
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    const fz_point points[] = {quad.ul, quad.ur, quad.ll, quad.lr};
    for (const auto& point : points) {
        minX = std::min(minX, static_cast<double>(point.x));
        minY = std::min(minY, static_cast<double>(point.y));
        maxX = std::max(maxX, static_cast<double>(point.x));
        maxY = std::max(maxY, static_cast<double>(point.y));
    }
    return {minX, minY, maxX - minX, maxY - minY};
}

} // namespace

class Document::Impl {
public:
    Impl() {
        context = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
        if (!context) {
            throw std::runtime_error("Cannot create MuPDF context");
        }
        fz_try(context) {
            fz_register_document_handlers(context);
        }
        fz_catch(context) {
            const char* caught = fz_caught_message(context);
            const std::string message = caught ? caught : "unknown MuPDF error";
            fz_drop_context(context);
            context = nullptr;
            throw std::runtime_error(
                "Cannot register MuPDF document handlers: " + message);
        }
    }

    ~Impl() {
        if (sourceDocument) {
            fz_drop_document(context, sourceDocument);
        }
        fz_drop_context(context);
    }

    void clear() {
        if (sourceDocument) {
            fz_drop_document(context, sourceDocument);
            sourceDocument = nullptr;
        }
        sourcePath.clear();
        pages.clear();
    }

    PageData& checkedPage(std::size_t index) {
        if (index >= pages.size()) {
            throw std::out_of_range("PDF page index is out of range");
        }
        return pages[index];
    }

    const PageData& checkedPage(std::size_t index) const {
        if (index >= pages.size()) {
            throw std::out_of_range("PDF page index is out of range");
        }
        return pages[index];
    }

    void renderDocumentPageToPng(fz_document* document, int pageIndex,
                                 double dpi,
                                 const std::filesystem::path& output) const {
        fz_page* page = nullptr;
        fz_pixmap* pixmap = nullptr;
        const std::string outputString = output.string();
        fz_var(page);
        fz_var(pixmap);

        fz_try(context) {
            page = fz_load_page(context, document, pageIndex);
            const float scale = static_cast<float>(dpi / 72.0);
            pixmap = fz_new_pixmap_from_page(
                context, page, fz_scale(scale, scale),
                fz_device_rgb(context), 0);
            fz_save_pixmap_as_png(context, pixmap, outputString.c_str());
        }
        fz_always(context) {
            fz_drop_pixmap(context, pixmap);
            fz_drop_page(context, page);
        }
        fz_catch(context) {
            throwMuPdf(context, "Cannot render a page");
        }
    }

    SurfacePtr loadImage(const std::string& imagePath) const {
        SurfacePtr image(cairo_image_surface_create_from_png(imagePath.c_str()));
        if (cairo_surface_status(image.get()) == CAIRO_STATUS_SUCCESS) {
            return image;
        }
        image.reset();

        fz_document* imageDocument = nullptr;
        fz_var(imageDocument);
        fz_try(context) {
            imageDocument = fz_open_document(context, imagePath.c_str());
        }
        fz_catch(context) {
            throwMuPdf(context, "Cannot open image");
        }

        TemporaryPng converted;
        try {
            renderDocumentPageToPng(imageDocument, 0, 72.0, converted.path());
        } catch (...) {
            fz_drop_document(context, imageDocument);
            throw;
        }
        fz_drop_document(context, imageDocument);

        image.reset(cairo_image_surface_create_from_png(
            converted.path().string().c_str()));
        checkCairo(cairo_surface_status(image.get()), "Cannot decode image");
        return image;
    }

    void drawImage(cairo_t* cr, const ImageCommand& command) const {
        SurfacePtr image = loadImage(command.path);
        const int imageWidth = cairo_image_surface_get_width(image.get());
        const int imageHeight = cairo_image_surface_get_height(image.get());
        if (imageWidth <= 0 || imageHeight <= 0 ||
            command.destination.width <= 0.0 ||
            command.destination.height <= 0.0) {
            return;
        }

        double width = command.destination.width;
        double height = command.destination.height;
        double x = command.destination.x;
        double y = command.destination.y;
        if (command.preserveAspectRatio) {
            const double scale = std::min(
                width / static_cast<double>(imageWidth),
                height / static_cast<double>(imageHeight));
            width = imageWidth * scale;
            height = imageHeight * scale;
            x += (command.destination.width - width) * 0.5;
            y += (command.destination.height - height) * 0.5;
        }

        cairo_save(cr);
        cairo_rectangle(cr, command.destination.x, command.destination.y,
                        command.destination.width, command.destination.height);
        cairo_clip(cr);
        cairo_translate(cr, x, y);
        cairo_scale(cr, width / imageWidth, height / imageHeight);
        cairo_set_source_surface(cr, image.get(), 0.0, 0.0);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    void drawCommand(cairo_t* cr, const Command& command) const {
        std::visit(
            [&](const auto& item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, TextCommand>) {
                    drawText(cr, item.x, item.y, item.text, item.style);
                } else if constexpr (std::is_same_v<T, LineCommand>) {
                    setColor(cr, item.style.color);
                    cairo_set_line_width(cr, item.style.width);
                    cairo_move_to(cr, item.x1, item.y1);
                    cairo_line_to(cr, item.x2, item.y2);
                    cairo_stroke(cr);
                } else if constexpr (std::is_same_v<T, RectangleCommand>) {
                    cairo_rectangle(cr, item.rect.x, item.rect.y,
                                    item.rect.width, item.rect.height);
                    if (item.fill.alpha > 0.0) {
                        setColor(cr, item.fill);
                        cairo_fill_preserve(cr);
                    }
                    if (item.stroke.width > 0.0 &&
                        item.stroke.color.alpha > 0.0) {
                        setColor(cr, item.stroke.color);
                        cairo_set_line_width(cr, item.stroke.width);
                        cairo_stroke(cr);
                    } else {
                        cairo_new_path(cr);
                    }
                } else if constexpr (std::is_same_v<T, ImageCommand>) {
                    drawImage(cr, item);
                } else if constexpr (std::is_same_v<T, TableCommand>) {
                    drawTable(cr, item);
                } else if constexpr (std::is_same_v<T, ChartCommand>) {
                    drawChart(cr, item);
                } else if constexpr (std::is_same_v<T, ReplaceTextCommand>) {
                    const double padding = std::max(0.0, item.coverPadding);
                    setColor(cr, item.coverColor);
                    cairo_rectangle(cr, item.area.x - padding,
                                    item.area.y - padding,
                                    item.area.width + 2.0 * padding,
                                    item.area.height + 2.0 * padding);
                    cairo_fill(cr);
                    drawText(cr, item.area.x, item.area.y,
                             item.text, item.style);
                }
            },
            command);
    }

    fz_context* context = nullptr;
    fz_document* sourceDocument = nullptr;
    std::filesystem::path sourcePath;
    std::vector<PageData> pages;
};

Document::Document() : impl_(std::make_unique<Impl>()) {}

Document::Document(const std::string& inputPath) : Document() {
    open(inputPath);
}

Document::~Document() = default;
Document::Document(Document&&) noexcept = default;
Document& Document::operator=(Document&&) noexcept = default;

void Document::open(const std::string& inputPath) {
    if (inputPath.empty()) {
        throw std::invalid_argument("Input path is empty");
    }

    fz_document* newDocument = nullptr;
    fz_var(newDocument);
    fz_try(impl_->context) {
        newDocument = fz_open_document(impl_->context, inputPath.c_str());
    }
    fz_catch(impl_->context) {
        throwMuPdf(impl_->context, "Cannot open document");
    }

    std::vector<PageData> newPages;
    try {
        int count = 0;
        fz_var(count);
        fz_try(impl_->context) {
            count = fz_count_pages(impl_->context, newDocument);
        }
        fz_catch(impl_->context) {
            throwMuPdf(impl_->context, "Cannot count document pages");
        }

        newPages.reserve(static_cast<std::size_t>(std::max(0, count)));
        for (int index = 0; index < count; ++index) {
            fz_page* page = nullptr;
            fz_rect bounds{};
            fz_var(page);
            fz_var(bounds);
            fz_try(impl_->context) {
                page = fz_load_page(impl_->context, newDocument, index);
                bounds = fz_bound_page(impl_->context, page);
            }
            fz_always(impl_->context) {
                fz_drop_page(impl_->context, page);
            }
            fz_catch(impl_->context) {
                throwMuPdf(impl_->context, "Cannot read page size");
            }

            PageData data;
            data.sourceIndex = index;
            data.sourceX = bounds.x0;
            data.sourceY = bounds.y0;
            data.size.width = std::max(1.0, static_cast<double>(bounds.x1 - bounds.x0));
            data.size.height = std::max(1.0, static_cast<double>(bounds.y1 - bounds.y0));
            newPages.push_back(std::move(data));
        }
    } catch (...) {
        fz_drop_document(impl_->context, newDocument);
        throw;
    }

    impl_->clear();
    impl_->sourceDocument = newDocument;
    impl_->sourcePath = normalizedAbsolutePath(inputPath);
    impl_->pages = std::move(newPages);
}

void Document::create() {
    impl_->clear();
}

std::size_t Document::pageCount() const noexcept {
    return impl_->pages.size();
}

PageSize Document::pageSize(std::size_t pageIndex) const {
    return impl_->checkedPage(pageIndex).size;
}

std::string Document::extractText(std::size_t pageIndex) const {
    const PageData& pageData = impl_->checkedPage(pageIndex);
    if (pageData.sourceIndex < 0 || !impl_->sourceDocument) {
        return {};
    }

    fz_page* page = nullptr;
    fz_buffer* buffer = nullptr;
    fz_var(page);
    fz_var(buffer);
    fz_try(impl_->context) {
        page = fz_load_page(impl_->context, impl_->sourceDocument,
                            pageData.sourceIndex);
        buffer = fz_new_buffer_from_page(impl_->context, page, nullptr);
    }
    fz_catch(impl_->context) {
        fz_drop_buffer(impl_->context, buffer);
        fz_drop_page(impl_->context, page);
        throwMuPdf(impl_->context, "Cannot extract page text");
    }

    unsigned char* bytes = nullptr;
    const std::size_t size = fz_buffer_storage(impl_->context, buffer, &bytes);
    std::string result;
    if (bytes && size > 0) {
        result.assign(reinterpret_cast<const char*>(bytes), size);
    }
    fz_drop_buffer(impl_->context, buffer);
    fz_drop_page(impl_->context, page);
    return result;
}

std::vector<Rect> Document::findText(std::size_t pageIndex,
                                     const std::string& text,
                                     std::size_t maxHits) const {
    const PageData& pageData = impl_->checkedPage(pageIndex);
    if (text.empty() || maxHits == 0 || pageData.sourceIndex < 0 ||
        !impl_->sourceDocument) {
        return {};
    }

    const int capacity = static_cast<int>(std::min<std::size_t>(
        maxHits, static_cast<std::size_t>(std::numeric_limits<int>::max())));
    std::vector<fz_quad> quads(static_cast<std::size_t>(capacity));
    std::vector<int> marks(static_cast<std::size_t>(capacity));
    int count = 0;
    fz_page* page = nullptr;
    fz_var(count);
    fz_var(page);
    fz_try(impl_->context) {
        page = fz_load_page(impl_->context, impl_->sourceDocument,
                            pageData.sourceIndex);
        count = fz_search_page(impl_->context, page, text.c_str(),
                               marks.data(), quads.data(), capacity);
    }
    fz_always(impl_->context) {
        fz_drop_page(impl_->context, page);
    }
    fz_catch(impl_->context) {
        throwMuPdf(impl_->context, "Cannot search page text");
    }

    std::vector<Rect> result;
    result.reserve(static_cast<std::size_t>(std::max(0, count)));
    for (int i = 0; i < count; ++i) {
        Rect rect = quadBounds(quads[static_cast<std::size_t>(i)]);
        rect.x -= pageData.sourceX;
        rect.y -= pageData.sourceY;
        result.push_back(rect);
    }
    return result;
}

std::size_t Document::addPage(PageSize size) {
    if (size.width <= 0.0 || size.height <= 0.0) {
        throw std::invalid_argument("Page size must be positive");
    }
    PageData page;
    page.size = size;
    impl_->pages.push_back(std::move(page));
    return impl_->pages.size() - 1;
}

void Document::removePage(std::size_t pageIndex) {
    impl_->checkedPage(pageIndex);
    impl_->pages.erase(impl_->pages.begin() +
                       static_cast<std::ptrdiff_t>(pageIndex));
}

std::size_t Document::replaceText(std::size_t pageIndex,
                                  const std::string& oldText,
                                  const std::string& newText,
                                  const TextStyle& style,
                                  Color coverColor,
                                  bool replaceAll,
                                  double coverPadding) {
    auto matches = findText(pageIndex, oldText, replaceAll ? 256 : 1);
    auto& page = impl_->checkedPage(pageIndex);
    for (const Rect& match : matches) {
        page.commands.emplace_back(ReplaceTextCommand{
            match, newText, style, coverColor, coverPadding});
    }
    return matches.size();
}

void Document::addText(std::size_t pageIndex, double x, double y,
                       std::string text, TextStyle style) {
    impl_->checkedPage(pageIndex).commands.emplace_back(
        TextCommand{x, y, std::move(text), std::move(style)});
}

void Document::addLine(std::size_t pageIndex,
                       double x1, double y1, double x2, double y2,
                       StrokeStyle style) {
    impl_->checkedPage(pageIndex).commands.emplace_back(
        LineCommand{x1, y1, x2, y2, style});
}

void Document::addRectangle(std::size_t pageIndex, Rect rect,
                            StrokeStyle stroke, Color fill) {
    impl_->checkedPage(pageIndex).commands.emplace_back(
        RectangleCommand{rect, stroke, fill});
}

void Document::addImage(std::size_t pageIndex, std::string imagePath,
                        Rect destination, bool preserveAspectRatio) {
    impl_->checkedPage(pageIndex).commands.emplace_back(
        ImageCommand{std::move(imagePath), destination, preserveAspectRatio});
}

void Document::addTable(std::size_t pageIndex,
                        std::vector<std::vector<std::string>> cells,
                        Rect destination, TableStyle style) {
    impl_->checkedPage(pageIndex).commands.emplace_back(
        TableCommand{std::move(cells), destination, std::move(style)});
}

void Document::addLineChart(std::size_t pageIndex,
                            std::vector<double> values,
                            Rect destination, ChartStyle style) {
    impl_->checkedPage(pageIndex).commands.emplace_back(
        ChartCommand{std::move(values), destination, style, false});
}

void Document::addBarChart(std::size_t pageIndex,
                           std::vector<double> values,
                           Rect destination, ChartStyle style) {
    impl_->checkedPage(pageIndex).commands.emplace_back(
        ChartCommand{std::move(values), destination, style, true});
}

void Document::save(const std::string& outputPath, double rasterDpi) const {
    if (outputPath.empty()) {
        throw std::invalid_argument("Output path is empty");
    }
    if (impl_->pages.empty()) {
        throw std::runtime_error("Cannot save a PDF without pages");
    }
    if (rasterDpi <= 0.0) {
        throw std::invalid_argument("Raster DPI must be positive");
    }

    const auto normalizedOutput = normalizedAbsolutePath(outputPath);
    if (!impl_->sourcePath.empty() && normalizedOutput == impl_->sourcePath) {
        throw std::invalid_argument(
            "Output path must differ from the opened input path; use save-as");
    }

    SurfacePtr output(cairo_pdf_surface_create(outputPath.c_str(), 1.0, 1.0));
    checkCairo(cairo_surface_status(output.get()), "Cannot create output PDF");

    for (const PageData& page : impl_->pages) {
        cairo_pdf_surface_set_size(output.get(), page.size.width,
                                  page.size.height);
        CairoPtr cr(cairo_create(output.get()));
        checkCairo(cairo_status(cr.get()), "Cannot create Cairo context");

        setColor(cr.get(), Color::white());
        cairo_paint(cr.get());

        if (page.sourceIndex >= 0 && impl_->sourceDocument) {
            TemporaryPng rendered;
            impl_->renderDocumentPageToPng(impl_->sourceDocument,
                                           page.sourceIndex, rasterDpi,
                                           rendered.path());
            SurfacePtr background(cairo_image_surface_create_from_png(
                rendered.path().string().c_str()));
            checkCairo(cairo_surface_status(background.get()),
                       "Cannot load rendered page");

            const int imageWidth = cairo_image_surface_get_width(background.get());
            const int imageHeight = cairo_image_surface_get_height(background.get());
            if (imageWidth > 0 && imageHeight > 0) {
                cairo_save(cr.get());
                cairo_scale(cr.get(), page.size.width / imageWidth,
                            page.size.height / imageHeight);
                cairo_set_source_surface(cr.get(), background.get(), 0.0, 0.0);
                cairo_paint(cr.get());
                cairo_restore(cr.get());
            }
        }

        for (const Command& command : page.commands) {
            impl_->drawCommand(cr.get(), command);
        }

        checkCairo(cairo_status(cr.get()), "Cannot draw PDF page");
        cairo_show_page(cr.get());
    }

    cairo_surface_finish(output.get());
    checkCairo(cairo_surface_status(output.get()), "Cannot finish output PDF");
}

} // namespace simplepdf
