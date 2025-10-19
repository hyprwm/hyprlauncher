#pragma once

#include "../IFinder.hpp"

#include <hyprutils/os/FileDescriptor.hpp>

class CIPCEntry;

class CIPCFinder : public IFinder {
  public:
    CIPCFinder();
    virtual ~CIPCFinder() = default;

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query);
    virtual void                       init();

    void                               setData(const std::vector<const char*>& data);
    void                               setData(const std::vector<std::string>& data);

  private:
    std::vector<SP<CIPCEntry>>     m_entries;
    std::vector<SP<IFinderResult>> m_entriesGeneric;

    friend class CIPCEntry;
};

inline UP<CIPCFinder> g_ipcFinder;
