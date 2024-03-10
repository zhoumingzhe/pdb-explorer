#include <DLL.h>
#include <dia2.h>
#include <iostream>
#include <PDBFile.h>
#include <PDBLoader.h>
#include <dia2.h>
#include <atlbase.h>
#include <filesystem>
#include <assert.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>

std::mutex g_inputMutex;
std::condition_variable g_inputCV;
std::list<std::wstring> g_inputQueue;
bool g_inputDone = false;

std::mutex g_outputMutex;
std::condition_variable g_outputCV;
struct Output
{
    std::set<std::wstring> stringPool;
    std::map<std::wstring_view, std::set<PDB::Symbol>> symbols;
};
std::list<Output> g_outputQueue;
bool g_outputDone = false;

PDB::DLL* g_pMsdia;

std::set<std::wstring> g_stringPool;
std::map<std::wstring_view, std::set<PDB::Symbol>> g_symbols;

void LoadPDBThread()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(g_inputMutex);
        g_inputCV.wait(lock, [] { return !g_inputQueue.empty() || g_inputDone; });
        if (g_inputQueue.empty())
        {
            if (g_inputDone)
            {
                return;
            }
            continue;
        }
        std::wstring path = std::move(g_inputQueue.front());
        g_inputQueue.pop_front();
        lock.unlock();

        CComPtr<IDiaDataSource> pDiaDataSource;
        HRESULT hr = PDB::CreateDiaDataSource(*g_pMsdia, &pDiaDataSource);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to create DIA data source" << std::endl;
            return;
        }
        Output output;
        PDB::File f(output.stringPool, output.symbols);
        std::wcout << L"Opening PDB file: " << path.c_str() << std::endl;
        if (!f.Open(path.c_str(), pDiaDataSource))
        {
            std::wcerr << L"Failed to open PDB file" << path.c_str() << std::endl;
            continue;
        }

        std::unique_lock<std::mutex> lock2(g_outputMutex);
        g_outputQueue.emplace_back(std::move(output));
        lock2.unlock();
        g_outputCV.notify_one();
    }
}

void MergeSymbolThread()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(g_outputMutex);
        g_outputCV.wait(lock, [] { return !g_outputQueue.empty() || g_outputDone; });
        if (g_outputQueue.empty())
        {
            if (g_inputDone)
            {
                return;
            }
            continue;
        }
        Output output = std::move(g_outputQueue.front());
        g_outputQueue.pop_front();
        lock.unlock();

        for (auto & [name, symbols] : output.symbols)
        {
            for (auto &s : symbols)
            {
                auto iterName = g_stringPool.insert(std::wstring(s.m_name));
                auto iterFileName = g_stringPool.insert(std::wstring(s.m_fileName));
                g_symbols[std::wstring_view(*iterName.first)].emplace(PDB::Symbol{ std::wstring_view(*iterName.first), std::wstring_view(*iterFileName.first), s.m_lineNumber });
            }
        }

    }

}
int wmain(int argc, wchar_t *argv[])
{
    if (argc < 2)
    {
        std::wcerr << L"Usage: PDBExplorer <folder to PDB file>" << std::endl;
        return 1;
    }
    {
        HMODULE hModule = LoadLibraryA("msdia140.dll");
        PDB::DLL msdia(hModule);
        g_pMsdia = &msdia;

        std::vector<std::thread> loaderThreads;
        for (size_t i = 0; i < std::thread::hardware_concurrency() - 1; i++)
        {
            loaderThreads.emplace_back(LoadPDBThread);
        }

        std::thread mergeThread(MergeSymbolThread);
        std::filesystem::recursive_directory_iterator it(argv[1]);
        for (auto &entry : it)
        {
            const std::wstring pdbExtension = L".pdb";
            if (entry.path().extension() == pdbExtension)
            {
                std::wstring path = entry.path().wstring();
                assert(path.size() > pdbExtension.size());
                std::wstring removedpdb = path.substr(0, path.size() - pdbExtension.size());
                if (!std::filesystem::exists(removedpdb + L".dll") && !std::filesystem::exists(removedpdb + L".exe"))
                {
                    std::wcout << L"Skipping " << path << L" as it doesn't have a corresponding .dll or .exe file" << std::endl;
                    continue;
                }
                std::unique_lock<std::mutex> lock(g_inputMutex);
                g_inputQueue.emplace_back(std::move(path));
                lock.unlock();
                g_inputCV.notify_one();
            }
        }
        std::unique_lock<std::mutex> lock(g_inputMutex);
        g_inputDone = true;
        lock.unlock();
        g_inputCV.notify_all();
        for (auto &t : loaderThreads)
        {
            t.join();
        }

        std::unique_lock<std::mutex> lock2(g_outputMutex);
        g_outputDone = true;
        lock2.unlock();
        g_outputCV.notify_all();
        mergeThread.join();
    }
    for (;;)
    {
        std::wcout << "Please enter a symbol name: ";
        std::wstring symbolName;
        std::wcin >> symbolName;
        for (auto &[name, symbol] : g_symbols)
        {
            if (name.find(symbolName) != std::wstring::npos)
            {
                std::wcout << name << L": " << std::endl;
                for (auto &s : symbol)
                {
                    std::wcout << s.m_fileName << L"(" << s.m_lineNumber << L")" << std::endl;
                }
                std::wcout << std::endl;
            }
        }
    }
    return 0;
}