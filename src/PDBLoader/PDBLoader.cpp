#include <DLL.h>
#include <dia2.h>
#include <iostream>
#include <wrl/client.h>
namespace PDB
{
    typedef HRESULT (*pfnDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

    HRESULT CreateDiaDataSource(const PDB::DLL &msdia, IDiaDataSource** ppDataSource)
    {
        if (!ppDataSource)
        {
            return E_INVALIDARG;
        }

        pfnDllGetClassObject dllGetClassObject = msdia.GetProcAddress<pfnDllGetClassObject>("DllGetClassObject");
        if (!dllGetClassObject)
        {
            return E_NOTIMPL;
        }

        Microsoft::WRL::ComPtr<IClassFactory> pClassFactory;
        HRESULT hr = dllGetClassObject(__uuidof(DiaSource), __uuidof(IClassFactory), (void **)&pClassFactory);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to get class factory of DiaSource" << std::endl;
            return hr;
        }

        Microsoft::WRL::ComPtr<IDiaDataSource> pSource;
        hr = pClassFactory->CreateInstance(nullptr, __uuidof(IDiaDataSource), (void **)&pSource);
        if (FAILED(hr))
        {
            std::wcerr << L"Failed to create IDiaDataSource instance" << std::endl;
            return hr;
        }
        *ppDataSource = pSource.Detach();
        return S_OK;
    }
}