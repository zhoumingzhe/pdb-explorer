#include <string>
#include <cstdint>
namespace PDB
{
    class File;
    struct Symbol
    {
        std::wstring_view m_name;
        std::wstring_view m_fileName;
        uint32_t m_lineNumber;
    };
    bool operator < (const Symbol& lhs, const Symbol& rhs);
}