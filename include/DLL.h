#include <windows.h>
namespace PDB
{
    class DLL
    {
    private:
        HMODULE m_hModule;
    public:

        // Constructors
        DLL() = delete;
        DLL(const DLL&) = delete;
        DLL(DLL&& rhs);
        DLL(HMODULE hModule);

        // Operators
        DLL operator= (const DLL&) = delete;
        void operator= (DLL&& rhs);

        // Destructor
        ~DLL();

        template<typename T>
        T GetProcAddress(const char* procName) const
        {
            return reinterpret_cast<T>(::GetProcAddress(m_hModule, procName));
        }
    };
}