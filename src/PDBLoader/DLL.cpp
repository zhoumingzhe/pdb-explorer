#include <DLL.h>
namespace PDB
{
    DLL::DLL(HMODULE hModule)
        :m_hModule(hModule)
    {
    }
    DLL::DLL(DLL&& rhs)
        :m_hModule(rhs.m_hModule)
    {
        rhs.m_hModule = NULL;
    }

    void DLL::operator= (DLL&& rhs)
    {
        m_hModule = rhs.m_hModule;
        rhs.m_hModule = NULL;
    }

    DLL::~DLL()
    {
        if (m_hModule)
        {
            FreeLibrary(m_hModule);
        }
    }
}