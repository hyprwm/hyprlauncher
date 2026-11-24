#include "helpers/Icons.hpp"
#include <cstdlib>
#include <iostream>

using namespace Hyprutils::Memory;

static void check(bool condition) {
    if (!condition) {
        std::cerr << "Icon regression check failed\n";
        std::exit(1);
    }
}

class CTestIcon : public Hyprtoolkit::ISystemIconDescription {
  public:
    explicit CTestIcon(bool valid) : m_valid(valid) {}
    bool exists() override {
        return m_valid;
    }
    bool scalable() override {
        return true;
    }

  private:
    bool m_valid = false;
};

class CTestFactory : public Hyprtoolkit::ISystemIconFactory {
  public:
    CSharedPointer<Hyprtoolkit::ISystemIconDescription> lookupIcon(const std::string& name) override {
        m_requests.push_back(name);
        return makeShared<CTestIcon>(name == "app" || name == "utilities-terminal");
    }
    std::vector<std::string> m_requests;
};

int main() {
    CTestFactory                   factory;
    const std::vector<std::string> FALLBACKS{"absent", "utilities-terminal"};
    check(!applicationIcon(factory, "app", FALLBACKS, false));
    check(factory.m_requests.empty());
    check(bool(applicationIcon(factory, "app", FALLBACKS, true)));
    check(factory.m_requests == std::vector<std::string>{"app"});
    factory.m_requests.clear();
    check(bool(applicationIcon(factory, "broken", FALLBACKS, true)));
    check(factory.m_requests == std::vector<std::string>({"broken", "absent", "utilities-terminal"}));
    factory.m_requests.clear();
    check(bool(applicationIcon(factory, "", FALLBACKS, true)));
    check(factory.m_requests == FALLBACKS);
    check(!applicationIcon(factory, "missing", {}, true));
    check(applicationFallbackIcons("NotSettings;", false) != applicationFallbackIcons("Settings;", false));
    check(applicationFallbackIcons("Settings;", true) == applicationFallbackIcons("TerminalEmulator;", false));
    check(applicationFallbackIcons("Mixer;", false).front() == "audio-volume-high");
    check(applicationFallbackIcons("Video;", false).front() == "camera-video");
    std::cout << "Native icon lookup, fallbacks and disabled-icon checks passed\n";
}
