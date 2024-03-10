#include <string>
#include <map>
#include <set>
#include <dia2.h>
#include <PDBSymbol.h>
namespace PDB
{
    class File
    {
    private:
        std::wstring m_path;
        std::set<std::wstring>& m_stringPool;
        // TODO: Add MD5/SHA hash of the PDB file
        // TODO: Add timestamp of the PDB file

        std::map<std::wstring_view, std::set<Symbol>> &m_symbols;

        // Load user defined types, enums
        bool LoadUDTs(IDiaSession* pSession, IDiaSymbol* pGlobal, enum SymTagEnum tag);
        bool LoadFunctions(IDiaSession* pSession, IDiaSymbol* pGlobal, enum SymTagEnum tag);

    public:
        File(std::set<std::wstring>& stringPool, std::map<std::wstring_view, std::set<Symbol>> &symbols);
        bool Open(const wchar_t* path, IDiaDataSource* pDataSource);

        // Using std::set here. Same symbol can be defined in multiple files.
        std::map<std::wstring_view, std::set<Symbol>>& GetSymbols();
    };
}