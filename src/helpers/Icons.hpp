#pragma once

#include <string>
#include <vector>
#include <hyprtoolkit/system/Icons.hpp>

Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> applicationIcon(Hyprtoolkit::ISystemIconFactory& factory, const std::string& name,
                                                                                       const std::vector<std::string>& fallbacks, bool enabled);

std::vector<std::string>                                               applicationFallbackIcons(const std::string& categories, bool terminal);
