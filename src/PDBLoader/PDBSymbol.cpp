#include <PDBSymbol.h>
namespace PDB
{
    bool operator < (const Symbol& lhs, const Symbol& rhs)
    {
        return lhs.m_name < rhs.m_name || 
            (lhs.m_name == rhs.m_name && lhs.m_fileName < rhs.m_fileName) ||
            (lhs.m_name == rhs.m_name && lhs.m_fileName == rhs.m_fileName && lhs.m_lineNumber < rhs.m_lineNumber);
    }
}