add_library(PDBLoader
    DLL.cpp
    PDBFile.cpp
    PDBLoader.cpp
    PDBSymbol.cpp
    PDBType.cpp
)

target_include_directories(PDBLoader
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/DIASDK/include
)