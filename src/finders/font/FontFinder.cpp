#include "FontFinder.hpp"
#include "../../helpers/Log.hpp"
#include "../Fuzzy.hpp"

#include <hyprutils/os/Process.hpp>
#include <fontconfig/fontconfig.h>

#include <algorithm>

using namespace Hyprutils::OS;

class CFontEntry : public IFinderResult {
  public:
    CFontEntry()          = default;
    virtual ~CFontEntry() = default;

    virtual const std::string& fuzzable() {
        return m_fuzzable;
    }

    virtual eFinderTypes type() {
        return FINDER_MATH;
    }

    virtual const std::string& name() {
        return m_font;
    }

    virtual void run() {
        Debug::log(TRACE, "Copying {} with wl-copy", m_font);

        CProcess proc("wl-copy", {m_font});
        proc.runAsync();
    }

    std::string m_font, m_fuzzable;
};

CFontFinder::CFontFinder() : m_valid(FcInit()) {
    if (!m_valid)
        Debug::log(ERR, "font: FcInit failed, fonts unavailable");

    refreshFonts();
}

CFontFinder::~CFontFinder() {
    FcFini();
}

void CFontFinder::refreshFonts() {
    if (!m_valid)
        return;

    m_entries.clear();
    m_entriesGeneric.clear();

    FcPattern*   pattern   = FcPatternCreate();
    FcObjectSet* objectSet = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, nullptr);
    FcFontSet*   fontSet   = FcFontList(nullptr, pattern, objectSet);

    if (fontSet) {
        for (int i = 0; i < fontSet->nfont; i++) {
            FcPattern* font   = fontSet->fonts[i];
            FcChar8 *  family = nullptr, *style = nullptr;

            if (FcPatternGetString(font, FC_FAMILY, 0, &family) != FcResultMatch || FcPatternGetString(font, FC_STYLE, 0, &style) != FcResultMatch)
                continue;

            if (!family || !style)
                continue;

            auto e        = m_entries.emplace_back(makeShared<CFontEntry>());
            e->m_font     = std::format("{} {}", (char*)family, (char*)style);
            e->m_fuzzable = e->m_font;
            std::ranges::transform(e->m_fuzzable, e->m_fuzzable.begin(), ::tolower);
            m_entriesGeneric.emplace_back(std::move(e));
        }
    }

    FcFontSetDestroy(fontSet);
    FcObjectSetDestroy(objectSet);
    FcPatternDestroy(pattern);
}

std::vector<SFinderResult> CFontFinder::getResultsForQuery(const std::string& query) {
    std::vector<SFinderResult>     results;

    std::vector<SP<IFinderResult>> fuzzed;
    if (!query.empty())
        fuzzed = Fuzzy::getNResults(m_entriesGeneric, query, MAX_RESULTS_PER_FINDER);
    else
        fuzzed = std::vector<SP<IFinderResult>>{m_entriesGeneric.begin(), m_entriesGeneric.begin() + std::min(m_entriesGeneric.size(), MAX_RESULTS_PER_FINDER)};

    results.reserve(fuzzed.size());

    for (const auto& f : fuzzed) {
        const auto p = reinterpretPointerCast<CFontEntry>(f);
        if (!p)
            continue;
        results.emplace_back(SFinderResult{
            .label        = p->m_font,
            .icon         = "",
            .result       = p,
            .overrideFont = p->m_font,
        });
    }

    return results;
}
