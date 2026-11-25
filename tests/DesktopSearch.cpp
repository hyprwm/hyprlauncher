#include "finders/desktop/Search.hpp"
#include <cstdlib>
#include <iostream>

class Entry : public IFinderResult {
  public:
    Entry(std::string name, std::vector<std::string> aliases = {}, uint32_t frequency = 0) : title(std::move(name)), aliases(std::move(aliases)), count(frequency) {}
    eFinderTypes type() override {
        return FINDER_DESKTOP;
    }
    void                            run() override {}
    const std::vector<std::string>& fuzzables() override {
        return aliases;
    }
    const std::string& name() override {
        return title;
    }
    uint32_t frequency() override {
        return count;
    }
    std::string              title;
    std::vector<std::string> aliases;
    uint32_t                 count;
};
static void expect(const std::vector<SP<IFinderResult>>& entries, const std::string& query, std::vector<std::string> names, size_t limit = 50) {
    std::vector<std::string> actual;
    for (const auto& result : DesktopSearch::match(entries, query, limit))
        actual.push_back(result->name());
    if (actual != names) {
        std::cerr << "Unexpected results for " << query << '\n';
        for (const auto& name : actual)
            std::cerr << name << '\n';
        std::exit(1);
    }
}
int main() {
    std::vector<SP<IFinderResult>> entries{makeShared<Entry>("RetroArch"),
                                           makeShared<Entry>("Qt V4L2 video capture utility"),
                                           makeShared<Entry>("Volume Control", std::vector<std::string>{"pavucontrol", "Audio", "Mixer"}, 999),
                                           makeShared<Entry>("Brave"),
                                           makeShared<Entry>("Recorder"),
                                           makeShared<Entry>("Screen Recorder"),
                                           makeShared<Entry>("Éditeur"),
                                           makeShared<Entry>("Straße")};
    expect(entries, "RetroArch", {"RetroArch"});
    expect(entries, "  RETROARCH  ", {"RetroArch"});
    expect(entries, "retrarch", {"RetroArch"});
    expect(entries, "retorarch", {"RetroArch"});
    expect(entries, "Re", {"Recorder", "RetroArch", "Qt V4L2 video capture utility", "Screen Recorder"});
    expect(entries, "R", {"Recorder", "RetroArch", "Brave", "Qt V4L2 video capture utility", "Screen Recorder", "Straße", "Volume Control", "Éditeur"});
    expect(entries, "pavucontrol", {"Volume Control"});
    expect(entries, "voluem", {"Volume Control"});
    expect(entries, "nonsensezzzz", {});
    expect(entries, "zx", {});
    expect(entries, "ÉDI", {"Éditeur"});
    expect(entries, "E\u0301di", {"Éditeur"});
    expect(entries, "strasse", {"Straße"});
    expect(entries, "", {"Volume Control"}, 1);
    expect(entries, "re", {}, 0);
    // Literal matches suppress typo suggestions, even for a popular app.
    auto extended = entries;
    extended.push_back(makeShared<Entry>("Retrarch Tools"));
    expect(extended, "retrarch", {"Retrarch Tools"});
    // Full case folding, canonical normalization and code-point edit budgets.
    expect({makeShared<Entry>("ΟΣ")}, "ος", {"ΟΣ"});
    expect({makeShared<Entry>("Cafe\u0301")}, "CAFÉ", {"Cafe\u0301"});
    expect({makeShared<Entry>("\U00010400tool")}, "\U00010428TOOL", {"\U00010400tool"});
    expect({makeShared<Entry>("猫犬鳥魚")}, "猫犬鳥馬", {"猫犬鳥魚"});
    expect({makeShared<Entry>("猫犬鳥")}, "猫犬馬", {});
    expect({makeShared<Entry>("①")}, "1", {});
    expect({makeShared<Entry>(std::string("\xff") + "Foo")}, "Foo", {});
    expect({makeShared<Entry>(std::string("Foo\0Bar", 7))}, "Foo", {});
    std::cout << "Desktop search regression checks passed\n";
}
