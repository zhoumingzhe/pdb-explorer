
#include <PDBFile.h>
#include <utility>
#include <iostream>
#include <filesystem>
#include <atlbase.h>
namespace PDB
{

    File::File(std::set<std::wstring>& stringPool, std::map<std::wstring_view, std::set<Symbol>> &symbols)
        : m_stringPool(stringPool)
        , m_symbols(symbols)
    {
    }

    bool File::Open(const wchar_t *path, IDiaDataSource *pDataSource)
    {
        CComPtr<IDiaSession> pSession;
        CComPtr<IDiaSymbol> pGlobal;
        HRESULT hr = pDataSource->loadDataFromPdb(path);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to load data from PDB file " << path << std::endl;
            return false;
        }
        hr = pDataSource->openSession(&pSession);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to open session" << std::endl;
            return false;
        }

        hr = pSession->get_globalScope(&pGlobal);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to get global scope" << std::endl;
            return false;
        }

        static bool (File::*pfnLoadSymbols[SymTagMax])(IDiaSession* , IDiaSymbol *, enum SymTagEnum) = {
            nullptr,              // SymTagNull,
            nullptr,              // SymTagExe,
            nullptr,              // SymTagCompiland,
            nullptr,              // SymTagCompilandDetails,
            nullptr,              // SymTagCompilandEnv,
            &File::LoadFunctions, // SymTagFunction,
            nullptr,              // SymTagBlock,
            nullptr,              // SymTagData,
            nullptr,              // SymTagAnnotation,
            nullptr,              // SymTagLabel,
            nullptr,              // SymTagPublicSymbol,
            &File::LoadUDTs,      // SymTagUDT,
            &File::LoadUDTs,      // SymTagEnum,
            nullptr,              // SymTagFunctionType,
            nullptr,              // SymTagPointerType,
            nullptr,              // SymTagArrayType,
            nullptr,              // SymTagBaseType,
            nullptr,              // SymTagTypedef,
            nullptr,              // SymTagBaseClass,
            nullptr,              // SymTagFriend,
            nullptr,              // SymTagFunctionArgType,
            nullptr,              // SymTagFuncDebugStart,
            nullptr,              // SymTagFuncDebugEnd,
            nullptr,              // SymTagUsingNamespace,
            nullptr,              // SymTagVTableShape,
            nullptr,              // SymTagVTable,
            nullptr,              // SymTagCustom,
            nullptr,              // SymTagThunk,
            nullptr,              // SymTagCustomType,
            nullptr,              // SymTagManagedType,
            nullptr,              // SymTagDimension,
            nullptr,              // SymTagCallSite,
            nullptr,              // SymTagInlineSite,
            nullptr,              // SymTagBaseInterface,
            nullptr,              // SymTagVectorType,
            nullptr,              // SymTagMatrixType,
            nullptr,              // SymTagHLSLType,
            nullptr,              // SymTagCaller,
            nullptr,              // SymTagCallee,
            nullptr,              // SymTagExport,
            nullptr,              // SymTagHeapAllocationSite,
            nullptr,              // SymTagCoffGroup,
            nullptr,              // SymTagInlinee,
            nullptr,              // SymTagTaggedUnionCase,
        };

        for (size_t tag = 0; tag < SymTagMax; ++tag)
        {
            auto pfnLoadSymbol = pfnLoadSymbols[tag];
            if (!pfnLoadSymbol)
            {
                continue;
            }
            if (!(this->*pfnLoadSymbols[tag])(pSession, pGlobal, (enum SymTagEnum)tag))
            {
                return false;
            }
        }
        m_path = path;
        return true;
    }

    bool File::LoadFunctions(IDiaSession* pSession, IDiaSymbol *pGlobal, enum SymTagEnum tag)
    {
        CComPtr<IDiaEnumSymbols> pEnumSymbols;
        HRESULT hr = pGlobal->findChildrenEx(tag, nullptr, nsNone, &pEnumSymbols);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to find UDT symbols" << std::endl;
            return false;
        }
        
        for (;;)
        {
            CComPtr<IDiaSymbol> pSymbol;
            ULONG celt = 0;
            if (FAILED(pEnumSymbols->Next(1, &pSymbol, &celt)) || celt == 0)
            {
                break;
            }
            CComBSTR name;
            if (FAILED(pSymbol->get_name(&name)))
            {
                std::wcerr << L"Failed to get function name" << std::endl;
                continue;
            }

            DWORD dwRVA;
            hr = pSymbol->get_relativeVirtualAddress(&dwRVA);
            if (FAILED(hr))
            {
                std::wcerr << L"Failed to get relative virtual address" << std::endl;
                continue;
            }

            CComPtr<IDiaEnumLineNumbers> pEnumLines;
            hr = pSession->findLinesByRVA(dwRVA, sizeof(void*), &pEnumLines);
            if (FAILED(hr))
            {
                std::wcerr << L"Failed to get lines by RVA" << std::endl;
                continue;
            }

            if (!pEnumLines)
            {
                // Stripped symbol file or optimized away
                continue;
            }

            for (;;)
            {
                CComPtr<IDiaLineNumber> pLineNumber;
                DWORD celt;
                hr = pEnumLines->Next(1, &pLineNumber, &celt);
                if (FAILED(hr) || celt == 0)
                {
                    // Stripped symbol file or optimized away
                    break;
                }
                CComPtr<IDiaSourceFile> pSourceFile;
                hr = pLineNumber->get_sourceFile(&pSourceFile);
                if (FAILED(hr))
                {
                    std::wcerr << L"Failed to get source file" << std::endl;
                    continue;
                }

                CComBSTR sourceFileName = nullptr;
                if (FAILED(pSourceFile->get_fileName(&sourceFileName)))
                {
                    std::wcerr << L"Failed to get source file name" << std::endl;
                    continue;
                }
                // We don't want to include file that doesn't exist.
                if (std::filesystem::exists(BSTR(sourceFileName)) == false)
                {
                    break;
                }
                DWORD lineNumber = 0;
                hr = pLineNumber->get_lineNumber(&lineNumber);
                if (FAILED(hr))
                {
                    std::wcerr << L"Failed to get line number" << std::endl;
                    continue;
                }
                auto iterName = m_stringPool.insert(BSTR(name));
                auto iterSourceName = m_stringPool.insert(BSTR(sourceFileName));
                m_symbols[std::wstring_view(*iterName.first)].emplace(Symbol{std::wstring_view(*iterName.first), std::wstring_view(*iterSourceName.first), lineNumber});

                // Break here as only the starting address of the function is needed.
                break;
            }

        }
        return true;
    }

    bool File::LoadUDTs(IDiaSession* pSession, IDiaSymbol *pGlobal, enum SymTagEnum tag)
    {
        CComPtr<IDiaEnumSymbols> pEnumSymbols;
        HRESULT hr = pGlobal->findChildrenEx(tag, nullptr, nsNone, &pEnumSymbols);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to find UDT symbols" << std::endl;
            return false;
        }

        for (;;)
        {
            CComPtr<IDiaSymbol> pSymbol;
            ULONG celt = 0;
            if (FAILED(pEnumSymbols->Next(1, &pSymbol, &celt)) || celt == 0)
            {
                break;
            }
            CComBSTR name;
            if (FAILED(pSymbol->get_name(&name)))
            {
                std::wcerr << L"Failed to get UDT name" << std::endl;
                continue;
            }

            // std::wcout << L"Found UDT: " << BSTR(name) << std::endl;
            CComPtr<IDiaLineNumber> pLineNumber;
            hr = pSymbol->getSrcLineOnTypeDefn(&pLineNumber);
            if (FAILED(hr))
            {
                std::wcerr << L"Failed to get source line" << std::endl;
                continue;
            }

            if (!pLineNumber)
            {
                // Stripped symbol file or optimized away
                continue;
            }

            CComPtr<IDiaSourceFile> sourceFile = nullptr;
            hr = pLineNumber->get_sourceFile(&sourceFile);
            if (FAILED(hr))
            {
                std::wcerr << L"Failed to get source file" << std::endl;
                continue;
            }

            CComBSTR sourceFileName = nullptr;
            if (FAILED(sourceFile->get_fileName(&sourceFileName)))
            {
                std::wcerr << L"Failed to get source file name" << std::endl;
                continue;
            }
            if (!std::filesystem::exists(BSTR(sourceFileName)))
            {
                // We don't want to include file that doesn't exist.
                continue;
            }
            DWORD lineNumber = 0;
            hr = pLineNumber->get_lineNumber(&lineNumber);
            if (FAILED(hr))
            {
                std::wcerr << L"Failed to get line number" << std::endl;
                continue;
            }
            auto iterName = m_stringPool.insert(BSTR(name));
            auto iterSourceName = m_stringPool.insert(BSTR(sourceFileName));
            m_symbols[std::wstring_view(*iterName.first)].emplace(Symbol{std::wstring_view(*iterName.first), std::wstring_view(*iterSourceName.first), lineNumber});
        }
        return true;
    }

    std::map<std::wstring_view, std::set<Symbol>> &File::GetSymbols()
    {
        return m_symbols;
    }

}