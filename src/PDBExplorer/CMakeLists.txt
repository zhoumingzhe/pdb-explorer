add_executable(PDBExplorer
    PDBExplorer.cpp
)

target_link_libraries(PDBExplorer
    PRIVATE
    PDBLoader
)

target_compile_definitions(PDBExplorer PRIVATE
    -DUNICODE
    -D_UNICODE
)